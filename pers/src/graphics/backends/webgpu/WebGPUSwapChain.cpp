#include "pers/graphics/backends/webgpu/WebGPUSwapChain.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoSomeday.h"
#include <algorithm>
#include <stdexcept>

namespace pers {

// Helper function for default texture dimension
constexpr uint32_t getDefaultMaxTextureDimension() {
    // WebGPU minimum guaranteed value
    return 8192;
}

WebGPUSwapChain::WebGPUSwapChain(const std::shared_ptr<WebGPULogicalDevice>& device,
                                 WGPUSurface surface,
                                 const SwapChainDesc& desc)
    : _device(device)
    , _surface(surface)
    , _desc(desc) {
    
    if (!_device) {
        throw std::invalid_argument("WebGPUSwapChain: device cannot be null");
    }
    
    if (!_surface) {
        throw std::invalid_argument("WebGPUSwapChain: surface cannot be null");
    }
    
    configureSurface();
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUSwapChain", 
                          "Created: " + std::to_string(_desc.width) + "x" + std::to_string(_desc.height), 
                          PERS_SOURCE_LOC);
}

WebGPUSwapChain::~WebGPUSwapChain() {
    releaseCurrentTexture();
    
    // Surface is not owned by SwapChain, so we don't release it
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUSwapChain", "Destroyed", PERS_SOURCE_LOC);
}

void WebGPUSwapChain::configureSurface() {
    // Configure surface with the new Surface API
    _surfaceConfig = {};
    _surfaceConfig.device = _device->getNativeDeviceHandle().as<WGPUDevice>();
    _surfaceConfig.format = convertToWGPUFormat(_desc.format);
    _surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    _surfaceConfig.width = _desc.width;
    _surfaceConfig.height = _desc.height;
    _surfaceConfig.presentMode = convertToWGPUPresentMode(_desc.presentMode);
    _surfaceConfig.alphaMode = convertToWGPUAlphaMode(_desc.alphaMode);
    
    // Configure the surface
    wgpuSurfaceConfigure(_surface, &_surfaceConfig);
}

void WebGPUSwapChain::releaseCurrentTexture() {
    if (_currentTextureView) {
        wgpuTextureViewRelease(_currentTextureView);
        _currentTextureView = nullptr;
    }
    
    if (_currentSurfaceTexture.texture) {
        wgpuTextureRelease(_currentSurfaceTexture.texture);
        _currentSurfaceTexture.texture = nullptr;
    }
    
    _currentTextureViewWrapper.reset();
    _hasCurrentTexture = false;
}

std::shared_ptr<ITextureView> WebGPUSwapChain::getCurrentTextureView() {
    // Release previous texture if any
    releaseCurrentTexture();
    
    // Get current texture from surface
    wgpuSurfaceGetCurrentTexture(_surface, &_currentSurfaceTexture);
    
    if (_currentSurfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && 
        _currentSurfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUSwapChain", 
                              "Failed to get current texture from surface, status: " + 
                              std::to_string(static_cast<int>(_currentSurfaceTexture.status)), 
                              PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Log suboptimal state for debugging
    if (_currentSurfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        Logger::Instance().Log(LogLevel::Debug, "WebGPUSwapChain", 
                              "Surface is suboptimal, may need reconfiguration", PERS_SOURCE_LOC);
    }
    
    // Create texture view from the surface texture  
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.format = convertToWGPUFormat(_desc.format);
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = WGPUTextureAspect_All;
    _currentTextureView = wgpuTextureCreateView(_currentSurfaceTexture.texture, &viewDesc);
    
    if (!_currentTextureView) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUSwapChain", 
                              "Failed to create texture view from surface texture", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Create wrapper for the texture view
    _currentTextureViewWrapper = std::make_shared<WebGPUTextureView>(
        _currentTextureView,
        _desc.width,
        _desc.height,
        _desc.format
    );
    
    _hasCurrentTexture = true;
    
    return _currentTextureViewWrapper;
}

void WebGPUSwapChain::present() {
    if (!_hasCurrentTexture) {
        Logger::Instance().Log(LogLevel::Warning, "WebGPUSwapChain", 
                              "present() called without current texture", PERS_SOURCE_LOC);
        return;
    }
    
    // Present the surface
    wgpuSurfacePresent(_surface);
    
    // After present, the current texture is no longer valid
    releaseCurrentTexture();
}

void WebGPUSwapChain::resize(uint32_t width, uint32_t height) {
    if (width == _desc.width && height == _desc.height) {
        return; // No change needed
    }
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUSwapChain", 
                          "Resizing from " + std::to_string(_desc.width) + "x" + std::to_string(_desc.height) + 
                          " to " + std::to_string(width) + "x" + std::to_string(height), 
                          PERS_SOURCE_LOC);
    
    // Release current texture if any
    releaseCurrentTexture();
    
    // Update descriptor
    _desc.width = width;
    _desc.height = height;
    
    // Reconfigure surface with new size
    configureSurface();
}

