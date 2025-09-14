#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <future>

namespace pers {

// Helper to align buffer size to 4-byte boundary for WebGPU
static size_t alignBufferSize(size_t size) {
    const size_t COPY_BUFFER_ALIGNMENT = 4;
    if (size % COPY_BUFFER_ALIGNMENT != 0) {
        return ((size / COPY_BUFFER_ALIGNMENT) + 1) * COPY_BUFFER_ALIGNMENT;
    }
    return size;
}


ImmediateStagingBuffer::ImmediateStagingBuffer()
    : _desc()
    , _mappedData(nullptr)
    , _finalized(false)
    , _bytesWritten(0)
    , _created(false) {
}

ImmediateStagingBuffer::~ImmediateStagingBuffer() {
    destroy();
}

bool ImmediateStagingBuffer::create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device) {
    if (_created) {
        LOG_ERROR("ImmediateStagingBuffer", "Buffer already created");
        return false;
    }
    
    if (!desc.isValid()) {
        LOG_ERROR("ImmediateStagingBuffer", "Invalid buffer description");
        return false;
    }
    
    if (!device) {
        LOG_ERROR("ImmediateStagingBuffer", "Device is null");
        return false;
    }
    
    const auto& resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        LOG_ERROR("ImmediateStagingBuffer", "Failed to get resource factory from device");
        return false;
    }
    
    _desc = desc;
    
    // Force immediate mapping configuration
    BufferDesc stagingDesc = desc;
    stagingDesc.mappedAtCreation = true;
    stagingDesc.usage |= BufferUsage::CopySrc;
    
    if (stagingDesc.memoryLocation == MemoryLocation::Auto) {
        stagingDesc.memoryLocation = MemoryLocation::HostVisible;
    }
    
    _buffer = resourceFactory->createMappableBuffer(stagingDesc);
    if (!_buffer) {
        LOG_ERROR("ImmediateStagingBuffer", "Failed to create underlying buffer");
        return false;
    }
    
    _mappedData = _buffer->getMappedData();
    if (!_mappedData) {
        LOG_ERROR("ImmediateStagingBuffer", "Failed to get mapped data");
        _buffer.reset();
        return false;
    }
    
    _created = true;
    _finalized = false;
    _bytesWritten = 0;
    
    std::stringstream ss;
    ss << "Created staging buffer '" << _desc.debugName << "' size=" << _desc.size << " mapped=true";
    LOG_DEBUG("ImmediateStagingBuffer", ss.str().c_str());
    
    return true;
}

void ImmediateStagingBuffer::destroy() {

    LOG_DEBUG("ImmediateStagingBuffer", "about to destroy immediate staging buffer");
    if (!_created) {
        return;
    }
    
    if (!_finalized && _mappedData) {
        // Only unmap if we still have mapped data (not already finalized)
        std::stringstream ss;
        ss << "Buffer '" << _desc.debugName << "' destroyed without being finalized";
        LOG_WARNING("ImmediateStagingBuffer", ss.str().c_str());
        
        // Check if buffer is actually mapped before unmapping
        if (_buffer && _buffer->isMapped()) {
            _buffer->unmap();
        }
    }
    
    _buffer.reset();
    _mappedData = nullptr;
    _finalized = false;
    _bytesWritten = 0;
    _created = false;
    _desc = BufferDesc();
    
    LOG_DEBUG("ImmediateStagingBuffer", "Destroyed staging buffer");
}

ImmediateStagingBuffer::ImmediateStagingBuffer(ImmediateStagingBuffer&& other) noexcept
    : _buffer(std::move(other._buffer))
    , _desc(std::move(other._desc))
    , _mappedData(other._mappedData)
    , _finalized(other._finalized)
    , _bytesWritten(other._bytesWritten)
    , _created(other._created) {
    other._mappedData = nullptr;
    other._finalized = false;
    other._bytesWritten = 0;
    other._created = false;
}

