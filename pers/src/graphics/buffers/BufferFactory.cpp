#include "pers/graphics/buffers/BufferFactory.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUMappableBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/buffers/DeferredStagingBuffer.h"
#include "pers/graphics/buffers/DynamicBuffer.h"
#include "pers/utils/Logger.h"
#include <cstring>  // for memcpy

namespace pers {

BufferFactory::BufferFactory(WGPUDevice device)
    : _device(device) {
    
    if (!_device) {
        LOG_ERROR("BufferFactory", "Device is null");
    }
}

BufferFactory::~BufferFactory() = default;

std::unique_ptr<IBuffer> BufferFactory::createBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create buffer: device is null");
        return nullptr;
    }
    
    // Determine buffer type based on usage and mapping requirements
    bool needsMapping = hasFlag(desc.usage, BufferUsage::MapRead) || 
                       hasFlag(desc.usage, BufferUsage::MapWrite) ||
                       desc.mappedAtCreation;
    
    if (needsMapping) {
        return std::make_unique<WebGPUMappableBuffer>(_device, desc);
    } else {
        return std::make_unique<WebGPUBuffer>(_device, desc);
    }
}

std::unique_ptr<IMappableBuffer> BufferFactory::createMappableBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create mappable buffer: device is null");
        return nullptr;
    }
    
    BufferDesc mappableDesc = desc;
    mappableDesc.usage |= BufferUsage::MapRead | BufferUsage::MapWrite;
    
    return std::make_unique<WebGPUMappableBuffer>(_device, mappableDesc);
}

std::unique_ptr<IBuffer> BufferFactory::createBufferWithSyncWrite(
    const BufferDesc& desc,
    const void* initialData,
    size_t dataSize) {
    
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create buffer with sync write: device is null");
        return nullptr;
    }
    
    if (!initialData || dataSize == 0) {
        LOG_ERROR("BufferFactory", "Invalid initial data or size");
        return nullptr;
    }
    
    if (dataSize > desc.size) {
        LOG_ERROR("BufferFactory", "Data size exceeds buffer size");
        return nullptr;
    }
    
    // Create buffer with mappedAtCreation for synchronous data write
    BufferDesc syncDesc = desc;
    syncDesc.mappedAtCreation = true;
    
    auto buffer = std::make_unique<WebGPUBuffer>(_device, syncDesc);
    if (!buffer || !buffer->isValid()) {
        LOG_ERROR("BufferFactory", "Failed to create buffer");
        return nullptr;
    }
    
    // Write data synchronously using mapped memory
    void* mappedData = buffer->getMappedDataAtCreation();
    if (mappedData) {
        memcpy(mappedData, initialData, dataSize);
        buffer->unmapAtCreation();
    } else {
        LOG_ERROR("BufferFactory", "Failed to get mapped data");
        return nullptr;
    }
    
    return buffer;
}

std::shared_ptr<DeviceBuffer> BufferFactory::createDeviceBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create device buffer: device is null");
        return nullptr;
    }
    
    // DeviceBuffer will use this factory to create its internal buffer
    return std::make_shared<DeviceBuffer>(desc, shared_from_this());
}

std::shared_ptr<ImmediateStagingBuffer> BufferFactory::createImmediateStagingBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create immediate staging buffer: device is null");
        return nullptr;
    }
    
    // ImmediateStagingBuffer will use this factory to create its internal buffer
    return std::make_shared<ImmediateStagingBuffer>(desc, shared_from_this());
}

std::shared_ptr<DeferredStagingBuffer> BufferFactory::createDeferredStagingBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create deferred staging buffer: device is null");
        return nullptr;
    }
    
    // DeferredStagingBuffer will use this factory to create its internal buffer
    return std::make_shared<DeferredStagingBuffer>(desc, shared_from_this());
}

std::shared_ptr<DynamicBuffer> BufferFactory::createDynamicBuffer(const BufferDesc& desc, uint32_t frameCount) {
    if (!_device) {
        LOG_ERROR("BufferFactory", "Cannot create dynamic buffer: device is null");
        return nullptr;
    }
    
    // Create DynamicBuffer with this factory
    return std::make_shared<DynamicBuffer>(desc, shared_from_this(), frameCount);
}

bool BufferFactory::isSupported(const BufferDesc& desc) const {
    if (!_device) {
        return false;
    }
    
    // Basic validation
    if (!desc.isValid()) {
        return false;
    }
    
    // Check size limits (WebGPU typically supports up to 2GB)
    if (desc.size > getMaxBufferSize()) {
        return false;
    }
    
    return true;
}

uint64_t BufferFactory::getMaxBufferSize() const {
    // WebGPU typical limit
    return 2147483648ULL; // 2GB
}

uint64_t BufferFactory::getAlignment(BufferUsage usage) const {
    // WebGPU alignment requirements
    if (hasFlag(usage, BufferUsage::Uniform)) {
        return 256; // Uniform buffer alignment
    }
    if (hasFlag(usage, BufferUsage::Storage)) {
        return 256; // Storage buffer alignment
    }
    return 4; // Default alignment
}

} // namespace pers