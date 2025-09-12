#include "pers/graphics/buffers/ImmediateDeviceBuffer.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/utils/Logger.h"
#include <cstring>

namespace pers {

ImmediateDeviceBuffer::ImmediateDeviceBuffer(
    const std::shared_ptr<IResourceFactory>& resourceFactory,
    const BufferDesc& desc,
    const void* initialData,
    size_t dataSize)
    : _desc(desc)
    , _initialized(false) {
    
    if (!resourceFactory) {
        LOG_ERROR("ImmediateDeviceBuffer", "Resource factory is null");
        return;
    }
    
    if (!initialData || dataSize == 0) {
        LOG_ERROR("ImmediateDeviceBuffer", "Invalid initial data or size");
        return;
    }
    
    if (dataSize > desc.size) {
        LOG_ERROR("ImmediateDeviceBuffer", "Data size exceeds buffer size");
        return;
    }
    
    // Create buffer with mappedAtCreation for synchronous data write
    BufferDesc syncDesc = desc;
    syncDesc.mappedAtCreation = true;
    
    _buffer = resourceFactory->createBuffer(syncDesc);
    if (!_buffer || !_buffer->isValid()) {
        LOG_ERROR("ImmediateDeviceBuffer", "Failed to create underlying buffer");
        return;
    }
    
    // For WebGPU buffers created with mappedAtCreation, we need special handling
    // The buffer should provide a way to get the mapped memory
    // This is backend-specific, so we'll need to handle it through the resource factory
    
    // For now, we'll use the createInitializableDeviceBuffer method from IResourceFactory
    // Reset and recreate using the proper method
    _buffer.reset();
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
    return _buffer ? _buffer->getSize() : 0;
}

BufferUsage ImmediateDeviceBuffer::getUsage() const {
    return _buffer ? _buffer->getUsage() : BufferUsage::None;
}

const std::string& ImmediateDeviceBuffer::getDebugName() const {
    static const std::string empty;
    return _buffer ? _buffer->getDebugName() : empty;
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