ImmediateStagingBuffer& ImmediateStagingBuffer::operator=(ImmediateStagingBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        
        _buffer = std::move(other._buffer);
        _desc = std::move(other._desc);
        _mappedData = other._mappedData;
        _finalized = other._finalized;
        _bytesWritten = other._bytesWritten;
        _created = other._created;
        
        other._mappedData = nullptr;
        other._finalized = false;
        other._bytesWritten = 0;
        other._created = false;
    }
    return *this;
}

uint64_t ImmediateStagingBuffer::writeBytes(const void* data, uint64_t size, uint64_t offset) {
    if (!_created || _finalized || !_mappedData || !data) {
        return 0;
    }
    
    if (offset + size > _desc.size) {
        std::stringstream ss;
        ss << "Write would exceed buffer size (offset=" << offset << ", size=" << size << ", buffer=" << _desc.size << ")";
        LOG_ERROR("ImmediateStagingBuffer", ss.str().c_str());
        return 0;
    }
    
    std::memcpy(static_cast<char*>(_mappedData) + offset, data, size);
    _bytesWritten = std::max(_bytesWritten, offset + size);
    return size;
}

void ImmediateStagingBuffer::finalize() {
    if (!_created || _finalized || !_buffer) {
        return;
    }
    
    _buffer->unmap();
    _mappedData = nullptr;
    _finalized = true;
    
    std::stringstream ss;
    ss << "Finalized buffer '" << _desc.debugName << "' with " << _bytesWritten << " bytes written";
    LOG_DEBUG("ImmediateStagingBuffer", ss.str().c_str());
}

bool ImmediateStagingBuffer::isFinalized() const {
    return _finalized;
}

uint64_t ImmediateStagingBuffer::getBytesWritten() const {
    return _bytesWritten;
}

uint64_t ImmediateStagingBuffer::getSize() const {
    return _created && _buffer ? _buffer->getSize() : 0;
}

BufferUsage ImmediateStagingBuffer::getUsage() const {
    return _created && _buffer ? _buffer->getUsage() : BufferUsage::None;
}

const std::string& ImmediateStagingBuffer::getDebugName() const {
    static const std::string empty;
    return _created ? _desc.debugName : empty;
}

NativeBufferHandle ImmediateStagingBuffer::getNativeHandle() const {
    return _created && _buffer ? _buffer->getNativeHandle() : NativeBufferHandle{};
}

bool ImmediateStagingBuffer::isValid() const {
    return _created && _buffer && _buffer->isValid();
}

BufferState ImmediateStagingBuffer::getState() const {
    if (!_created || !_buffer) return BufferState::Uninitialized;
    if (!_finalized) return BufferState::Mapped;
    return BufferState::Ready;
}

MemoryLocation ImmediateStagingBuffer::getMemoryLocation() const {
    return _created && _buffer ? _buffer->getMemoryLocation() : MemoryLocation::Auto;
}

AccessPattern ImmediateStagingBuffer::getAccessPattern() const {
    return AccessPattern::Static;
}

void* ImmediateStagingBuffer::getMappedData() {
    return _mappedData;
}

const void* ImmediateStagingBuffer::getMappedData() const {
    return _mappedData;
}

std::future<MappedData> ImmediateStagingBuffer::mapAsync(MapMode mode, const BufferMapRange& range) {
    std::promise<MappedData> promise;
    
    if (!_created || _finalized) {
        promise.set_value(MappedData{nullptr, 0, nullptr});
    } else if (_mappedData) {
        promise.set_value(MappedData{_mappedData, _desc.size, nullptr});
    } else {
        promise.set_value(MappedData{nullptr, 0, nullptr});
    }
    
    return promise.get_future();
}

void ImmediateStagingBuffer::unmap() {
    if (_created && !_finalized) {
        finalize();
    }
}

bool ImmediateStagingBuffer::isMapped() const {
    return _created && !_finalized && _mappedData != nullptr;
}

bool ImmediateStagingBuffer::isMapPending() const {
    return false;
}

void ImmediateStagingBuffer::flushMappedRange(uint64_t offset, uint64_t size) {
    // No-op for immediate buffers
}

void ImmediateStagingBuffer::invalidateMappedRange(uint64_t offset, uint64_t size) {
    // No-op for immediate buffers
}

} // namespace pers
