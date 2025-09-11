#pragma once

#include "pers/graphics/IBuffer.h"
#include <webgpu/webgpu.h>

namespace pers {

class WebGPUBuffer final : public IBuffer {
public:
    WebGPUBuffer(const BufferDesc& desc, WGPUDevice device);
    ~WebGPUBuffer() override;
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    void* map(uint64_t offset = 0, uint64_t size = 0) override;
    void unmap() override;
    NativeBufferHandle getNativeHandle() const override;
    
private:
    // Helper method to get mapped range
    void* getMappedRange(uint64_t offset, uint64_t size);
    
    uint64_t _size;
    BufferUsage _usage;
    std::string _debugName;
    WGPUBuffer _buffer = nullptr;
    void* _mappedData = nullptr;
};

} // namespace pers