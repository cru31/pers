#include "pers/graphics/backends/webgpu/WebGPUCommandEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUCommandBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUCommandEncoder::WebGPUCommandEncoder(WGPUCommandEncoder encoder)
    : _encoder(encoder) {
    if (!_encoder) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Created with null encoder handle");
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
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Cannot begin render pass with null encoder");
        return nullptr;
    }
    
    if (_finished) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Cannot begin render pass on finished encoder");
        return nullptr;
    }
    
    // Create render pass descriptor
    WGPURenderPassDescriptor renderPassDesc = {};
    if (!desc.label.empty()) {
        renderPassDesc.label = WGPUStringView{.data = desc.label.c_str(), .length = desc.label.length()};
    } else {
        renderPassDesc.label = WGPUStringView{.data = "Render Pass", .length = 11};
    }
    
    // Setup color attachments
    std::vector<WGPURenderPassColorAttachment> colorAttachments;
    for (const auto& attachment : desc.colorAttachments) {
        if (!attachment.view) {
            LOG_ERROR("WebGPUCommandEncoder", 
                                  "Color attachment has null view");
            return nullptr;
        }
        
        auto webgpuTextureView = std::dynamic_pointer_cast<WebGPUTextureView>(attachment.view);
        if (!webgpuTextureView) {
            LOG_ERROR("WebGPUCommandEncoder", 
                                  "Invalid color attachment type");
            return nullptr;
        }
        
        WGPURenderPassColorAttachment wgpuAttachment = {};
        wgpuAttachment.view = webgpuTextureView->getNativeTextureViewHandle().as<WGPUTextureView>();
        wgpuAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        
        // Convert LoadOp
        switch (attachment.loadOp) {
            case LoadOp::Clear:
                wgpuAttachment.loadOp = WGPULoadOp_Clear;
                wgpuAttachment.clearValue = WGPUColor{
                    attachment.clearColor.r,
                    attachment.clearColor.g,
                    attachment.clearColor.b,
                    attachment.clearColor.a
                };
                break;
            case LoadOp::Load:
                wgpuAttachment.loadOp = WGPULoadOp_Load;
                break;
            case LoadOp::Undefined:
            default:
                wgpuAttachment.loadOp = WGPULoadOp_Undefined;
                break;
        }
        
        // Convert StoreOp
        switch (attachment.storeOp) {
            case StoreOp::Store:
                wgpuAttachment.storeOp = WGPUStoreOp_Store;
                break;
            case StoreOp::Discard:
                wgpuAttachment.storeOp = WGPUStoreOp_Discard;
                break;
            default:
                wgpuAttachment.storeOp = WGPUStoreOp_Store;
                break;
        }
        
        colorAttachments.push_back(wgpuAttachment);
    }
    
    renderPassDesc.colorAttachmentCount = colorAttachments.size();
    renderPassDesc.colorAttachments = colorAttachments.empty() ? nullptr : colorAttachments.data();
    
    // Setup depth stencil attachment if provided
    WGPURenderPassDepthStencilAttachment wgpuDepthStencilAttachment = {};
    if (desc.depthStencilAttachment && desc.depthStencilAttachment->view) {
        auto webgpuTextureView = std::dynamic_pointer_cast<WebGPUTextureView>(desc.depthStencilAttachment->view);
        if (!webgpuTextureView) {
            LOG_ERROR("WebGPUCommandEncoder", 
                                  "Invalid depth stencil attachment type");
            return nullptr;
        }
        
        wgpuDepthStencilAttachment.view = webgpuTextureView->getNativeTextureViewHandle().as<WGPUTextureView>();
        
        // Convert depth LoadOp
        switch (desc.depthStencilAttachment->depthLoadOp) {
            case LoadOp::Clear:
                wgpuDepthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
                wgpuDepthStencilAttachment.depthClearValue = desc.depthStencilAttachment->depthClearValue;
                break;
            case LoadOp::Load:
                wgpuDepthStencilAttachment.depthLoadOp = WGPULoadOp_Load;
                break;
            case LoadOp::Undefined:
            default:
                wgpuDepthStencilAttachment.depthLoadOp = WGPULoadOp_Undefined;
                break;
        }
        
        // Convert depth StoreOp
        switch (desc.depthStencilAttachment->depthStoreOp) {
            case StoreOp::Store:
                wgpuDepthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
                break;
            case StoreOp::Discard:
                wgpuDepthStencilAttachment.depthStoreOp = WGPUStoreOp_Discard;
                break;
            default:
                wgpuDepthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
                break;
        }
        
        // Convert stencil LoadOp
        switch (desc.depthStencilAttachment->stencilLoadOp) {
            case LoadOp::Clear:
                wgpuDepthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
                wgpuDepthStencilAttachment.stencilClearValue = desc.depthStencilAttachment->stencilClearValue;
                break;
            case LoadOp::Load:
                wgpuDepthStencilAttachment.stencilLoadOp = WGPULoadOp_Load;
                break;
            case LoadOp::Undefined:
            default:
                wgpuDepthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
                break;
        }
        
        // Convert stencil StoreOp
        switch (desc.depthStencilAttachment->stencilStoreOp) {
            case StoreOp::Store:
                wgpuDepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
                break;
            case StoreOp::Discard:
                wgpuDepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Discard;
                break;
            default:
                wgpuDepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
                break;
        }
        
        wgpuDepthStencilAttachment.depthReadOnly = desc.depthStencilAttachment->depthReadOnly;
        wgpuDepthStencilAttachment.stencilReadOnly = desc.depthStencilAttachment->stencilReadOnly;
        
        renderPassDesc.depthStencilAttachment = &wgpuDepthStencilAttachment;
    } else {
        renderPassDesc.depthStencilAttachment = nullptr;
    }
    
    // Begin render pass
    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(_encoder, &renderPassDesc);
    if (!renderPassEncoder) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Failed to begin render pass");
        return nullptr;
    }
    
    return std::make_shared<WebGPURenderPassEncoder>(renderPassEncoder);
}

std::shared_ptr<ICommandBuffer> WebGPUCommandEncoder::finish() {
    if (!_encoder) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Cannot finish null encoder");
        return nullptr;
    }
    
    if (_finished) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Encoder already finished");
        return nullptr;
    }
    
    // Finish encoding
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.label = WGPUStringView{.data = "Command Buffer", .length = 14};
    
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(_encoder, &cmdBufferDesc);
    if (!commandBuffer) {
        LOG_ERROR("WebGPUCommandEncoder", 
                              "Failed to finish command encoder");
        return nullptr;
    }
    
    _finished = true;
    
    return std::make_shared<WebGPUCommandBuffer>(commandBuffer);
}

NativeEncoderHandle WebGPUCommandEncoder::getNativeEncoderHandle() const {
    return NativeEncoderHandle::fromBackend(_encoder);
}

} // namespace pers