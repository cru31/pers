#pragma once

#include "pers/graphics/buffers/IBuffer.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <string>

namespace pers {

/**
 * WebGPU implementation of IBuffer
 * Basic GPU buffer without CPU mapping capability
 */
class WebGPUBuffer : public IBuffer {
public:
    WebGPUBuffer(WGPUDevice device, const BufferDesc& desc);
    virtual ~WebGPUBuffer();
    
    // Delete copy operations
    WebGPUBuffer(const WebGPUBuffer&) = delete;
    WebGPUBuffer& operator=(const WebGPUBuffer&) = delete;
    
    // Move operations
    WebGPUBuffer(WebGPUBuffer&& other) noexcept;
    WebGPUBuffer& operator=(WebGPUBuffer&& other) noexcept;
    
    // IBuffer interface
    virtual uint64_t getSize() const override;
    virtual BufferUsage getUsage() const override;
    virtual const std::string& getDebugName() const override;
    virtual NativeBufferHandle getNativeHandle() const override;
    virtual bool isValid() const override;
    virtual BufferState getState() const override;
    virtual MemoryLocation getMemoryLocation() const override;
    virtual AccessPattern getAccessPattern() const override;
    
    // WebGPU-specific methods for internal use
    void* getMappedDataAtCreation();
    void unmapAtCreation();
    
private:
    WGPUDevice _device;
    WGPUBuffer _buffer;
    BufferDesc _desc;
    void* _mappedData;  // Only valid if mappedAtCreation is true
};

} // namespace pers
