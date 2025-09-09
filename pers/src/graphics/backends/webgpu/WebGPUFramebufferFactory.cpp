#include "pers/graphics/backends/webgpu/WebGPUFramebufferFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUSurfaceFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUOffscreenFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUDepthOnlyFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/utils/Logger.h"

namespace pers {

std::shared_ptr<ISurfaceFramebuffer> WebGPUFramebufferFactory::createSurfaceFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    WGPUSurface surface,
    uint32_t width,
    uint32_t height,
    TextureFormat format) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for surface framebuffer");
        return nullptr;
    }
    
    if (!surface) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid surface provided");
        return nullptr;
    }
    
    return std::make_shared<WebGPUSurfaceFramebuffer>(device, surface, width, height, format);
}

std::shared_ptr<IResizableFramebuffer> WebGPUFramebufferFactory::createOffscreenFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    const OffscreenFramebufferConfig& config) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for offscreen framebuffer");
        return nullptr;
    }
    
    return std::make_shared<WebGPUOffscreenFramebuffer>(device, config);
}

std::shared_ptr<IResizableFramebuffer> WebGPUFramebufferFactory::createDepthOnlyFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    uint32_t width,
    uint32_t height,
    TextureFormat format,
    uint32_t sampleCount,
    TextureUsage usage) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for depth-only framebuffer");
        return nullptr;
    }
    
    return std::make_shared<WebGPUDepthOnlyFramebuffer>(
        device, width, height, format, sampleCount, usage);
}

std::shared_ptr<IResizableFramebuffer> WebGPUFramebufferFactory::createMSAAFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    uint32_t width,
    uint32_t height,
    TextureFormat colorFormat,
    TextureFormat depthFormat,
    uint32_t sampleCount) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for MSAA framebuffer");
        return nullptr;
    }
    
    if (sampleCount <= 1) {
        LOG_WARNING("WebGPUFramebufferFactory", 
            "MSAA framebuffer created with sampleCount=" + std::to_string(sampleCount) + 
            ", should be > 1 for MSAA");
    }
    
    // Configure MSAA framebuffer
    OffscreenFramebufferConfig config;
    config.width = width;
    config.height = height;
    config.sampleCount = sampleCount;
    config.colorFormats.push_back(colorFormat);
    config.depthFormat = depthFormat;
    
    // MSAA textures typically don't need TextureBinding since they can't be sampled directly
    // They are only used as render attachments and resolved to single-sample textures
    config.colorUsage = TextureUsage::RenderAttachment;
    config.depthUsage = TextureUsage::RenderAttachment;
    
    return std::make_shared<WebGPUOffscreenFramebuffer>(device, config);
}

std::shared_ptr<IResizableFramebuffer> WebGPUFramebufferFactory::createShadowMapFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    uint32_t size,
    TextureFormat format) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for shadow map framebuffer");
        return nullptr;
    }
    
    // Shadow maps need to be sampled in shaders
    TextureUsage usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    
    return std::make_shared<WebGPUDepthOnlyFramebuffer>(
        device, size, size, format, 1, usage);
}

std::shared_ptr<IResizableFramebuffer> WebGPUFramebufferFactory::createMRTFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    uint32_t width,
    uint32_t height,
    const TextureFormat* colorFormats,
    uint32_t colorCount,
    TextureFormat depthFormat) {
    
    if (!device) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid device provided for MRT framebuffer");
        return nullptr;
    }
    
    if (!colorFormats || colorCount == 0) {
        LOG_ERROR("WebGPUFramebufferFactory", "Invalid color formats for MRT framebuffer");
        return nullptr;
    }
    
    if (colorCount > 8) {
        LOG_ERROR("WebGPUFramebufferFactory", 
            "Too many color attachments for MRT: " + std::to_string(colorCount) + 
            ", maximum is 8");
        return nullptr;
    }
    
    // Configure MRT framebuffer
    OffscreenFramebufferConfig config;
    config.width = width;
    config.height = height;
    config.sampleCount = 1;  // MRT typically doesn't use MSAA
    config.depthFormat = depthFormat;
    
    // Add all color formats
    for (uint32_t i = 0; i < colorCount; ++i) {
        config.colorFormats.push_back(colorFormats[i]);
    }
    
    // MRT textures need to be sampled for deferred shading
    config.colorUsage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    config.depthUsage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    
    LOG_DEBUG("WebGPUFramebufferFactory", 
        "Creating MRT framebuffer with " + std::to_string(colorCount) + " color attachments");
    
    return std::make_shared<WebGPUOffscreenFramebuffer>(device, config);
}

} // namespace pers