#include "pers/graphics/buffers/INativeBuffer.h"
#include "pers/graphics/buffers/ImmediateDeviceBuffer.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/utils/Logger.h"
#include <cstring>

namespace pers {

ImmediateDeviceBuffer::ImmediateDeviceBuffer(
    const std::shared_ptr<IResourceFactory>& resourceFactory,
    uint64_t size,
    BufferUsage usage,
    const void* initialData,
    size_t dataSize,
    const std::string& debugName)
    : _size(size)
    , _usage(usage)
    , _debugName(debugName)
    , _initialized(false) {
    
    if (!resourceFactory) {
        LOG_ERROR("ImmediateDeviceBuffer", "Resource factory is null");
        return;
    }
    
    if (!initialData || dataSize == 0) {
        LOG_ERROR("ImmediateDeviceBuffer", "Invalid initial data or size");
        return;
    }
    
    if (dataSize > size) {
        LOG_ERROR("ImmediateDeviceBuffer", "Data size exceeds buffer size");
        return;
    }

    // Create BufferDesc with internal settings for mappedAtCreation
    BufferDesc desc;
    desc.size = size;
    desc.usage = usage;
    desc.debugName = debugName;
    // Note: createInitializableDeviceBuffer will internally handle mappedAtCreation
    
    // Use createInitializableDeviceBuffer which handles mappedAtCreation internally
    _buffer = resourceFactory->createInitializableDeviceBuffer(desc, initialData, dataSize);
    
    if (!_buffer || !_buffer->isValid()) {
        LOG_ERROR("ImmediateDeviceBuffer", "Failed to create buffer with initial data");
        return;
    }
    
    _initialized = true;
    LOG_DEBUG("ImmediateDeviceBuffer", "Created buffer with immediate data, size: " + std::to_string(dataSize));
}

ImmediateDeviceBuffer::~ImmediateDeviceBuffer() = default;

uint64_t ImmediateDeviceBuffer::getSize() const {
    return _buffer ? _buffer->getSize() : _size;
}

BufferUsage ImmediateDeviceBuffer::getUsage() const {
    return _buffer ? _buffer->getUsage() : _usage;
}

const std::string& ImmediateDeviceBuffer::getDebugName() const {
    return _buffer ? _buffer->getDebugName() : _debugName;
}

NativeBufferHandle ImmediateDeviceBuffer::getNativeHandle() const {
    return _buffer ? _buffer->getNativeHandle() : NativeBufferHandle{};
}

bool ImmediateDeviceBuffer::isValid() const {
    return _initialized && _buffer && _buffer->isValid();
}

BufferState ImmediateDeviceBuffer::getState() const {
    return _buffer ? _buffer->getState() : BufferState::Uninitialized;
}

MemoryLocation ImmediateDeviceBuffer::getMemoryLocation() const {
    return _buffer ? _buffer->getMemoryLocation() : MemoryLocation::Auto;
}

AccessPattern ImmediateDeviceBuffer::getAccessPattern() const {
    return _buffer ? _buffer->getAccessPattern() : AccessPattern::Static;
}

} // namespace pers