uint32_t WebGPUSwapChain::getWidth() const {
    return _desc.width;
}

uint32_t WebGPUSwapChain::getHeight() const {
    return _desc.height;
}

PresentMode WebGPUSwapChain::getPresentMode() const {
    return _desc.presentMode;
}

TextureFormat WebGPUSwapChain::getFormat() const {
    return _desc.format;
}

SurfaceCapabilities WebGPUSwapChain::querySurfaceCapabilities(
    WGPUDevice device,
    WGPUAdapter adapter,
    WGPUSurface surface) {
    
    WGPUSurfaceCapabilities wgpuCaps = {};
    wgpuSurfaceGetCapabilities(surface, adapter, &wgpuCaps);
    
    SurfaceCapabilities caps;
    
    // Convert formats
    caps.formats.reserve(wgpuCaps.formatCount);
    for (size_t i = 0; i < wgpuCaps.formatCount; ++i) {
        auto format = convertFromWGPUFormat(wgpuCaps.formats[i]);
        if (format != TextureFormat::Undefined) {
            caps.formats.push_back(format);
        }
    }
    
    // Convert present modes
    caps.presentModes.reserve(wgpuCaps.presentModeCount);
    for (size_t i = 0; i < wgpuCaps.presentModeCount; ++i) {
        caps.presentModes.push_back(
            convertFromWGPUPresentMode(wgpuCaps.presentModes[i]));
    }
    
    // Convert alpha modes
    caps.alphaModes.reserve(wgpuCaps.alphaModeCount);
    for (size_t i = 0; i < wgpuCaps.alphaModeCount; ++i) {
        caps.alphaModes.push_back(
            convertFromWGPUAlphaMode(wgpuCaps.alphaModes[i]));
    }
    
    // Use default texture limits with runtime notification
    caps.maxWidth = getDefaultMaxTextureDimension();
    caps.maxHeight = getDefaultMaxTextureDimension();
    
    TodoSomeday::Log("WebGPUSwapChain", 
        "Using default texture limits (8192x8192). Actual adapter limits will be queried when API stabilizes.", 
        PERS_SOURCE_LOC);
    
    // Set minimum dimensions
    caps.minWidth = 1;
    caps.minHeight = 1;
    
    // Image count based on present mode support
    caps.minImageCount = 2;
    
    // Check if triple buffering is supported
    bool hasMailbox = std::find(caps.presentModes.begin(),
                               caps.presentModes.end(),
                               PresentMode::Mailbox) != caps.presentModes.end();
    caps.maxImageCount = hasMailbox ? 3 : 2;
    
    // Free WebGPU allocated memory
    wgpuSurfaceCapabilitiesFreeMembers(wgpuCaps);
    
    Logger::Instance().Log(LogLevel::Debug, "WebGPUSwapChain", 
                          "Surface capabilities: " + std::to_string(caps.formats.size()) + " formats, " +
                          std::to_string(caps.presentModes.size()) + " present modes, max size " +
                          std::to_string(caps.maxWidth) + "x" + std::to_string(caps.maxHeight), 
                          PERS_SOURCE_LOC);
    
    return caps;
}

