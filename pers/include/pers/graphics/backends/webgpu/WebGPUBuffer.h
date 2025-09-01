#pragma once

#include "pers/graphics/IBuffer.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

class WebGPUBuffer final : public IBuffer {
public:
    WebGPUBuffer(const BufferDesc& desc, WGPUDevice device);
    ~WebGPUBuffer() override;
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    void* map(uint64_t offset = 0, uint64_t size = 0) override;
    void unmap() override;
    NativeBufferHandle getNativeBufferHandle() const override;
    
    // WebGPU specific - internal use only
    WGPUBuffer getNativeHandle() const;
    
private:
    uint64_t _size;
    BufferUsage _usage;
    std::string _debugName;
    WGPUBuffer _buffer = nullptr;
    void* _mappedData = nullptr;
};

} // namespace webgpu
} // namespace pers