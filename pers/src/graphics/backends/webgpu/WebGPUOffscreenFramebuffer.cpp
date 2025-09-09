#include "pers/graphics/backends/webgpu/WebGPUOffscreenFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/graphics/backends/webgpu/WebGPUConverters.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUOffscreenFramebuffer::WebGPUOffscreenFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    const OffscreenFramebufferConfig& config)
    : _device(device)
    , _config(config) {
    
    if (!device) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "Invalid device provided");
        return;
    }
    
    if (_config.width == 0 || _config.height == 0) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "Invalid dimensions: " + 
            std::to_string(_config.width) + "x" + std::to_string(_config.height));
        return;
    }
    
    if (_config.colorFormats.empty()) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "At least one color format is required");
        return;
    }
    
    if (_config.colorFormats.size() > 8) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "Too many color attachments: " + 
            std::to_string(_config.colorFormats.size()) + ", maximum is 8");
        return;
    }
    
    // Validate sample count
    if (_config.sampleCount != 1 && _config.sampleCount != 2 && 
        _config.sampleCount != 4 && _config.sampleCount != 8) {
        LOG_WARNING("WebGPUOffscreenFramebuffer", "Invalid sample count: " + 
            std::to_string(_config.sampleCount) + ", defaulting to 1");
        _config.sampleCount = 1;
    }
    
    createTextures();
}

WebGPUOffscreenFramebuffer::~WebGPUOffscreenFramebuffer() {
    releaseTextures();
}

void WebGPUOffscreenFramebuffer::createTextures() {
    auto device = _device.lock();
    if (!device) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "Device expired during texture creation");
        return;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    
    // Reserve space for textures and views
    _colorTextures.reserve(_config.colorFormats.size());
    _colorViews.reserve(_config.colorFormats.size());
    
    // Create color attachments
    for (size_t i = 0; i < _config.colorFormats.size(); ++i) {
        WGPUTextureDescriptor textureDesc = {};
        std::string label = "Offscreen Color Texture " + std::to_string(i);
        textureDesc.label = WGPUStringView{.data = label.c_str(), .length = label.length()};
        textureDesc.usage = WebGPUConverters::convertTextureUsage(_config.colorUsage);
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = {_config.width, _config.height, 1};
        textureDesc.format = WebGPUConverters::convertTextureFormat(_config.colorFormats[i]);
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = _config.sampleCount;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats = nullptr;
        
        WGPUTexture texture = wgpuDeviceCreateTexture(wgpuDevice, &textureDesc);
        if (!texture) {
            LOG_ERROR("WebGPUOffscreenFramebuffer", 
                "Failed to create color texture " + std::to_string(i));
            releaseTextures();
            return;
        }
        _colorTextures.push_back(texture);
        
        // Create texture view
        WGPUTextureViewDescriptor viewDesc = {};
        std::string viewLabel = "Offscreen Color View " + std::to_string(i);
        viewDesc.label = WGPUStringView{.data = viewLabel.c_str(), .length = viewLabel.length()};
        viewDesc.format = textureDesc.format;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;
        
        WGPUTextureView view = wgpuTextureCreateView(texture, &viewDesc);
        if (!view) {
            LOG_ERROR("WebGPUOffscreenFramebuffer", 
                "Failed to create color texture view " + std::to_string(i));
            releaseTextures();
            return;
        }
        
        _colorViews.push_back(std::make_shared<WebGPUTextureView>(
            view, _config.width, _config.height, _config.colorFormats[i], false));
    }
    
    // Create depth/stencil attachment if requested
    if (_config.depthFormat != TextureFormat::Undefined) {
        WGPUTextureDescriptor textureDesc = {};
        const char* label = "Offscreen Depth Texture";
        textureDesc.label = WGPUStringView{.data = label, .length = strlen(label)};
        textureDesc.usage = WebGPUConverters::convertTextureUsage(_config.depthUsage);
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = {_config.width, _config.height, 1};
        textureDesc.format = WebGPUConverters::convertTextureFormat(_config.depthFormat);
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = _config.sampleCount;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats = nullptr;
        
        _depthTexture = wgpuDeviceCreateTexture(wgpuDevice, &textureDesc);
        if (!_depthTexture) {
            LOG_ERROR("WebGPUOffscreenFramebuffer", "Failed to create depth texture");
            releaseTextures();
            return;
        }
        
        // Create depth texture view
        WGPUTextureViewDescriptor viewDesc = {};
        const char* viewLabel = "Offscreen Depth View";
        viewDesc.label = WGPUStringView{.data = viewLabel, .length = strlen(viewLabel)};
        viewDesc.format = textureDesc.format;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        
        // Set aspect based on format
        if (isDepthStencilFormat(_config.depthFormat)) {
            viewDesc.aspect = WGPUTextureAspect_All;
        } else {
            viewDesc.aspect = WGPUTextureAspect_DepthOnly;
        }
        
        WGPUTextureView view = wgpuTextureCreateView(_depthTexture, &viewDesc);
        if (!view) {
            LOG_ERROR("WebGPUOffscreenFramebuffer", "Failed to create depth texture view");
            releaseTextures();
            return;
        }
        
        _depthView = std::make_shared<WebGPUTextureView>(
            view, _config.width, _config.height, _config.depthFormat, false);
    }
    
    LOG_DEBUG("WebGPUOffscreenFramebuffer", 
        "Created offscreen framebuffer: " + std::to_string(_config.width) + "x" + 
        std::to_string(_config.height) + ", " + std::to_string(_config.colorFormats.size()) + 
        " color attachments, sampleCount=" + std::to_string(_config.sampleCount));
}

