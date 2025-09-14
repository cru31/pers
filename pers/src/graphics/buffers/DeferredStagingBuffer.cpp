#include "pers/graphics/buffers/INativeMappableBuffer.h"
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
    : _size(0)
    , _usage(BufferUsage::None)
    , _debugName()
    , _mapMode(MapMode::Write)
    , _currentMapping{nullptr, 0, 0}
    , _mappingPending(false)
    , _created(false) {
}

DeferredStagingBuffer::~DeferredStagingBuffer() {
    destroy();
}

bool DeferredStagingBuffer::create(uint64_t size,
                                   MapMode mapMode,
                                   const std::shared_ptr<ILogicalDevice>& device,
                                   const std::string& debugName) {
    if (_created) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer already created");
        return false;
    }
    
    if (size == 0) {
        LOG_ERROR("DeferredStagingBuffer", "Invalid buffer size (0)");
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
    
    _size = size;
    _debugName = debugName;
    _mapMode = mapMode;
    
    // Build internal BufferDesc with appropriate settings
    BufferDesc stagingDesc;
    stagingDesc.size = size;
    stagingDesc.debugName = debugName;
    stagingDesc.mappedAtCreation = false;  // Always false for deferred mapping
    stagingDesc.memoryLocation = MemoryLocation::HostVisible;  // Always host visible for staging
    
    // Set usage based on map mode
    if (mapMode == MapMode::Read) {
        stagingDesc.usage = BufferUsage::MapRead | BufferUsage::CopyDst;  // For GPU->CPU readback
        _usage = stagingDesc.usage;
    } else if (mapMode == MapMode::Write) {
        stagingDesc.usage = BufferUsage::MapWrite | BufferUsage::CopySrc;  // For CPU->GPU upload
        _usage = stagingDesc.usage;
    } else {
        LOG_ERROR("DeferredStagingBuffer", "Invalid map mode");
        return false;
    }
    
    _buffer = resourceFactory->createMappableBuffer(stagingDesc);
    if (!_buffer) {
        LOG_ERROR("DeferredStagingBuffer", "Failed to create underlying buffer");
        return false;
    }
    
    _created = true;
    _mappingPending = false;
    
    std::stringstream ss;
    ss << "Created deferred staging buffer '" << _debugName << "' size=" << _size;
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
}

void DeferredStagingBuffer::destroy() {
    if (!_created) {
        return;
    }
    
    if (isMapped()) {
        std::stringstream ss;
        ss << "Buffer '" << _debugName << "' destroyed while mapped";
        LOG_WARNING("DeferredStagingBuffer", ss.str().c_str());
        unmap();
    }
    
    _buffer.reset();
    _currentMapping = MappedData{nullptr, 0, nullptr};
    _mappingPending = false;
    _created = false;
    
    LOG_DEBUG("DeferredStagingBuffer", "Destroyed deferred staging buffer");
}



bool DeferredStagingBuffer::writeBytes(const void* data, uint64_t size, uint64_t offset) {
    if (!_created || !data) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer not created or source data is null");
        return false;
    }
    
    if (offset + size > _size) {
        std::stringstream ss;
        ss << "Write exceeds buffer size: offset=" << offset << " + size=" << size << " > buffer_size=" << _size;
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
    ss << "Wrote " << size << " bytes at offset " << offset << " to buffer '" << _debugName << "'";
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
}
bool DeferredStagingBuffer::readBytes(void* data, uint64_t size, uint64_t offset) const {
    if (!_created || !data) {
        LOG_ERROR("DeferredStagingBuffer", "Buffer not created or destination data is null");
        return false;
    }
    
    if (offset + size > _size) {
        std::stringstream ss;
        ss << "Read exceeds buffer size: offset=" << offset << " + size=" << size << " > buffer_size=" << _size;
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
    ss << "Read " << size << " bytes at offset " << offset << " from buffer '" << _debugName << "'";
    LOG_DEBUG("DeferredStagingBuffer", ss.str().c_str());
    
    return true;
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
    return _created ? _debugName : empty;
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
    return AccessPattern::Dynamic;
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

