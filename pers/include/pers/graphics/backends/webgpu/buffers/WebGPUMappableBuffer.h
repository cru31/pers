#pragma once

#include "pers/graphics/buffers/IMappableBuffer.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUBuffer.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <atomic>
#include <future>

namespace pers {

/**
 * WebGPU implementation of IMappableBuffer
 * GPU buffer with CPU mapping capability via mapAsync
 */
class WebGPUMappableBuffer : public IMappableBuffer {
public:
    WebGPUMappableBuffer(WGPUDevice device, const BufferDesc& desc);
    virtual ~WebGPUMappableBuffer();
    
    // Delete copy operations
    WebGPUMappableBuffer(const WebGPUMappableBuffer&) = delete;
    WebGPUMappableBuffer& operator=(const WebGPUMappableBuffer&) = delete;
    
    // Move operations
    WebGPUMappableBuffer(WebGPUMappableBuffer&& other) noexcept;
    WebGPUMappableBuffer& operator=(WebGPUMappableBuffer&& other) noexcept;
    
    // IMappableBuffer interface
    virtual void* getMappedData() override;
    virtual const void* getMappedData() const override;
    virtual std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, 
                                            const BufferMapRange& range = {0, BufferMapRange::WHOLE_BUFFER}) override;
    virtual void unmap() override;
    virtual bool isMapped() const override;
    virtual bool isMapPending() const override;
    virtual void flushMappedRange(uint64_t offset, uint64_t size) override;
    virtual void invalidateMappedRange(uint64_t offset, uint64_t size) override;
    
    // IBuffer interface
    virtual uint64_t getSize() const override;
    virtual BufferUsage getUsage() const override;
    virtual const std::string& getDebugName() const override;
    virtual NativeBufferHandle getNativeHandle() const override;
    virtual bool isValid() const override;
    virtual BufferState getState() const override;
    virtual MemoryLocation getMemoryLocation() const override;
    virtual AccessPattern getAccessPattern() const override;
    
    // Friend function for map callback (defined in cpp file)
    
public:
    // These need to be accessible by callback
    void* _mappedData;
    std::atomic<bool> _isMapped;
    struct {
        uint64_t offset;
        uint64_t size;
    } _mappedRange;
    
private:
    std::unique_ptr<WebGPUBuffer> _impl;
};

} // namespace pers
