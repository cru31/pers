#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <cstring>
#include <algorithm>
#include <sstream>

namespace pers {
namespace graphics {

ImmediateStagingBuffer::ImmediateStagingBuffer(const BufferDesc& desc)
    : _desc(desc)
    , _mappedData(nullptr)
    , _finalized(false)
    , _bytesWritten(0) {
    
    if (!desc.isValid()) {
        LOG_ERROR("ImmediateStagingBuffer", "Invalid buffer description");
        return;
    }
    
    // Force immediate mapping configuration
    _desc.mappedAtCreation = true;
    _desc.usage |= BufferUsage::CopySrc;
    
    if (_desc.memoryLocation == MemoryLocation::Auto) {
        _desc.memoryLocation = MemoryLocation::HostVisible;
    }
    
    std::stringstream ss;
    ss << "Created staging buffer '" << _desc.debugName << "' size=" << _desc.size << " mapped=true";
    LOG_DEBUG("ImmediateStagingBuffer", ss.str().c_str());
}

ImmediateStagingBuffer::~ImmediateStagingBuffer() {
    if (!_finalized) {
        std::stringstream ss;
        ss << "Buffer '" << _desc.debugName << "' destroyed without being finalized";
        LOG_WARNING("ImmediateStagingBuffer", ss.str().c_str());
        finalize();
    }
}

ImmediateStagingBuffer::ImmediateStagingBuffer(ImmediateStagingBuffer&& other) noexcept
    : _desc(std::move(other._desc))
    , _mappedData(other._mappedData)
    , _finalized(other._finalized)
    , _bytesWritten(other._bytesWritten) {
    other._mappedData = nullptr;
    other._finalized = true;
    other._bytesWritten = 0;
}

ImmediateStagingBuffer& ImmediateStagingBuffer::operator=(ImmediateStagingBuffer&& other) noexcept {
    if (this != &other) {
        if (!_finalized) {
            finalize();
        }
        
        _desc = std::move(other._desc);
        _mappedData = other._mappedData;
        _finalized = other._finalized;
        _bytesWritten = other._bytesWritten;
        
        other._mappedData = nullptr;
        other._finalized = true;
        other._bytesWritten = 0;
    }
    return *this;
}

void ImmediateStagingBuffer::writeBytes(const void* data, uint64_t size, uint64_t offset) {
    if (_finalized) {
        LOG_ERROR("ImmediateStagingBuffer", "Cannot write to finalized buffer");
        return;
    }
    
    if (!_mappedData) {
        LOG_ERROR("ImmediateStagingBuffer", "Buffer is not mapped");
        return;
    }
    
    if (!data) {
        LOG_ERROR("ImmediateStagingBuffer", "Source data is null");
        return;
    }
    
    if (offset + size > _desc.size) {
        std::stringstream ss;
        ss << "Write exceeds buffer size: offset=" << offset << " + size=" << size << " > buffer_size=" << _desc.size;
        LOG_ERROR("ImmediateStagingBuffer", ss.str().c_str());
        return;
    }
    
    uint8_t* dst = static_cast<uint8_t*>(_mappedData) + offset;
    std::memcpy(dst, data, size);
    
    _bytesWritten = std::max(_bytesWritten, offset + size);
    
    std::stringstream ss2;
    ss2 << "Wrote " << size << " bytes at offset " << offset << " to buffer '" << _desc.debugName << "'";
    LOG_DEBUG("ImmediateStagingBuffer", ss2.str().c_str());
}

void ImmediateStagingBuffer::finalize() {
    if (_finalized) {
        return;
    }
    
    if (isMapped()) {
        unmap();
    }
    
    _mappedData = nullptr;
    _finalized = true;
    
    std::stringstream ss;
    ss << "Finalized buffer '" << _desc.debugName << "' with " << _bytesWritten << " bytes written";
    LOG_DEBUG("ImmediateStagingBuffer", ss.str().c_str());
}

bool ImmediateStagingBuffer::uploadTo(const std::shared_ptr<ICommandEncoder>& encoder, 
                                      const std::shared_ptr<DeviceBuffer>& target,
                                      const BufferCopyDesc& copyDesc) {
    return uploadTo(encoder, std::static_pointer_cast<IBuffer>(target), copyDesc);
}

bool ImmediateStagingBuffer::uploadTo(const std::shared_ptr<ICommandEncoder>& encoder, 
                                      const std::shared_ptr<IBuffer>& target,
                                      const BufferCopyDesc& copyDesc) {
    if (!encoder) {
        LOG_ERROR("ImmediateStagingBuffer", "uploadTo: encoder is null");
        return false;
    }
    
    if (!target) {
        LOG_ERROR("ImmediateStagingBuffer", "uploadTo: target is null");
        return false;
    }
    
    if (!isValid()) {
        LOG_ERROR("ImmediateStagingBuffer", "uploadTo: source buffer not valid");
        return false;
    }
    
    if (!_finalized) {
        LOG_WARNING("ImmediateStagingBuffer", "Buffer not finalized, finalizing now");
        finalize();
    }
    
    uint64_t size = copyDesc.size;
    if (size == BufferCopyDesc::WHOLE_SIZE) {
        size = std::min(_bytesWritten - copyDesc.srcOffset, 
                       target->getSize() - copyDesc.dstOffset);
    }
    
    if (size == 0) {
        LOG_WARNING("ImmediateStagingBuffer", "Nothing to upload (size=0)");
        return true;
    }
    
    // ImmediateStagingBuffer to DeviceBuffer upload
    auto deviceTarget = std::dynamic_pointer_cast<graphics::DeviceBuffer>(target);
    if (deviceTarget) {
        bool result = encoder->uploadToDeviceBuffer(
            std::static_pointer_cast<ImmediateStagingBuffer>(shared_from_this()),
            deviceTarget,
            copyDesc);
        if (result) {
            std::stringstream ss;
            ss << "Uploaded " << size << " bytes from '" << _desc.debugName << "' to target";
            LOG_DEBUG("ImmediateStagingBuffer", ss.str().c_str());
        }
        return result;
    }
    
    LOG_ERROR("ImmediateStagingBuffer", "uploadTo: target is not a DeviceBuffer");
    return false;
}

bool ImmediateStagingBuffer::isFinalized() const {
    return _finalized;
}

uint64_t ImmediateStagingBuffer::getBytesWritten() const {
    return _bytesWritten;
}

const std::string& ImmediateStagingBuffer::getDebugName() const {
    return _desc.debugName;
}

BufferState ImmediateStagingBuffer::getState() const {
    if (_finalized) {
        return BufferState::Ready;
    } else if (_mappedData) {
        return BufferState::Mapped;
    } else {
        return BufferState::Uninitialized;
    }
}

MemoryLocation ImmediateStagingBuffer::getMemoryLocation() const {
    return _desc.memoryLocation;
}

AccessPattern ImmediateStagingBuffer::getAccessPattern() const {
    return _desc.accessPattern;
}

} // namespace graphics
} // namespace pers