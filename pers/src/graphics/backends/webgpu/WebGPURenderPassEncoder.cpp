#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUBuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPURenderPassEncoder::WebGPURenderPassEncoder(WGPURenderPassEncoder encoder)
    : _encoder(encoder) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Created with null encoder handle");
    }
}

WebGPURenderPassEncoder::~WebGPURenderPassEncoder() {
    if (_encoder) {
        if (!_ended) {
            LOG_WARNING("WebGPURenderPassEncoder", 
                                  "Render pass encoder destroyed without calling end()");
            end();
        }
        wgpuRenderPassEncoderRelease(_encoder);
        _encoder = nullptr;
    }
}

void WebGPURenderPassEncoder::setPipeline(const std::shared_ptr<IRenderPipeline>& pipeline) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set pipeline with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set pipeline on ended render pass");
        return;
    }
    
    // Cast to WebGPU implementation
    auto webgpuPipeline = std::dynamic_pointer_cast<WebGPURenderPipeline>(pipeline);
    if (!webgpuPipeline) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Invalid pipeline type - not a WebGPURenderPipeline");
        return;
    }
    
    // Set the pipeline
    wgpuRenderPassEncoderSetPipeline(_encoder, webgpuPipeline->getNativeHandle());
}

void WebGPURenderPassEncoder::setBindGroup(uint32_t index, const std::shared_ptr<IBindGroup>& bindGroup) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set bind group with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set bind group on ended render pass");
        return;
    }
    
    TODO_OR_DIE("WebGPURenderPassEncoder::setBindGroup", 
                   "Cast bindGroup to WebGPUBindGroup and call wgpuRenderPassEncoderSetBindGroup");
}

void WebGPURenderPassEncoder::setVertexBuffer(uint32_t slot, const std::shared_ptr<IBuffer>& buffer, 
                                             uint64_t offset, uint64_t size) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set vertex buffer with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set vertex buffer on ended render pass");
        return;
    }
    
    // Cast to WebGPU implementation
    auto webgpuBuffer = std::dynamic_pointer_cast<WebGPUBuffer>(buffer);
    if (!webgpuBuffer) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Invalid buffer type - not a WebGPUBuffer");
        return;
    }
    
    // If size is 0, use the entire buffer size from offset
    uint64_t bufferSize = size;
    if (bufferSize == 0) {
        bufferSize = webgpuBuffer->getSize() - offset;
    }
    
    // Set the vertex buffer
    WGPUBuffer wgpuBuffer = webgpuBuffer->getNativeHandle().as<WGPUBuffer>();
    wgpuRenderPassEncoderSetVertexBuffer(_encoder, slot, wgpuBuffer, offset, bufferSize);
}

void WebGPURenderPassEncoder::setIndexBuffer(const std::shared_ptr<IBuffer>& buffer, 
                                            IndexFormat indexFormat,
                                            uint64_t offset, uint64_t size) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set index buffer with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot set index buffer on ended render pass");
        return;
    }
    
    TODO_OR_DIE("WebGPURenderPassEncoder::setIndexBuffer", 
                   "Cast buffer to WebGPUBuffer and call wgpuRenderPassEncoderSetIndexBuffer");
}

void WebGPURenderPassEncoder::draw(uint32_t vertexCount, uint32_t instanceCount,
                                  uint32_t firstVertex, uint32_t firstInstance) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot draw with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot draw on ended render pass");
        return;
    }
    
    // Call WebGPU draw
    wgpuRenderPassEncoderDraw(_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPURenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                         uint32_t firstIndex, int32_t baseVertex,
                                         uint32_t firstInstance) {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot draw indexed with null encoder");
        return;
    }
    
    if (_ended) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot draw indexed on ended render pass");
        return;
    }
    
    // Call WebGPU draw indexed
    wgpuRenderPassEncoderDrawIndexed(_encoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void WebGPURenderPassEncoder::end() {
    if (!_encoder) {
        LOG_ERROR("WebGPURenderPassEncoder", 
                              "Cannot end null encoder");
        return;
    }
    
    if (_ended) {
        LOG_WARNING("WebGPURenderPassEncoder", 
                              "Render pass already ended");
        return;
    }
    
    wgpuRenderPassEncoderEnd(_encoder);
    _ended = true;
}

NativeRenderPassEncoderHandle WebGPURenderPassEncoder::getNativeRenderPassEncoderHandle() const {
    return NativeRenderPassEncoderHandle::fromBackend(_encoder);
}

} // namespace pers