#include "pers/graphics/backends/webgpu/WebGPUSurfaceFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/graphics/backends/webgpu/WebGPUConverters.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUSurfaceFramebuffer::WebGPUSurfaceFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    WGPUSurface surface,
    uint32_t width,
    uint32_t height,
    TextureFormat format)
    : _device(device)
    , _surface(surface)
    , _width(width)
    , _height(height)
    , _format(format) {
    
    if (!_surface) {
        LOG_ERROR("WebGPUSurfaceFramebuffer", "Invalid surface provided");
        return;
    }
    
    if (!device) {
        LOG_ERROR("WebGPUSurfaceFramebuffer", "Invalid device provided");
        return;
    }
    
    configureSurface();
}

WebGPUSurfaceFramebuffer::~WebGPUSurfaceFramebuffer() {
    releaseCurrentTexture();
    
    // Surface is not owned by this framebuffer, so don't release it
    // The application/window system owns the surface
}

void WebGPUSurfaceFramebuffer::configureSurface() {
    auto device = _device.lock();
    if (!device) {
        LOG_ERROR("WebGPUSurfaceFramebuffer", "Device expired during surface configuration");
        return;
    }
    
    // Release any current texture before reconfiguring
    releaseCurrentTexture();
    
    // Configure surface (all parameters externally configurable)
    _surfaceConfig.device = device->getNativeDeviceHandle().as<WGPUDevice>();
    _surfaceConfig.format = WebGPUConverters::convertTextureFormat(_format);
    _surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    _surfaceConfig.viewFormatCount = 0;
    _surfaceConfig.viewFormats = nullptr;
    _surfaceConfig.alphaMode = WebGPUConverters::convertCompositeAlphaMode(_alphaMode);
    _surfaceConfig.width = _width;
    _surfaceConfig.height = _height;
    _surfaceConfig.presentMode = WebGPUConverters::convertPresentMode(_presentMode);
    
    wgpuSurfaceConfigure(_surface, &_surfaceConfig);
    
    LOG_DEBUG("WebGPUSurfaceFramebuffer", 
        "Surface configured: " + std::to_string(_width) + "x" + std::to_string(_height));
}

void WebGPUSurfaceFramebuffer::releaseCurrentTexture() {
    if (_currentColorView) {
        _currentColorView.reset();
    }
    
    // Note: WGPUSurfaceTexture is released when we get the next texture
    // or when the surface is reconfigured
    _acquired = false;
}

bool WebGPUSurfaceFramebuffer::acquireNextImage() {
    if (_acquired) {
        LOG_WARNING("WebGPUSurfaceFramebuffer", 
            "Attempting to acquire image when one is already acquired");
        return false;
    }
    
    // Get the next texture from the surface
    wgpuSurfaceGetCurrentTexture(_surface, &_currentTexture);
    
    if (_currentTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        _currentTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        LOG_ERROR("WebGPUSurfaceFramebuffer", 
            "Failed to acquire surface texture. Status: " + std::to_string(_currentTexture.status));
        
        // Handle specific error cases
        if (_currentTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
            LOG_INFO("WebGPUSurfaceFramebuffer", "Surface outdated, reconfiguring");
            configureSurface();
        }
        
        return false;
    }
    
    // Log suboptimal state for debugging
    if (_currentTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        LOG_DEBUG("WebGPUSurfaceFramebuffer", 
            "Surface is suboptimal, may need reconfiguration");
    }
    
    // Create texture view for the acquired texture
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label = WGPUStringView{.data = "Surface Color View", .length = 18};
    viewDesc.format = _surfaceConfig.format;
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = WGPUTextureAspect_All;
    
    WGPUTextureView view = wgpuTextureCreateView(_currentTexture.texture, &viewDesc);
    
    auto device = _device.lock();
    if (!device) {
        wgpuTextureViewRelease(view);
        LOG_ERROR("WebGPUSurfaceFramebuffer", "Device expired during texture view creation");
        return false;
    }
    
    _currentColorView = std::make_shared<WebGPUTextureView>(view, _width, _height, _format, true);
    _acquired = true;
    
    return true;
}

