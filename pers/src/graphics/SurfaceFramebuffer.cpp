#include "pers/graphics/SurfaceFramebuffer.h"
#include "pers/graphics/OffscreenFramebuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

SurfaceFramebuffer::SurfaceFramebuffer(const std::shared_ptr<ILogicalDevice>& device)
    : _device(device)
    , _width(0)
    , _height(0)
    , _format(TextureFormat::Undefined)
    , _depthFormat(TextureFormat::Undefined)
    , _acquired(false) {
    
    if (!device) {
        LOG_ERROR("SurfaceFramebuffer", "Invalid device provided");
    }
}

SurfaceFramebuffer::~SurfaceFramebuffer() {
    destroy();
}

bool SurfaceFramebuffer::create(const NativeSurfaceHandle& surface, const SwapChainDesc& desc, 
                                TextureFormat depthFormat) {
    if (!_device) {
        LOG_ERROR("SurfaceFramebuffer", "Device not set");
        return false;
    }
    
    // Destroy existing swap chain if any
    destroy();
    
    // Store dimensions and format from description
    _width = desc.width;
    _height = desc.height;
    _format = desc.format;
    _depthFormat = depthFormat;
    
    // Create swap chain through device
    _swapChain = _device->createSwapChain(surface, desc);
    
    if (!_swapChain) {
        LOG_ERROR("SurfaceFramebuffer", "Failed to create swap chain");
        return false;
    }
    
    // Query and store surface capabilities
    _surfaceCapabilities = _swapChain->querySurfaceCapabilities();
    
    // Create depth buffer if needed
    if (_depthFormat != TextureFormat::Undefined) {
        createDepthBuffer();
    }
    
    LOG_INFO("SurfaceFramebuffer", "Created swap chain");
    return true;
}

void SurfaceFramebuffer::destroy() {
    // Clean up acquired state first
    if (_acquired) {
        LOG_WARNING("SurfaceFramebuffer", "Destroying while image is acquired");
        _currentColorView.reset();
        _acquired = false;
    }
    
    // Destroy in exact reverse order of creation
    // 3. Destroy depth buffer (created last in create())
    _depthFramebuffer.reset();
    
    // 2. Destroy swap chain (created second in create())
    _swapChain.reset();
    
    // 1. Reset dimensions and format (set first in create())
    _width = 0;
    _height = 0;
    _format = TextureFormat::Undefined;
    // Note: _depthFormat stays as-is since it's a configuration value, not a created resource
}

void SurfaceFramebuffer::createDepthBuffer() {
    const auto& factory = _device->getResourceFactory();
    if (!factory) {
        LOG_ERROR("SurfaceFramebuffer", "Failed to get resource factory");
        return;
    }
    
    // Create offscreen framebuffer for depth only
    OffscreenFramebufferConfig config;
    config.width = _width;
    config.height = _height;
    config.depthFormat = _depthFormat;
    config.depthUsage = TextureUsage::RenderAttachment;
    config.sampleCount = 1;
    
    _depthFramebuffer = std::make_shared<OffscreenFramebuffer>(factory, config);
    
    if (!_depthFramebuffer) {
        LOG_WARNING("SurfaceFramebuffer", "Failed to create depth buffer");
    }
}

std::shared_ptr<ITextureView> SurfaceFramebuffer::getColorAttachment(uint32_t index) const {
    if (index != 0) {
        return nullptr;
    }
    
    if (!_acquired) {
        LOG_ERROR("SurfaceFramebuffer", "Image not acquired");
        return nullptr;
    }
    
    return _currentColorView;
}

std::shared_ptr<ITextureView> SurfaceFramebuffer::getDepthStencilAttachment() const {
    if (_depthFramebuffer) {
        return _depthFramebuffer->getDepthStencilAttachment();
    }
    return nullptr;
}

uint32_t SurfaceFramebuffer::getWidth() const {
    return _width;
}

uint32_t SurfaceFramebuffer::getHeight() const {
    return _height;
}

uint32_t SurfaceFramebuffer::getSampleCount() const {
    return 1;
}

TextureFormat SurfaceFramebuffer::getColorFormat(uint32_t index) const {
    if (index != 0) {
        return TextureFormat::Undefined;
    }
    return _format;
}

TextureFormat SurfaceFramebuffer::getDepthFormat() const {
    return _depthFormat;
}

uint32_t SurfaceFramebuffer::getColorAttachmentCount() const {
    return 1;
}

bool SurfaceFramebuffer::hasDepthStencilAttachment() const {
    return _depthFramebuffer != nullptr;
}

bool SurfaceFramebuffer::resize(uint32_t width, uint32_t height) {
    if (_width == width && _height == height) {
        return true;
    }
    
    _width = width;
    _height = height;
    
    // Resize swap chain
    if (_swapChain) {
        _swapChain->resize(width, height);
    }
    
    // Recreate depth buffer with new size
    createDepthBuffer();
    
    return true;
}

bool SurfaceFramebuffer::acquireNextImage() {
    if (_acquired) {
        LOG_WARNING("SurfaceFramebuffer", "Image already acquired");
        _currentColorView.reset();
        _acquired = false;
    }
    
    if (!_swapChain) {
        LOG_ERROR("SurfaceFramebuffer", "No swap chain available");
        return false;
    }
    
    _currentColorView = _swapChain->getCurrentTextureView();
    
    if (!_currentColorView) {
        LOG_ERROR("SurfaceFramebuffer", "Failed to acquire next image");
        return false;
    }
    
    _acquired = true;
    return true;
}

void SurfaceFramebuffer::present() {
    if (!_acquired) {
        LOG_WARNING("SurfaceFramebuffer", "No image to present");
        return;
    }
    
    if (_swapChain) {
        _swapChain->present();
    }
    
    _currentColorView.reset();
    _acquired = false;
}

bool SurfaceFramebuffer::isReady() const {
    return _acquired;
}

void SurfaceFramebuffer::setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer) {
    _depthFramebuffer = depthFramebuffer;
    
    if (_depthFramebuffer) {
        if (_depthFramebuffer->getWidth() != _width || 
            _depthFramebuffer->getHeight() != _height) {
            LOG_WARNING("SurfaceFramebuffer", 
                "Depth framebuffer dimensions don't match surface dimensions");
        }
    }
}

} // namespace pers