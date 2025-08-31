#include "pers/graphics/backends/webgpu/WebGPUCommandBuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUCommandBuffer::WebGPUCommandBuffer(WGPUCommandBuffer commandBuffer)
    : _commandBuffer(commandBuffer) {
    if (!_commandBuffer) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandBuffer", 
                              "Created with null command buffer handle", PERS_SOURCE_LOC);
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