#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoOrDie.h"

namespace pers {

WebGPURenderPassEncoder::WebGPURenderPassEncoder(WGPURenderPassEncoder encoder)
    : _encoder(encoder) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Created with null encoder handle", PERS_SOURCE_LOC);
    }
}

WebGPURenderPassEncoder::~WebGPURenderPassEncoder() {
    if (_encoder) {
        if (!_ended) {
            Logger::Instance().Log(LogLevel::Warning, "WebGPURenderPassEncoder", 
                                  "Render pass encoder destroyed without calling end()", PERS_SOURCE_LOC);
            end();
        }
        wgpuRenderPassEncoderRelease(_encoder);
        _encoder = nullptr;
    }
}

void WebGPURenderPassEncoder::setPipeline(const std::shared_ptr<IRenderPipeline>& pipeline) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set pipeline with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set pipeline on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    TodoOrDie::Log("WebGPURenderPassEncoder::setPipeline", 
                   "Cast pipeline to WebGPURenderPipeline and call wgpuRenderPassEncoderSetPipeline", 
                   PERS_SOURCE_LOC);
}

void WebGPURenderPassEncoder::setBindGroup(uint32_t index, const std::shared_ptr<IBindGroup>& bindGroup) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set bind group with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set bind group on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    TodoOrDie::Log("WebGPURenderPassEncoder::setBindGroup", 
                   "Cast bindGroup to WebGPUBindGroup and call wgpuRenderPassEncoderSetBindGroup", 
                   PERS_SOURCE_LOC);
}

void WebGPURenderPassEncoder::setVertexBuffer(uint32_t slot, const std::shared_ptr<IBuffer>& buffer, 
                                             uint64_t offset, uint64_t size) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set vertex buffer with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set vertex buffer on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    TodoOrDie::Log("WebGPURenderPassEncoder::setVertexBuffer", 
                   "Cast buffer to WebGPUBuffer and call wgpuRenderPassEncoderSetVertexBuffer", 
                   PERS_SOURCE_LOC);
}

void WebGPURenderPassEncoder::setIndexBuffer(const std::shared_ptr<IBuffer>& buffer, 
                                            IndexFormat indexFormat,
                                            uint64_t offset, uint64_t size) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set index buffer with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot set index buffer on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    TodoOrDie::Log("WebGPURenderPassEncoder::setIndexBuffer", 
                   "Cast buffer to WebGPUBuffer and call wgpuRenderPassEncoderSetIndexBuffer", 
                   PERS_SOURCE_LOC);
}

void WebGPURenderPassEncoder::draw(uint32_t vertexCount, uint32_t instanceCount,
                                  uint32_t firstVertex, uint32_t firstInstance) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot draw with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot draw on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    // Call WebGPU draw
    wgpuRenderPassEncoderDraw(_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPURenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                         uint32_t firstIndex, int32_t baseVertex,
                                         uint32_t firstInstance) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot draw indexed with null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot draw indexed on ended render pass", PERS_SOURCE_LOC);
        return;
    }
    
    // Call WebGPU draw indexed
    wgpuRenderPassEncoderDrawIndexed(_encoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void WebGPURenderPassEncoder::end() {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPassEncoder", 
                              "Cannot end null encoder", PERS_SOURCE_LOC);
        return;
    }
    
    if (_ended) {
        Logger::Instance().Log(LogLevel::Warning, "WebGPURenderPassEncoder", 
                              "Render pass already ended", PERS_SOURCE_LOC);
        return;
    }
    
    wgpuRenderPassEncoderEnd(_encoder);
    _ended = true;
}

NativeRenderPassEncoderHandle WebGPURenderPassEncoder::getNativeRenderPassEncoderHandle() const {
    return NativeRenderPassEncoderHandle::fromBackend(_encoder);
}

} // namespace pers