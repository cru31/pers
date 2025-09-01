#pragma once

#include "pers/graphics/ICommandBuffer.h"
#include <webgpu/webgpu.h>

namespace pers {

/**
 * @brief WebGPU implementation of ICommandBuffer
 */
class WebGPUCommandBuffer : public ICommandBuffer {
public:
    /**
     * @brief Constructor
     * @param commandBuffer WebGPU command buffer handle
     */
    explicit WebGPUCommandBuffer(WGPUCommandBuffer commandBuffer);
    ~WebGPUCommandBuffer() override;
    
    // ICommandBuffer interface implementation
    NativeCommandBufferHandle getNativeCommandBufferHandle() const override;
    
private:
    WGPUCommandBuffer _commandBuffer = nullptr;
};

} // namespace pers