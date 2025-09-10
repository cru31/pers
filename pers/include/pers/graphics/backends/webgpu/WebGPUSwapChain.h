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
    SurfaceCapabilities querySurfaceCapabilities() const override;
    
    /**
     * @brief Query surface capabilities from the device
     */
    static SurfaceCapabilities querySurfaceCapabilities(
        WGPUAdapter adapter,
        WGPUSurface surface);
    
private:
    void configureSurface();
    void releaseCurrentTexture();
    
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
};

} // namespace pers