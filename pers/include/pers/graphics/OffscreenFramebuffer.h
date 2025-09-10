#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/ITexture.h"
#include "pers/graphics/ITextureView.h"
#include "pers/graphics/IResourceFactory.h"
#include <vector>
#include <memory>

namespace pers {

/**
 * @brief Configuration for offscreen framebuffer
 */
struct OffscreenFramebufferConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t sampleCount = 1;
    std::vector<TextureFormat> colorFormats;
    TextureFormat depthFormat = TextureFormat::Undefined;
    TextureUsage colorUsage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    TextureUsage depthUsage = TextureUsage::RenderAttachment;
};

/**
 * @brief Platform-independent offscreen framebuffer implementation
 * 
 * Creates and manages textures for offscreen rendering.
 * Works with any graphics backend through the IResourceFactory interface.
 */
class OffscreenFramebuffer final : public IResizableFramebuffer {
public:
    /**
     * @brief Construct an offscreen framebuffer
     * @param factory Resource factory for creating textures
     * @param config Framebuffer configuration
     */
    OffscreenFramebuffer(
        const std::shared_ptr<IResourceFactory>& factory,
        const OffscreenFramebufferConfig& config);
    
    ~OffscreenFramebuffer() override;
    
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
    
private:
    void createTextures();
    void releaseTextures();
    
private:
    std::shared_ptr<IResourceFactory> _factory;
    OffscreenFramebufferConfig _config;
    
    std::vector<std::shared_ptr<ITexture>> _colorTextures;
    std::vector<std::shared_ptr<ITextureView>> _colorViews;
    std::shared_ptr<ITexture> _depthTexture;
    std::shared_ptr<ITextureView> _depthView;
};

} // namespace pers