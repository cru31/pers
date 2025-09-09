#include "pers/graphics/backends/webgpu/WebGPUDepthOnlyFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/graphics/backends/webgpu/WebGPUConverters.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUDepthOnlyFramebuffer::WebGPUDepthOnlyFramebuffer(
    const std::shared_ptr<WebGPULogicalDevice>& device,
    uint32_t width,
    uint32_t height,
    TextureFormat format,
    uint32_t sampleCount,
    TextureUsage usage)
    : _device(device)
    , _width(width)
    , _height(height)
    , _format(format)
    , _sampleCount(sampleCount)
    , _usage(usage) {
    
    if (!device) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Invalid device provided");
        return;
    }
    
    if (_width == 0 || _height == 0) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Invalid dimensions: " + 
            std::to_string(_width) + "x" + std::to_string(_height));
        return;
    }
    
    // Validate that format is a depth format
    switch (_format) {
        case TextureFormat::Depth16Unorm:
        case TextureFormat::Depth24Plus:
        case TextureFormat::Depth24PlusStencil8:
        case TextureFormat::Depth32Float:
        case TextureFormat::Depth32FloatStencil8:
        case TextureFormat::Stencil8:
            // Valid depth/stencil format
            break;
        default:
            LOG_ERROR("WebGPUDepthOnlyFramebuffer", 
                "Invalid depth format: " + std::to_string(static_cast<int>(_format)));
            return;
    }
    
    // Validate sample count
    if (_sampleCount != 1 && _sampleCount != 2 && 
        _sampleCount != 4 && _sampleCount != 8) {
        LOG_WARNING("WebGPUDepthOnlyFramebuffer", "Invalid sample count: " + 
            std::to_string(_sampleCount) + ", defaulting to 1");
        _sampleCount = 1;
    }
    
    createDepthTexture();
}

WebGPUDepthOnlyFramebuffer::~WebGPUDepthOnlyFramebuffer() {
    releaseDepthTexture();
}

void WebGPUDepthOnlyFramebuffer::createDepthTexture() {
    auto device = _device.lock();
    if (!device) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Device expired during texture creation");
        return;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    
    // Create depth texture
    WGPUTextureDescriptor textureDesc = {};
    const char* label = "Depth-Only Texture";
    textureDesc.label = WGPUStringView{.data = label, .length = strlen(label)};
    textureDesc.usage = WebGPUConverters::convertTextureUsage(_usage);
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = {_width, _height, 1};
    textureDesc.format = WebGPUConverters::convertTextureFormat(_format);
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = _sampleCount;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    
    _depthTexture = wgpuDeviceCreateTexture(wgpuDevice, &textureDesc);
    if (!_depthTexture) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Failed to create depth texture");
        return;
    }
    
    // Create depth texture view
    WGPUTextureViewDescriptor viewDesc = {};
    const char* viewLabel = "Depth-Only View";
    viewDesc.label = WGPUStringView{.data = viewLabel, .length = strlen(viewLabel)};
    viewDesc.format = textureDesc.format;
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    
    // Set aspect based on format
    if (isDepthStencilFormat(_format)) {
        viewDesc.aspect = WGPUTextureAspect_All;
    } else if (_format == TextureFormat::Stencil8) {
        viewDesc.aspect = WGPUTextureAspect_StencilOnly;
    } else {
        viewDesc.aspect = WGPUTextureAspect_DepthOnly;
    }
    
    WGPUTextureView view = wgpuTextureCreateView(_depthTexture, &viewDesc);
    if (!view) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Failed to create depth texture view");
        wgpuTextureRelease(_depthTexture);
        _depthTexture = nullptr;
        return;
    }
    
    _depthView = std::make_shared<WebGPUTextureView>(
        view, _width, _height, _format, false);
    
    LOG_DEBUG("WebGPUDepthOnlyFramebuffer", 
        "Created depth-only framebuffer: " + std::to_string(_width) + "x" + 
        std::to_string(_height) + ", format=" + std::to_string(static_cast<int>(_format)) +
        ", sampleCount=" + std::to_string(_sampleCount));
}

void WebGPUDepthOnlyFramebuffer::releaseDepthTexture() {
    if (_depthTexture) {
        wgpuTextureRelease(_depthTexture);
        _depthTexture = nullptr;
    }
    _depthView.reset();
}

bool WebGPUDepthOnlyFramebuffer::isDepthStencilFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::Depth24PlusStencil8:
        case TextureFormat::Depth32FloatStencil8:
            return true;
        default:
            return false;
    }
}

std::shared_ptr<ITextureView> WebGPUDepthOnlyFramebuffer::getColorAttachment(uint32_t index) const {
    // Depth-only framebuffer has no color attachments
    return nullptr;
}

std::shared_ptr<ITextureView> WebGPUDepthOnlyFramebuffer::getDepthStencilAttachment() const {
    return _depthView;
}

uint32_t WebGPUDepthOnlyFramebuffer::getWidth() const {
    return _width;
}

uint32_t WebGPUDepthOnlyFramebuffer::getHeight() const {
    return _height;
}

uint32_t WebGPUDepthOnlyFramebuffer::getSampleCount() const {
    return _sampleCount;
}

TextureFormat WebGPUDepthOnlyFramebuffer::getColorFormat(uint32_t index) const {
    // No color attachments
    return TextureFormat::Undefined;
}

TextureFormat WebGPUDepthOnlyFramebuffer::getDepthFormat() const {
    return _format;
}

uint32_t WebGPUDepthOnlyFramebuffer::getColorAttachmentCount() const {
    // No color attachments
    return 0;
}

bool WebGPUDepthOnlyFramebuffer::hasDepthStencilAttachment() const {
    // Always has depth attachment
    return true;
}

bool WebGPUDepthOnlyFramebuffer::resize(uint32_t width, uint32_t height) {
    if (_width == width && _height == height) {
        // No change needed
        return true;
    }
    
    _width = width;
    _height = height;
    
    // Release old texture
    releaseDepthTexture();
    
    // Create new texture with new dimensions
    createDepthTexture();
    
    // Check if creation succeeded
    if (!_depthTexture) {
        LOG_ERROR("WebGPUDepthOnlyFramebuffer", "Failed to recreate depth texture after resize");
        return false;
    }
    
    return true;
}

WGPUTexture WebGPUDepthOnlyFramebuffer::getDepthTexture() const {
    return _depthTexture;
}

} // namespace pers