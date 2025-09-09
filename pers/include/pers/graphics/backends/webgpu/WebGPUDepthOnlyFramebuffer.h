#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/GraphicsTypes.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

class WebGPULogicalDevice;
class WebGPUTextureView;

/**
 * @brief WebGPU implementation of depth-only framebuffer
 * 
 * Creates and manages a depth/stencil texture without color attachments.
 * Useful for shadow mapping and depth pre-pass rendering.
 */
class WebGPUDepthOnlyFramebuffer : public IResizableFramebuffer {
public:
    /**
     * @brief Construct a depth-only framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param width Width of the depth texture
     * @param height Height of the depth texture
     * @param format Depth format (externally configurable, no hardcoding)
     * @param sampleCount Sample count for MSAA (1 for no MSAA)
     * @param usage Texture usage flags (externally configurable)
     */
    WebGPUDepthOnlyFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::Depth24Plus,
        uint32_t sampleCount = 1,
        TextureUsage usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding);
    
    ~WebGPUDepthOnlyFramebuffer() override;
    
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
     * @brief Get the native depth texture
     * @return Native texture handle
     */
    WGPUTexture getDepthTexture() const;
    
private:
    void createDepthTexture();
    void releaseDepthTexture();
    bool isDepthStencilFormat(TextureFormat format) const;
    
    // Device (weak_ptr to avoid circular reference)
    std::weak_ptr<WebGPULogicalDevice> _device;
    
    // Configuration
    uint32_t _width;
    uint32_t _height;
    TextureFormat _format;
    uint32_t _sampleCount;
    TextureUsage _usage;
    
    // Depth texture and view
    WGPUTexture _depthTexture = nullptr;
    std::shared_ptr<WebGPUTextureView> _depthView;
};

} // namespace pers