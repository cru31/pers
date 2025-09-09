#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/SwapChainTypes.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

class WebGPUTextureView;
class WebGPULogicalDevice;

/**
 * @brief WebGPU implementation of ISurfaceFramebuffer
 * 
 * Wraps a WebGPU surface for presentation to a window.
 * Manages the acquire/present lifecycle of SwapChain images.
 */
class WebGPUSurfaceFramebuffer : public ISurfaceFramebuffer {
public:
    /**
     * @brief Construct a surface framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param surface The WebGPU surface
     * @param width Initial width
     * @param height Initial height
     * @param format Surface format (externally configurable, no hardcoding)
     */
    WebGPUSurfaceFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        WGPUSurface surface,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::BGRA8Unorm);
    
    ~WebGPUSurfaceFramebuffer() override;
    
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
    
    // ISurfaceFramebuffer interface
    bool acquireNextImage() override;
    void present() override;
    bool isReady() const override;
    void setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer) override;
    
    /**
     * @brief Set present mode (externally configurable)
     * @param mode The present mode (Immediate, Mailbox, Fifo, etc.)
     */
    void setPresentMode(PresentMode mode);
    
    /**
     * @brief Set composite alpha mode (externally configurable)
     * @param mode The alpha mode
     */
    void setCompositeAlphaMode(CompositeAlphaMode mode);
    
private:
    void configureSurface();
    void releaseCurrentTexture();
    
    // Device and surface (weak_ptr for device to avoid circular reference)
    std::weak_ptr<WebGPULogicalDevice> _device;
    WGPUSurface _surface = nullptr;
    
    // Current frame resources
    WGPUSurfaceTexture _currentTexture = {};
    std::shared_ptr<WebGPUTextureView> _currentColorView;
    bool _acquired = false;
    
    // Optional external depth framebuffer
    std::shared_ptr<IFramebuffer> _depthFramebuffer;
    
    // Surface configuration (all externally configurable)
    uint32_t _width;
    uint32_t _height;
    TextureFormat _format;
    PresentMode _presentMode = PresentMode::Fifo;  // Default VSync
    CompositeAlphaMode _alphaMode = CompositeAlphaMode::Auto;
    
    // WebGPU surface configuration
    WGPUSurfaceConfiguration _surfaceConfig = {};
};

} // namespace pers