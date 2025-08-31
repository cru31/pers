#include "pers/graphics/backends/webgpu/WebGPUCommandEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUCommandBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUCommandEncoder::WebGPUCommandEncoder(WGPUCommandEncoder encoder)
    : _encoder(encoder) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Created with null encoder handle", PERS_SOURCE_LOC);
    }
}

WebGPUCommandEncoder::~WebGPUCommandEncoder() {
    if (_encoder) {
        wgpuCommandEncoderRelease(_encoder);
        _encoder = nullptr;
    }
}

std::shared_ptr<IRenderPassEncoder> WebGPUCommandEncoder::beginRenderPass(const RenderPassDesc& desc) {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Cannot begin render pass with null encoder", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    if (_finished) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Cannot begin render pass on finished encoder", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Create render pass descriptor
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = WGPUStringView{.data = "Render Pass", .length = 11};
    
    // Setup color attachment
    WGPURenderPassColorAttachment colorAttachment = {};
    if (desc.colorAttachment) {
        auto webgpuTextureView = std::dynamic_pointer_cast<WebGPUTextureView>(desc.colorAttachment);
        if (!webgpuTextureView) {
            Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                                  "Invalid color attachment type", PERS_SOURCE_LOC);
            return nullptr;
        }
        
        colorAttachment.view = webgpuTextureView->getNativeTextureViewHandle().as<WGPUTextureView>();
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        
        // Setup clear value if needed
        if (desc.clearColor) {
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.clearValue = WGPUColor{
                desc.clearColorValue[0],
                desc.clearColorValue[1], 
                desc.clearColorValue[2],
                desc.clearColorValue[3]
            };
        } else {
            colorAttachment.loadOp = WGPULoadOp_Load;
        }
        
        colorAttachment.storeOp = WGPUStoreOp_Store;
    }
    
    renderPassDesc.colorAttachmentCount = desc.colorAttachment ? 1 : 0;
    renderPassDesc.colorAttachments = desc.colorAttachment ? &colorAttachment : nullptr;
    
    // Setup depth stencil attachment if provided
    WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
    if (desc.depthStencilAttachment) {
        auto webgpuTextureView = std::dynamic_pointer_cast<WebGPUTextureView>(desc.depthStencilAttachment);
        if (!webgpuTextureView) {
            Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                                  "Invalid depth stencil attachment type", PERS_SOURCE_LOC);
            return nullptr;
        }
        
        depthStencilAttachment.view = webgpuTextureView->getNativeTextureViewHandle().as<WGPUTextureView>();
        
        // Setup depth clear
        if (desc.clearDepth) {
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.depthClearValue = desc.clearDepthValue;
        } else {
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Load;
        }
        depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
        
        // For now, we don't handle stencil
        depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
        depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
        depthStencilAttachment.stencilClearValue = 0;
        depthStencilAttachment.stencilReadOnly = false;
        depthStencilAttachment.depthReadOnly = false;
        
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }
    
    // Begin render pass
    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(_encoder, &renderPassDesc);
    if (!renderPassEncoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Failed to begin render pass", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    return std::make_shared<WebGPURenderPassEncoder>(renderPassEncoder);
}

std::shared_ptr<ICommandBuffer> WebGPUCommandEncoder::finish() {
    if (!_encoder) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Cannot finish null encoder", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    if (_finished) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Encoder already finished", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Finish encoding
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.label = WGPUStringView{.data = "Command Buffer", .length = 14};
    
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(_encoder, &cmdBufferDesc);
    if (!commandBuffer) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUCommandEncoder", 
                              "Failed to finish command encoder", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    _finished = true;
    
    return std::make_shared<WebGPUCommandBuffer>(commandBuffer);
}

NativeEncoderHandle WebGPUCommandEncoder::getNativeEncoderHandle() const {
    return NativeEncoderHandle::fromBackend(_encoder);
}

} // namespace pers