#include "pers/graphics/OffscreenFramebuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

OffscreenFramebuffer::OffscreenFramebuffer(
    const std::shared_ptr<IResourceFactory>& factory,
    const OffscreenFramebufferConfig& config)
    : _factory(factory)
    , _config(config) {
    
    if (!factory) {
        LOG_ERROR("OffscreenFramebuffer", "Invalid resource factory provided");
        return;
    }
    
    if (_config.width == 0 || _config.height == 0) {
        LOG_ERROR("OffscreenFramebuffer", "Invalid dimensions: " + 
            std::to_string(_config.width) + "x" + std::to_string(_config.height));
        return;
    }
    
    if (_config.colorFormats.empty() && _config.depthFormat == TextureFormat::Undefined) {
        LOG_ERROR("OffscreenFramebuffer", "At least one attachment is required");
        return;
    }
    
    createTextures();
}

OffscreenFramebuffer::~OffscreenFramebuffer() {
    releaseTextures();
}

void OffscreenFramebuffer::createTextures() {
    // Reserve space for textures and views
    _colorTextures.reserve(_config.colorFormats.size());
    _colorViews.reserve(_config.colorFormats.size());
    
    // Create color attachments
    for (size_t i = 0; i < _config.colorFormats.size(); ++i) {
        TextureDesc textureDesc;
        textureDesc.width = _config.width;
        textureDesc.height = _config.height;
        textureDesc.format = _config.colorFormats[i];
        textureDesc.usage = _config.colorUsage;
        textureDesc.sampleCount = _config.sampleCount;
        textureDesc.label = "OffscreenColorTexture" + std::to_string(i);
        
        auto texture = _factory->createTexture(textureDesc);
        if (!texture) {
            LOG_ERROR("OffscreenFramebuffer", 
                "Failed to create color texture " + std::to_string(i));
            releaseTextures();
            return;
        }
        _colorTextures.push_back(texture);
        
        // Create texture view
        TextureViewDesc viewDesc;
        viewDesc.format = _config.colorFormats[i];
        viewDesc.label = "OffscreenColorView" + std::to_string(i);
        
        auto view = _factory->createTextureView(texture, viewDesc);
        if (!view) {
            LOG_ERROR("OffscreenFramebuffer", 
                "Failed to create color texture view " + std::to_string(i));
            releaseTextures();
            return;
        }
        _colorViews.push_back(view);
    }
    
    // Create depth/stencil attachment if requested
    if (_config.depthFormat != TextureFormat::Undefined) {
        TextureDesc textureDesc;
        textureDesc.width = _config.width;
        textureDesc.height = _config.height;
        textureDesc.format = _config.depthFormat;
        textureDesc.usage = _config.depthUsage;
        textureDesc.sampleCount = _config.sampleCount;
        textureDesc.label = "OffscreenDepthTexture";
        
        _depthTexture = _factory->createTexture(textureDesc);
        if (!_depthTexture) {
            LOG_ERROR("OffscreenFramebuffer", "Failed to create depth texture");
            releaseTextures();
            return;
        }
        
        // Create depth texture view
        TextureViewDesc viewDesc;
        viewDesc.format = _config.depthFormat;
        viewDesc.label = "OffscreenDepthView";
        
        _depthView = _factory->createTextureView(_depthTexture, viewDesc);
        if (!_depthView) {
            LOG_ERROR("OffscreenFramebuffer", "Failed to create depth texture view");
            releaseTextures();
            return;
        }
    }
    
    LOG_DEBUG("OffscreenFramebuffer", 
        "Created offscreen framebuffer: " + std::to_string(_config.width) + "x" + 
        std::to_string(_config.height) + ", " + std::to_string(_config.colorFormats.size()) + 
        " color attachments, sampleCount=" + std::to_string(_config.sampleCount));
}

void OffscreenFramebuffer::releaseTextures() {
    _colorTextures.clear();
    _colorViews.clear();
    _depthTexture.reset();
    _depthView.reset();
}

std::shared_ptr<ITextureView> OffscreenFramebuffer::getColorAttachment(uint32_t index) const {
    if (index >= _colorViews.size()) {
        return nullptr;
    }
    return _colorViews[index];
}

std::shared_ptr<ITextureView> OffscreenFramebuffer::getDepthStencilAttachment() const {
    return _depthView;
}

uint32_t OffscreenFramebuffer::getWidth() const {
    return _config.width;
}

uint32_t OffscreenFramebuffer::getHeight() const {
    return _config.height;
}

uint32_t OffscreenFramebuffer::getSampleCount() const {
    return _config.sampleCount;
}

TextureFormat OffscreenFramebuffer::getColorFormat(uint32_t index) const {
    if (index >= _config.colorFormats.size()) {
        return TextureFormat::Undefined;
    }
    return _config.colorFormats[index];
}

TextureFormat OffscreenFramebuffer::getDepthFormat() const {
    return _config.depthFormat;
}

uint32_t OffscreenFramebuffer::getColorAttachmentCount() const {
    return static_cast<uint32_t>(_config.colorFormats.size());
}

bool OffscreenFramebuffer::hasDepthStencilAttachment() const {
    return _config.depthFormat != TextureFormat::Undefined;
}

bool OffscreenFramebuffer::resize(uint32_t width, uint32_t height) {
    if (_config.width == width && _config.height == height) {
        return true;
    }
    
    _config.width = width;
    _config.height = height;
    
    // Release old textures
    releaseTextures();
    
    // Create new textures with new dimensions
    createTextures();
    
    // Check if creation succeeded
    if (!_config.colorFormats.empty() && _colorViews.size() != _config.colorFormats.size()) {
        LOG_ERROR("OffscreenFramebuffer", "Failed to recreate color textures after resize");
        return false;
    }
    
    if (_config.depthFormat != TextureFormat::Undefined && !_depthView) {
        LOG_ERROR("OffscreenFramebuffer", "Failed to recreate depth texture after resize");
        return false;
    }
    
    return true;
}

} // namespace pers