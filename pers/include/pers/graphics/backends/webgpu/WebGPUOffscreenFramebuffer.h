#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/GraphicsTypes.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <vector>

namespace pers {

class WebGPULogicalDevice;
class WebGPUTextureView;

/**
 * @brief Configuration for creating an offscreen framebuffer
 * 
 * All parameters externally configurable, no hardcoding.
 */
struct OffscreenFramebufferConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t sampleCount = 1;  // 1 for no MSAA, 2/4/8 for MSAA
    
    // Color attachments (up to 8 for MRT)
    std::vector<TextureFormat> colorFormats;
    
    // Depth/stencil attachment (optional)
    TextureFormat depthFormat = TextureFormat::Undefined;
    
    // Texture usage flags for color attachments
    TextureUsage colorUsage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    
    // Texture usage flags for depth attachment
    TextureUsage depthUsage = TextureUsage::RenderAttachment;
};

/**
 * @brief WebGPU implementation of offscreen framebuffer
 * 
 * Creates and manages textures for offscreen rendering.
 * Supports MSAA, MRT, and optional depth/stencil attachments.
 */
class WebGPUOffscreenFramebuffer : public IResizableFramebuffer {
public:
    /**
     * @brief Construct an offscreen framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param config Configuration for the framebuffer
     */
    WebGPUOffscreenFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        const OffscreenFramebufferConfig& config);
    
    ~WebGPUOffscreenFramebuffer() override;
    
    // IFramebuffer interface
    std::shared_ptr<ITextureView> getColorAttachment(uint32_t index = 0) const override;
    std::shared_ptr<ITextureView> getDepthStencilAttachment() const override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    uint32_t getSampleCount() const override;
    TextureFormat getColorFormat(uint32_t index = 0) const override;
    TextureFormat getDepthFormat() const override;
    uint32_t getColorAttachmentCount() const override;
    bool hasDepthStencilAttachment() const override;
    
    // IResizableFramebuffer interface
    bool resize(uint32_t width, uint32_t height) override;
    
    /**
     * @brief Get the native texture for a color attachment
     * @param index Color attachment index
     * @return Native texture handle, or nullptr if index invalid
     */
    WGPUTexture getColorTexture(uint32_t index = 0) const;
    
    /**
     * @brief Get the native texture for depth/stencil attachment
     * @return Native texture handle, or nullptr if no depth attachment
     */
    WGPUTexture getDepthTexture() const;
    
private:
    void createTextures();
    void releaseTextures();
    bool isDepthStencilFormat(TextureFormat format) const;
    
    // Device (weak_ptr to avoid circular reference)
    std::weak_ptr<WebGPULogicalDevice> _device;
    
    // Configuration
    OffscreenFramebufferConfig _config;
    
    // Color attachments (textures and views)
    std::vector<WGPUTexture> _colorTextures;
    std::vector<std::shared_ptr<WebGPUTextureView>> _colorViews;
    
    // Depth/stencil attachment (optional)
    WGPUTexture _depthTexture = nullptr;
    std::shared_ptr<WebGPUTextureView> _depthView;
};

} // namespace pers