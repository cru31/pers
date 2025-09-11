#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <algorithm>
#include <sstream>

namespace pers {

DeviceBuffer::DeviceBuffer(const BufferDesc& desc, const std::shared_ptr<IBufferFactory>& factory)
    : _desc(desc)
    , _totalBytesTransferred(0)
    , _transferCount(0) {
    
    if (!desc.isValid()) {
        LOG_ERROR("DeviceBuffer", "Invalid buffer description");
        return;
    }
    
    if (!factory) {
        LOG_ERROR("DeviceBuffer", "Factory is null");
        return;
    }
    
    // Force GPU-only configuration
    BufferDesc deviceDesc = desc;
    deviceDesc.mappedAtCreation = false;
    deviceDesc.usage |= BufferUsage::CopyDst;
    
    if (deviceDesc.memoryLocation == MemoryLocation::Auto) {
        deviceDesc.memoryLocation = MemoryLocation::DeviceLocal;
    }
    
    _buffer = factory->createBuffer(deviceDesc);
    if (!_buffer) {
        LOG_ERROR("DeviceBuffer", "Failed to create underlying buffer");
        return;
    }
    
    std::stringstream ss;
    ss << "Created device buffer '" << _desc.debugName << "' size=" << _desc.size 
       << " usage=0x" << std::hex << static_cast<uint32_t>(_desc.usage);
    LOG_DEBUG("DeviceBuffer", ss.str().c_str());
}

DeviceBuffer::~DeviceBuffer() {
    if (_transferCount > 0) {
        std::stringstream ss;
        ss << "Destroyed device buffer '" << _desc.debugName 
           << "' - total transfers: " << _transferCount 
           << ", bytes: " << _totalBytesTransferred;
        LOG_DEBUG("DeviceBuffer", ss.str().c_str());
    }
}

DeviceBuffer::DeviceBuffer(DeviceBuffer&& other) noexcept
    : _buffer(std::move(other._buffer))
    , _desc(std::move(other._desc))
    , _totalBytesTransferred(other._totalBytesTransferred)
    , _transferCount(other._transferCount) {
    other._totalBytesTransferred = 0;
    other._transferCount = 0;
}

DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& other) noexcept {
    if (this != &other) {
        _buffer = std::move(other._buffer);
        _desc = std::move(other._desc);
        _totalBytesTransferred = other._totalBytesTransferred;
        _transferCount = other._transferCount;
        other._totalBytesTransferred = 0;
        other._transferCount = 0;
    }
    return *this;
}

bool DeviceBuffer::copyFrom(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<IBuffer>& source,
                            const BufferCopyDesc& copyDesc) {
    if (!encoder) {
        LOG_ERROR("DeviceBuffer", "copyFrom: encoder is null");
        return false;
    }
    
    if (!source) {
        LOG_ERROR("DeviceBuffer", "copyFrom: source is null");
        return false;
    }
    
    uint64_t size = copyDesc.size;
    if (size == BufferCopyDesc::WHOLE_SIZE) {
        size = std::min(source->getSize() - copyDesc.srcOffset, 
                       getSize() - copyDesc.dstOffset);
    }
    
    // Device-to-Device copy
    auto srcDevice = std::dynamic_pointer_cast<DeviceBuffer>(source);
    if (srcDevice) {
        bool result = encoder->copyDeviceToDevice(srcDevice, 
                                                 std::static_pointer_cast<DeviceBuffer>(shared_from_this()), 
                                                 copyDesc);
        if (result) {
            _totalBytesTransferred += size;
            _transferCount++;
        }
        return result;
    }
    
    LOG_ERROR("DeviceBuffer", "copyFrom: source is not a DeviceBuffer");
    return false;
}

bool DeviceBuffer::copyTo(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<IBuffer>& destination,
                          const BufferCopyDesc& copyDesc) {
    if (!encoder) {
        LOG_ERROR("DeviceBuffer", "copyTo: encoder is null");
        return false;
    }
    
    if (!destination) {
        LOG_ERROR("DeviceBuffer", "copyTo: destination is null");
        return false;
    }
    
    uint64_t size = copyDesc.size;
    if (size == BufferCopyDesc::WHOLE_SIZE) {
        size = std::min(getSize() - copyDesc.srcOffset, 
                       destination->getSize() - copyDesc.dstOffset);
    }
    
    // Device-to-Device copy
    auto dstDevice = std::dynamic_pointer_cast<DeviceBuffer>(destination);
    if (dstDevice) {
        bool result = encoder->copyDeviceToDevice(std::static_pointer_cast<DeviceBuffer>(shared_from_this()),
                                                 dstDevice,
                                                 copyDesc);
        if (result) {
            _totalBytesTransferred += size;
            _transferCount++;
        }
        return result;
    }
    
    LOG_ERROR("DeviceBuffer", "copyTo: destination is not a DeviceBuffer");
    return false;
}

uint64_t DeviceBuffer::getSize() const {
    return _buffer ? _buffer->getSize() : 0;
}

BufferUsage DeviceBuffer::getUsage() const {
    return _buffer ? _buffer->getUsage() : BufferUsage::None;
}

const std::string& DeviceBuffer::getDebugName() const {
    return _desc.debugName;
}

NativeBufferHandle DeviceBuffer::getNativeHandle() const {
    return _buffer ? _buffer->getNativeHandle() : NativeBufferHandle::fromBackend(nullptr);
}

bool DeviceBuffer::isValid() const {
    return _buffer && _buffer->isValid();
}

BufferState DeviceBuffer::getState() const {
    return _buffer ? _buffer->getState() : BufferState::Uninitialized;
}

MemoryLocation DeviceBuffer::getMemoryLocation() const {
    return _desc.memoryLocation;
}

AccessPattern DeviceBuffer::getAccessPattern() const {
    return _desc.accessPattern;
}

} // namespace pers