void WebGPUOffscreenFramebuffer::releaseTextures() {
    // Release color textures
    for (auto texture : _colorTextures) {
        if (texture) {
            wgpuTextureRelease(texture);
        }
    }
    _colorTextures.clear();
    _colorViews.clear();
    
    // Release depth texture
    if (_depthTexture) {
        wgpuTextureRelease(_depthTexture);
        _depthTexture = nullptr;
    }
    _depthView.reset();
}

bool WebGPUOffscreenFramebuffer::isDepthStencilFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::Depth24PlusStencil8:
        case TextureFormat::Depth32FloatStencil8:
            return true;
        default:
            return false;
    }
}

std::shared_ptr<ITextureView> WebGPUOffscreenFramebuffer::getColorAttachment(uint32_t index) const {
    if (index >= _colorViews.size()) {
        return nullptr;
    }
    return _colorViews[index];
}

std::shared_ptr<ITextureView> WebGPUOffscreenFramebuffer::getDepthStencilAttachment() const {
    return _depthView;
}

uint32_t WebGPUOffscreenFramebuffer::getWidth() const {
    return _config.width;
}

uint32_t WebGPUOffscreenFramebuffer::getHeight() const {
    return _config.height;
}

uint32_t WebGPUOffscreenFramebuffer::getSampleCount() const {
    return _config.sampleCount;
}

TextureFormat WebGPUOffscreenFramebuffer::getColorFormat(uint32_t index) const {
    if (index >= _config.colorFormats.size()) {
        return TextureFormat::Undefined;
    }
    return _config.colorFormats[index];
}

TextureFormat WebGPUOffscreenFramebuffer::getDepthFormat() const {
    return _config.depthFormat;
}

uint32_t WebGPUOffscreenFramebuffer::getColorAttachmentCount() const {
    return static_cast<uint32_t>(_config.colorFormats.size());
}

bool WebGPUOffscreenFramebuffer::hasDepthStencilAttachment() const {
    return _config.depthFormat != TextureFormat::Undefined;
}

bool WebGPUOffscreenFramebuffer::resize(uint32_t width, uint32_t height) {
    if (_config.width == width && _config.height == height) {
        // No change needed
        return true;
    }
    
    _config.width = width;
    _config.height = height;
    
    // Release old textures
    releaseTextures();
    
    // Create new textures with new dimensions
    createTextures();
    
    // Check if creation succeeded
    if (_config.colorFormats.size() != _colorViews.size()) {
        LOG_ERROR("WebGPUOffscreenFramebuffer", "Failed to recreate textures after resize");
        return false;
    }
    
    return true;
}

WGPUTexture WebGPUOffscreenFramebuffer::getColorTexture(uint32_t index) const {
    if (index >= _colorTextures.size()) {
        return nullptr;
    }
    return _colorTextures[index];
}

WGPUTexture WebGPUOffscreenFramebuffer::getDepthTexture() const {
    return _depthTexture;
}

} // namespace pers