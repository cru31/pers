#include "pers/graphics/buffers/DeferredStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <cstring>
#include <sstream>

namespace pers {
namespace graphics {

DeferredStagingBuffer::DeferredStagingBuffer(const BufferDesc& desc)
    : _desc(desc)
    , _currentMapping(nullptr, 0, nullptr)
    , _mappingPending(false) {
    
    if (!desc.isValid()) {
        LOG_ERROR("DeferredStagingBuffer", "Invalid buffer description");
        return;
    }
    
    // Force deferred mapping configuration
    _desc.mappedAtCreation = false;
    _desc.usage |= BufferUsage::MapRead | BufferUsage::MapWrite;
    
    // Readback buffers primarily need CopyDst
    _desc.usage |= BufferUsage::CopyDst;
    
    if (_desc.memoryLocation == MemoryLocation::Auto) {
        _desc.memoryLocation = MemoryLocation::HostVisible;
    }
    
    std::stringstream ss;
    ss << "Created deferred staging buffer '" << _desc.debugName << "' size=" << _desc.size;
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
}

DeferredStagingBuffer::~DeferredStagingBuffer() {
    if (isMapped()) {
        std::stringstream ss;
        ss << "Buffer '" << _desc.debugName << "' destroyed while mapped";
        LOG_WARNING("DeferredStagingBuffer", ss.str().c_str());
        unmap();
    }
}

DeferredStagingBuffer::DeferredStagingBuffer(DeferredStagingBuffer&& other) noexcept
    : _desc(std::move(other._desc))
    , _mappingFuture(std::move(other._mappingFuture))
    , _currentMapping(std::move(other._currentMapping))
    , _mappingPending(other._mappingPending) {
    other._mappingPending = false;
}

DeferredStagingBuffer& DeferredStagingBuffer::operator=(DeferredStagingBuffer&& other) noexcept {
    if (this != &other) {
        if (isMapped()) {
            unmap();
        }
        
        _desc = std::move(other._desc);
        _mappingFuture = std::move(other._mappingFuture);
        _currentMapping = std::move(other._currentMapping);
        _mappingPending = other._mappingPending;
        
        other._mappingPending = false;
    }
    return *this;
}


uint64_t DeferredStagingBuffer::getSize() const {
    return _desc.size;
}

BufferUsage DeferredStagingBuffer::getUsage() const {
    return _desc.usage;
}

bool DeferredStagingBuffer::writeBytes(const void* data, uint64_t size, uint64_t offset) {
    if (!data) {
        LOG_ERROR("DeferredStagingBuffer", "Source data is null");
        return false;
    }
    
    if (offset + size > _desc.size) {
        std::stringstream ss;
        ss << "Write exceeds buffer size: offset=" << offset << " + size=" << size << " > buffer_size=" << _desc.size;
        LOG_ERROR("DeferredStagingBuffer", ss.str().c_str());
        return false;
    }
    
    // Wait for pending mapping if necessary
    if (_mappingPending && _mappingFuture.valid()) {
        _currentMapping = _mappingFuture.get();
        _mappingPending = false;
    }
    
    if (!_currentMapping.data()) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer is not mapped");
        return false;
    }
    
    uint8_t* dst = static_cast<uint8_t*>(_currentMapping.data()) + offset;
    std::memcpy(dst, data, size);
    
    std::stringstream ss;
    ss << "Wrote " << size << " bytes at offset " << offset << " to buffer '" << _desc.debugName << "'";
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
}

bool DeferredStagingBuffer::readBytes(void* data, uint64_t size, uint64_t offset) const {
    if (!data) {
        LOG_ERROR("DeferredStagingBuffer", "Destination data is null");
        return false;
    }
    
    if (offset + size > _desc.size) {
        std::stringstream ss;
        ss << "Read exceeds buffer size: offset=" << offset << " + size=" << size << " > buffer_size=" << _desc.size;
        LOG_ERROR("DeferredStagingBuffer", ss.str().c_str());
        return false;
    }
    
    // Wait for pending mapping if necessary
    if (_mappingPending && _mappingFuture.valid()) {
        const_cast<DeferredStagingBuffer*>(this)->_currentMapping = 
            const_cast<DeferredStagingBuffer*>(this)->_mappingFuture.get();
        const_cast<DeferredStagingBuffer*>(this)->_mappingPending = false;
    }
    
    if (!_currentMapping.data()) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer is not mapped");
        return false;
    }
    
    const uint8_t* src = static_cast<const uint8_t*>(_currentMapping.data()) + offset;
    std::memcpy(data, src, size);
    
    std::stringstream ss;
    ss << "Read " << size << " bytes at offset " << offset << " from buffer '" << _desc.debugName << "'";
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
}

bool DeferredStagingBuffer::downloadFrom(const std::shared_ptr<ICommandEncoder>& encoder,
                                         const std::shared_ptr<DeviceBuffer>& source,
                                         const BufferCopyDesc& copyDesc) {
    if (!encoder) {
        LOG_ERROR("DeferredStagingBuffer", "downloadFrom: encoder is null");
        return false;
    }
    
    if (!source) {
        LOG_ERROR("DeferredStagingBuffer", "downloadFrom: source is null");
        return false;
    }
    
    // Ensure buffer is unmapped before transfer
    if (isMapped()) {
        unmap();
    }
    
    uint64_t size = copyDesc.size;
    if (size == BufferCopyDesc::WHOLE_SIZE) {
        size = std::min(source->getSize() - copyDesc.srcOffset, 
                       _desc.size - copyDesc.dstOffset);
    }
    
    if (size == 0) {
        LOG_WARNING("DeferredStagingBuffer", "Nothing to download (size=0)");
        return true;
    }
    
    // Use the specific download method on the encoder
    auto deferredBuffer = std::dynamic_pointer_cast<DeferredStagingBuffer>(shared_from_this());
    if (!deferredBuffer) {
        LOG_ERROR("DeferredStagingBuffer", "Failed to get shared_ptr to self");
        return false;
    }
    
    bool result = encoder->downloadFromDeviceBuffer(source, deferredBuffer, copyDesc);
    
    if (result) {
        std::stringstream ss;
        ss << "Downloaded " << size << " bytes from device to '" << _desc.debugName << "'";
        LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    }
    
    return result;
}

} // namespace graphics
} // namespace pers