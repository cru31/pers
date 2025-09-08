#pragma once

#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

class WebGPULogicalDevice;
class WebGPUTextureView;

/**
 * @brief WebGPU implementation of ISwapChain
 */
class IPhysicalDevice;

class WebGPUSwapChain : public ISwapChain {
public:
    WebGPUSwapChain(const std::shared_ptr<WebGPULogicalDevice>& device,
                    WGPUSurface surface,
                    const SwapChainDesc& desc);
    ~WebGPUSwapChain() override;
    
    // ISwapChain interface
    std::shared_ptr<ITextureView> getCurrentTextureView() override;
    void present() override;
    void resize(uint32_t width, uint32_t height) override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    PresentMode getPresentMode() const override;
    TextureFormat getFormat() const override;
    SurfaceCapabilities querySurfaceCapabilities(
        const std::shared_ptr<IPhysicalDevice>& physicalDevice) const override;
    void setDepthBufferEnabled(bool enabled) override;
    std::shared_ptr<ITextureView> getDepthTextureView() override;
    std::shared_ptr<RenderPassDepthStencilAttachment> getDepthStencilAttachment(
        const DepthStencilOptions& options = {}) override;
    
    /**
     * @brief Query surface capabilities from the device
     */
    static SurfaceCapabilities querySurfaceCapabilities(
        WGPUDevice device,
        WGPUAdapter adapter,
        WGPUSurface surface);
    
private:
    void configureSurface();
    void releaseCurrentTexture();
    void createDepthTexture();
    void releaseDepthTexture();
    
    static WGPUTextureFormat convertToWGPUFormat(TextureFormat format);
    static WGPUPresentMode convertToWGPUPresentMode(PresentMode mode);
    static WGPUCompositeAlphaMode convertToWGPUAlphaMode(CompositeAlphaMode mode);
    
    static TextureFormat convertFromWGPUFormat(WGPUTextureFormat format);
    static PresentMode convertFromWGPUPresentMode(WGPUPresentMode mode);
    static CompositeAlphaMode convertFromWGPUAlphaMode(WGPUCompositeAlphaMode mode);
    
    std::weak_ptr<WebGPULogicalDevice> _device;
    WGPUSurface _surface = nullptr;
    SwapChainDesc _desc = {};
    
    // Surface configuration
    WGPUSurfaceConfiguration _surfaceConfig = {};
    
    // Current frame resources
    WGPUSurfaceTexture _currentSurfaceTexture = {};
    WGPUTextureView _currentTextureView = nullptr;
    std::shared_ptr<WebGPUTextureView> _currentTextureViewWrapper;
    bool _hasCurrentTexture = false;
    
    // Depth buffer management
    bool _depthBufferEnabled = true;  // Enabled by default for 3D rendering
    WGPUTexture _depthTexture = nullptr;
    WGPUTextureView _depthTextureView = nullptr;
    std::shared_ptr<WebGPUTextureView> _depthTextureViewWrapper;
};

} // namespace pers