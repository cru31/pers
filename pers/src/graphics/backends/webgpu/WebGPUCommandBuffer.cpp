#include "pers/graphics/backends/webgpu/WebGPUCommandBuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUCommandBuffer::WebGPUCommandBuffer(WGPUCommandBuffer commandBuffer)
    : _commandBuffer(commandBuffer) {
    if (!_commandBuffer) {
        LOG_ERROR("WebGPUCommandBuffer", 
                              "Created with null command buffer handle");
    }
}

WebGPUCommandBuffer::~WebGPUCommandBuffer() {
    if (_commandBuffer) {
        wgpuCommandBufferRelease(_commandBuffer);
        _commandBuffer = nullptr;
    }
}

NativeCommandBufferHandle WebGPUCommandBuffer::getNativeCommandBufferHandle() const {
    return NativeCommandBufferHandle::fromBackend(_commandBuffer);
}

} // namespace pers