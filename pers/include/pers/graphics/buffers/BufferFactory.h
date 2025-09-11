#pragma once

#include "pers/graphics/buffers/IBufferFactory.h"
#include <webgpu/webgpu.h>

namespace pers {

// Forward declarations
class DeviceBuffer;
class ImmediateStagingBuffer;
class DeferredStagingBuffer;
class DynamicBuffer;

/**
 * Factory for creating buffer implementations
 * Currently uses WebGPU backend internally
 */
class BufferFactory final : public IBufferFactory, public std::enable_shared_from_this<BufferFactory> {
public:
    explicit BufferFactory(WGPUDevice device);
    virtual ~BufferFactory();
    
    BufferFactory(const BufferFactory&) = delete;
    BufferFactory& operator=(const BufferFactory&) = delete;
    
    // IBufferFactory interface
    virtual std::unique_ptr<IBuffer> createBuffer(const BufferDesc& desc) override;
    virtual std::unique_ptr<IMappableBuffer> createMappableBuffer(const BufferDesc& desc) override;
    virtual std::unique_ptr<IBuffer> createBufferWithSyncWrite(
        const BufferDesc& desc,
        const void* initialData,
        size_t dataSize) override;
    virtual bool isSupported(const BufferDesc& desc) const override;
    virtual uint64_t getMaxBufferSize() const override;
    virtual uint64_t getAlignment(BufferUsage usage) const override;
    
    // Additional factory methods for application-level buffers
    std::shared_ptr<DeviceBuffer> createDeviceBuffer(const BufferDesc& desc);
    std::shared_ptr<ImmediateStagingBuffer> createImmediateStagingBuffer(const BufferDesc& desc);
    std::shared_ptr<DeferredStagingBuffer> createDeferredStagingBuffer(const BufferDesc& desc);
    
    // Additional factory method
    std::shared_ptr<DynamicBuffer> createDynamicBuffer(const BufferDesc& desc, uint32_t frameCount = 3);
    
private:
    WGPUDevice _device;
};

} // namespace pers