// Format conversion functions
WGPUTextureFormat WebGPUSwapChain::convertToWGPUFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case TextureFormat::BGRA8UnormSrgb: return WGPUTextureFormat_BGRA8UnormSrgb;
        case TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        case TextureFormat::RGBA8UnormSrgb: return WGPUTextureFormat_RGBA8UnormSrgb;
        case TextureFormat::RGBA16Float: return WGPUTextureFormat_RGBA16Float;
        default:
            Logger::Instance().Log(LogLevel::Error, "WebGPUSwapChain", 
                                  "Unsupported swap chain format", PERS_SOURCE_LOC);
            return WGPUTextureFormat_BGRA8Unorm; // Default fallback
    }
}

TextureFormat WebGPUSwapChain::convertFromWGPUFormat(WGPUTextureFormat format) {
    switch (format) {
        case WGPUTextureFormat_BGRA8Unorm: return TextureFormat::BGRA8Unorm;
        case WGPUTextureFormat_BGRA8UnormSrgb: return TextureFormat::BGRA8UnormSrgb;
        case WGPUTextureFormat_RGBA8Unorm: return TextureFormat::RGBA8Unorm;
        case WGPUTextureFormat_RGBA8UnormSrgb: return TextureFormat::RGBA8UnormSrgb;
        case WGPUTextureFormat_RGBA16Float: return TextureFormat::RGBA16Float;
        default: return TextureFormat::Undefined;
    }
}

WGPUPresentMode WebGPUSwapChain::convertToWGPUPresentMode(PresentMode mode) {
    switch (mode) {
        case PresentMode::Fifo: return WGPUPresentMode_Fifo;
        case PresentMode::Immediate: return WGPUPresentMode_Immediate;
        case PresentMode::Mailbox: return WGPUPresentMode_Mailbox;
        case PresentMode::FifoRelaxed: return WGPUPresentMode_FifoRelaxed;
        default: return WGPUPresentMode_Fifo;
    }
}

PresentMode WebGPUSwapChain::convertFromWGPUPresentMode(WGPUPresentMode mode) {
    switch (mode) {
        case WGPUPresentMode_Fifo: return PresentMode::Fifo;
        case WGPUPresentMode_Immediate: return PresentMode::Immediate;
        case WGPUPresentMode_Mailbox: return PresentMode::Mailbox;
        case WGPUPresentMode_FifoRelaxed: return PresentMode::FifoRelaxed;
        default: return PresentMode::Fifo;
    }
}

WGPUCompositeAlphaMode WebGPUSwapChain::convertToWGPUAlphaMode(CompositeAlphaMode mode) {
    switch (mode) {
        case CompositeAlphaMode::Auto: return WGPUCompositeAlphaMode_Auto;
        case CompositeAlphaMode::Opaque: return WGPUCompositeAlphaMode_Opaque;
        case CompositeAlphaMode::PreMultiplied: return WGPUCompositeAlphaMode_Premultiplied;
        case CompositeAlphaMode::PostMultiplied: return WGPUCompositeAlphaMode_Unpremultiplied;
        case CompositeAlphaMode::Inherit: return WGPUCompositeAlphaMode_Inherit;
        default: return WGPUCompositeAlphaMode_Auto;
    }
}

CompositeAlphaMode WebGPUSwapChain::convertFromWGPUAlphaMode(WGPUCompositeAlphaMode mode) {
    switch (mode) {
        case WGPUCompositeAlphaMode_Auto: return CompositeAlphaMode::Auto;
        case WGPUCompositeAlphaMode_Opaque: return CompositeAlphaMode::Opaque;
        case WGPUCompositeAlphaMode_Premultiplied: return CompositeAlphaMode::PreMultiplied;
        case WGPUCompositeAlphaMode_Unpremultiplied: return CompositeAlphaMode::PostMultiplied;
        case WGPUCompositeAlphaMode_Inherit: return CompositeAlphaMode::Inherit;
        default: return CompositeAlphaMode::Auto;
    }
}

} // namespace pers