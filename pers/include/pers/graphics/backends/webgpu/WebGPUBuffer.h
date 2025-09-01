#pragma once

#include "pers/graphics/IBuffer.h"
#include <memory>

namespace pers {
namespace webgpu {

class WebGPUBuffer final : public IBuffer {
public:
    WebGPUBuffer(const BufferDesc& desc, void* device);
    ~WebGPUBuffer() override;
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    void* map(uint64_t offset = 0, uint64_t size = 0) override;
    void unmap() override;
    NativeBufferHandle getNativeBufferHandle() const override;
    
    // WebGPU specific - internal use only
    void* getNativeHandle() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace webgpu
} // namespace pers