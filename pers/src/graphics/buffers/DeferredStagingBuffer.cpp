#include "pers/graphics/buffers/DeferredStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <cstring>
#include <sstream>
#include <future>

namespace pers {

DeferredStagingBuffer::DeferredStagingBuffer()
    : _desc()
    , _currentMapping{nullptr, 0, 0}
    , _mappingPending(false)
    , _created(false) {
}

DeferredStagingBuffer::~DeferredStagingBuffer() {
    destroy();
}

bool DeferredStagingBuffer::create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device) {
    if (_created) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer already created");
        return false;
    }
    
    if (!desc.isValid()) {
        LOG_ERROR("DeferredStagingBuffer", "Invalid buffer description");
        return false;
    }
    
    if (!device) {
        LOG_ERROR("DeferredStagingBuffer", "Device is null");
        return false;
    }
    
    const auto& resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        LOG_ERROR("DeferredStagingBuffer", "Failed to get resource factory from device");
        return false;
    }
    
    _desc = desc;
    
    // Force deferred mapping configuration
    BufferDesc stagingDesc = desc;
    stagingDesc.mappedAtCreation = false;
    
    // Don't force both MapRead and MapWrite - respect what the user requested
    // The user should specify MapRead for readback or MapWrite for upload
    // We just ensure the corresponding COPY flags are present for WebGPU compliance
    if (static_cast<uint32_t>(stagingDesc.usage & BufferUsage::MapRead)) {
        stagingDesc.usage |= BufferUsage::CopyDst;  // For GPU->CPU readback
    }
    if (static_cast<uint32_t>(stagingDesc.usage & BufferUsage::MapWrite)) {
        stagingDesc.usage |= BufferUsage::CopySrc;  // For CPU->GPU upload
    }
    
    if (stagingDesc.memoryLocation == MemoryLocation::Auto) {
        stagingDesc.memoryLocation = MemoryLocation::HostVisible;
    }
    
    _buffer = resourceFactory->createMappableBuffer(stagingDesc);
    if (!_buffer) {
        LOG_ERROR("DeferredStagingBuffer", "Failed to create underlying buffer");
        return false;
    }
    
    _created = true;
    _mappingPending = false;
    
    std::stringstream ss;
    ss << "Created deferred staging buffer '" << _desc.debugName << "' size=" << _desc.size;
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
}

void DeferredStagingBuffer::destroy() {
    if (!_created) {
        return;
    }
    
    if (isMapped()) {
        std::stringstream ss;
        ss << "Buffer '" << _desc.debugName << "' destroyed while mapped";
        LOG_WARNING("DeferredStagingBuffer", ss.str().c_str());
        unmap();
    }
    
    _buffer.reset();
    _currentMapping = MappedData{nullptr, 0, nullptr};
    _mappingPending = false;
    _created = false;
    _desc = BufferDesc();
    
    LOG_DEBUG("DeferredStagingBuffer", "Destroyed deferred staging buffer");
}

DeferredStagingBuffer::DeferredStagingBuffer(DeferredStagingBuffer&& other) noexcept
    : _buffer(std::move(other._buffer))
    , _desc(std::move(other._desc))
    , _currentMapping(std::move(other._currentMapping))
    , _mappingFuture(std::move(other._mappingFuture))
    , _mappingPending(other._mappingPending)
    , _created(other._created) {
    other._currentMapping = MappedData{nullptr, 0, nullptr};
    other._mappingPending = false;
    other._created = false;
}

DeferredStagingBuffer& DeferredStagingBuffer::operator=(DeferredStagingBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        
        _buffer = std::move(other._buffer);
        _desc = std::move(other._desc);
        _currentMapping = std::move(other._currentMapping);
        _mappingFuture = std::move(other._mappingFuture);
        _mappingPending = other._mappingPending;
        _created = other._created;
        
        other._currentMapping = MappedData{nullptr, 0, nullptr};
        other._mappingPending = false;
        other._created = false;
    }
    return *this;
}

bool DeferredStagingBuffer::writeBytes(const void* data, uint64_t size, uint64_t offset) {
    if (!_created || !data) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer not created or source data is null");
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
    if (!_created || !data) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer not created or destination data is null");
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
    if (!_created) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer not created");
        return false;
    }
    
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

// IBuffer interface
uint64_t DeferredStagingBuffer::getSize() const {
    return _created && _buffer ? _buffer->getSize() : 0;
}

BufferUsage DeferredStagingBuffer::getUsage() const {
    return _created && _buffer ? _buffer->getUsage() : BufferUsage::None;
}

const std::string& DeferredStagingBuffer::getDebugName() const {
    static const std::string empty;
    return _created ? _desc.debugName : empty;
}

NativeBufferHandle DeferredStagingBuffer::getNativeHandle() const {
    return _created && _buffer ? _buffer->getNativeHandle() : NativeBufferHandle{};
}

bool DeferredStagingBuffer::isValid() const {
    return _created && _buffer && _buffer->isValid();
}

BufferState DeferredStagingBuffer::getState() const {
    if (!_created || !_buffer) return BufferState::Uninitialized;
    if (isMapped()) return BufferState::Mapped;
    if (_mappingPending) return BufferState::MapPending;
    return BufferState::Ready;
}

MemoryLocation DeferredStagingBuffer::getMemoryLocation() const {
    return _created && _buffer ? _buffer->getMemoryLocation() : MemoryLocation::Auto;
}

AccessPattern DeferredStagingBuffer::getAccessPattern() const {
    return _desc.accessPattern;
}

// IMappableBuffer interface
void* DeferredStagingBuffer::getMappedData() {
    return _currentMapping.data();
}

const void* DeferredStagingBuffer::getMappedData() const {
    return _currentMapping.data();
}

std::future<MappedData> DeferredStagingBuffer::mapAsync(MapMode mode, const BufferMapRange& range) {
    if (!_created || !_buffer) {
        std::promise<MappedData> promise;
        promise.set_value(MappedData{nullptr, 0, nullptr});
        return promise.get_future();
    }
    
    _mappingPending = true;
    
    // Can't copy future, need to get a new one from buffer
    // The buffer should handle multiple mapAsync calls properly
    return _buffer->mapAsync(mode, range);
}

void DeferredStagingBuffer::unmap() {
    if (_created && _buffer) {
        _buffer->unmap();
        _currentMapping = MappedData{nullptr, 0, nullptr};
        _mappingPending = false;
    }
}

bool DeferredStagingBuffer::isMapped() const {
    return _created && _buffer && _buffer->isMapped();
}

bool DeferredStagingBuffer::isMapPending() const {
    return _mappingPending;
}

void DeferredStagingBuffer::flushMappedRange(uint64_t offset, uint64_t size) {
    if (_created && _buffer) {
        _buffer->flushMappedRange(offset, size);
    }
}

void DeferredStagingBuffer::invalidateMappedRange(uint64_t offset, uint64_t size) {
    if (_created && _buffer) {
        _buffer->invalidateMappedRange(offset, size);
    }
}

} // namespace pers

