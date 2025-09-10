#pragma once

#include "pers/graphics/ISurfaceFramebuffer.h"
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include <memory>

namespace pers {

/**
 * @brief Backend-independent surface framebuffer implementation
 * 
 * This class wraps an ISwapChain and provides a unified framebuffer interface
 * for surface rendering. It automatically manages an associated depth buffer
 * and provides a consistent API regardless of the graphics backend.
 * 
 * The actual SwapChain implementation (WebGPU, Vulkan, D3D12, etc.) is
 * created through the ILogicalDevice interface, making this class completely
 * backend-agnostic.
 */
class SurfaceFramebuffer final : public ISurfaceFramebuffer {
public:
    /**
     * @brief Construct an empty surface framebuffer
     * @param device The logical device for creating resources
     */
    explicit SurfaceFramebuffer(const std::shared_ptr<ILogicalDevice>& device);
    
    ~SurfaceFramebuffer() override;
    
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
    bool create(const NativeSurfaceHandle& surface, const SwapChainDesc& desc,
                TextureFormat depthFormat = TextureFormat::Depth24PlusStencil8) override;
    void destroy() override;
    bool acquireNextImage() override;
    void present() override;
    bool isReady() const override;
    void setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer) override;
    
    /**
     * @brief Get the surface capabilities
     * @return Surface capabilities that were queried during create()
     */
    const SurfaceCapabilities& getSurfaceCapabilities() const { return _surfaceCapabilities; }
    
private:
    void createDepthBuffer();
    
private:
    std::shared_ptr<ILogicalDevice> _device;
    std::shared_ptr<ISwapChain> _swapChain;
    std::shared_ptr<IFramebuffer> _depthFramebuffer;
    std::shared_ptr<ITextureView> _currentColorView;
    
    uint32_t _width;
    uint32_t _height;
    TextureFormat _format;
    TextureFormat _depthFormat;
    bool _acquired;
    SurfaceCapabilities _surfaceCapabilities;
};

} // namespace pers