void WebGPUSurfaceFramebuffer::present() {
    if (!_acquired) {
        LOG_WARNING("WebGPUSurfaceFramebuffer", "Attempting to present without acquired image");
        return;
    }
    
    // Present the current texture
    wgpuSurfacePresent(_surface);
    
    // Release current resources
    releaseCurrentTexture();
}

bool WebGPUSurfaceFramebuffer::isReady() const {
    return _acquired;
}

void WebGPUSurfaceFramebuffer::setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer) {
    _depthFramebuffer = depthFramebuffer;
    
    if (_depthFramebuffer) {
        // Validate that the depth framebuffer has compatible dimensions
        if (_depthFramebuffer->getWidth() != _width || 
            _depthFramebuffer->getHeight() != _height) {
            LOG_WARNING("WebGPUSurfaceFramebuffer", 
                "Depth framebuffer dimensions don't match surface dimensions");
        }
    }
}

std::shared_ptr<ITextureView> WebGPUSurfaceFramebuffer::getColorAttachment(uint32_t index) const {
    if (index != 0) {
        // Surface framebuffer only has one color attachment
        return nullptr;
    }
    
    if (!_acquired) {
        LOG_ERROR("WebGPUSurfaceFramebuffer", 
            "getColorAttachment() called without acquiring image first");
        return nullptr;
    }
    
    return _currentColorView;
}

std::shared_ptr<ITextureView> WebGPUSurfaceFramebuffer::getDepthStencilAttachment() const {
    // Return depth from external framebuffer if set
    if (_depthFramebuffer) {
        return _depthFramebuffer->getDepthStencilAttachment();
    }
    
    return nullptr;
}

uint32_t WebGPUSurfaceFramebuffer::getWidth() const {
    return _width;
}

uint32_t WebGPUSurfaceFramebuffer::getHeight() const {
    return _height;
}

uint32_t WebGPUSurfaceFramebuffer::getSampleCount() const {
    // Surface framebuffers always have sample count of 1
    // MSAA is handled by offscreen framebuffers that resolve to this surface
    return 1;
}

TextureFormat WebGPUSurfaceFramebuffer::getColorFormat(uint32_t index) const {
    if (index != 0) {
        return TextureFormat::Undefined;
    }
    return _format;
}

TextureFormat WebGPUSurfaceFramebuffer::getDepthFormat() const {
    if (_depthFramebuffer) {
        return _depthFramebuffer->getDepthFormat();
    }
    return TextureFormat::Undefined;
}

uint32_t WebGPUSurfaceFramebuffer::getColorAttachmentCount() const {
    // Surface framebuffer always has exactly one color attachment
    return 1;
}

bool WebGPUSurfaceFramebuffer::hasDepthStencilAttachment() const {
    return _depthFramebuffer && _depthFramebuffer->hasDepthStencilAttachment();
}

bool WebGPUSurfaceFramebuffer::resize(uint32_t width, uint32_t height) {
    if (_width == width && _height == height) {
        // No change needed
        return true;
    }
    
    _width = width;
    _height = height;
    
    // Reconfigure surface with new dimensions
    configureSurface();
    
    // Also resize the associated depth framebuffer if it's resizable
    if (_depthFramebuffer) {
        if (auto resizable = dynamic_cast<IResizableFramebuffer*>(_depthFramebuffer.get())) {
            if (!resizable->resize(width, height)) {
                LOG_WARNING("WebGPUSurfaceFramebuffer", 
                    "Failed to resize associated depth framebuffer");
            }
        }
    }
    
    return true;
}

void WebGPUSurfaceFramebuffer::setPresentMode(PresentMode mode) {
    if (_presentMode != mode) {
        _presentMode = mode;
        // Reconfigure surface with new present mode
        configureSurface();
    }
}

void WebGPUSurfaceFramebuffer::setCompositeAlphaMode(CompositeAlphaMode mode) {
    if (_alphaMode != mode) {
        _alphaMode = mode;
        // Reconfigure surface with new alpha mode
        configureSurface();
    }
}

} // namespace pers