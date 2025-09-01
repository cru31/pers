#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"
#include <algorithm>

namespace pers {

SwapChainDescBuilder::SwapChainDescBuilder() {
}

SwapChainDescBuilder::~SwapChainDescBuilder() = default;

SwapChainDescBuilder& SwapChainDescBuilder::withDimensions(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withFormat(TextureFormat format,
                                                       const std::vector<TextureFormat>& fallbacks) {
    _preferredFormat = format;
    _formatFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withPresentMode(PresentMode mode,
                                                            const std::vector<PresentMode>& fallbacks) {
    _preferredPresentMode = mode;
    _presentModeFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withAlphaMode(CompositeAlphaMode mode,
                                                          const std::vector<CompositeAlphaMode>& fallbacks) {
    _preferredAlphaMode = mode;
    _alphaModeFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withDebugName(const std::string& name) {
    _debugName = name;
    return *this;
}

template<typename T>
static std::optional<T> selectFromCapabilities(const T& preferred,
                                              const std::vector<T>& fallbacks,
                                              const std::vector<T>& available) {
    // First try preferred
    if (std::find(available.begin(), available.end(), preferred) != available.end()) {
        return preferred;
    }
    
    // Then try fallbacks in order
    for (const auto& fallback : fallbacks) {
        if (std::find(available.begin(), available.end(), fallback) != available.end()) {
            return fallback;
        }
    }
    
    // No match found
    return std::nullopt;
}

SwapChainNegotiationResult SwapChainDescBuilder::negotiate(const SurfaceCapabilities& capabilities) const {
    SwapChainNegotiationResult result;
    
    // Store available options for transparency
    result.availableFormats = capabilities.formats;
    result.availablePresentModes = capabilities.presentModes;
    result.availableAlphaModes = capabilities.alphaModes;
    
    // Validate dimensions
    if (_width == 0 || _height == 0) {
        result.failureReason = "Invalid dimensions: width and height must be non-zero";
        return result;
    }
    
    if (_width < capabilities.minWidth || _width > capabilities.maxWidth ||
        _height < capabilities.minHeight || _height > capabilities.maxHeight) {
        result.failureReason = "Dimensions out of supported range";
        return result;
    }
    
    // Negotiate format
    auto selectedFormat = selectFromCapabilities(
        _preferredFormat, 
        _formatFallbacks,
        capabilities.formats
    );
    
    if (!selectedFormat.has_value()) {
        result.formatSupported = false;
        result.failureReason = "No supported texture format found";
        
        Logger::Instance().Log(LogLevel::Warning, "SwapChainDescBuilder",
                              "SwapChain format negotiation failed - Preferred format not available, no fallback formats matched",
                              PERS_SOURCE_LOC);
        
        return result;
    }
    
    result.formatSupported = true;
    result.negotiatedFormat = selectedFormat.value();
    
    // Negotiate present mode
    auto selectedPresentMode = selectFromCapabilities(
        _preferredPresentMode,
        _presentModeFallbacks,
        capabilities.presentModes
    );
    
    if (!selectedPresentMode.has_value()) {
        result.presentModeSupported = false;
        result.failureReason = "No supported present mode found";
        
        Logger::Instance().Log(LogLevel::Warning, "SwapChainDescBuilder",
                              "SwapChain present mode negotiation failed",
                              PERS_SOURCE_LOC);
        
        return result;
    }
    
    result.presentModeSupported = true;
    result.negotiatedPresentMode = selectedPresentMode.value();
    
    // Negotiate alpha mode
    auto selectedAlphaMode = selectFromCapabilities(
        _preferredAlphaMode,
        _alphaModeFallbacks,
        capabilities.alphaModes
    );
    
    if (!selectedAlphaMode.has_value()) {
        result.alphaModeSupported = false;
        result.failureReason = "No supported alpha mode found";
        
        Logger::Instance().Log(LogLevel::Warning, "SwapChainDescBuilder",
                              "SwapChain alpha mode negotiation failed",
                              PERS_SOURCE_LOC);
        
        return result;
    }
    
    result.alphaModeSupported = true;
    result.negotiatedAlphaMode = selectedAlphaMode.value();
    
    // Log successful negotiation
    Logger::Instance().Log(LogLevel::Info, "SwapChainDescBuilder",
                          "SwapChain negotiation successful",
                          PERS_SOURCE_LOC);
    
    return result;
}

SwapChainDesc SwapChainDescBuilder::build(const SwapChainNegotiationResult& negotiationResult) const {
    SwapChainDesc desc;
    
    if (!negotiationResult.formatSupported || 
        !negotiationResult.presentModeSupported || 
        !negotiationResult.alphaModeSupported) {
        Logger::Instance().Log(LogLevel::Error, "SwapChainDescBuilder",
                              "Cannot build SwapChainDesc from failed negotiation",
                              PERS_SOURCE_LOC);
        // Return default-initialized desc
        return desc;
    }
    
    desc.width = _width;
    desc.height = _height;
    desc.format = negotiationResult.negotiatedFormat;
    desc.presentMode = negotiationResult.negotiatedPresentMode;
    desc.alphaMode = negotiationResult.negotiatedAlphaMode;
    desc.debugName = _debugName;
    
    return desc;
}

uint32_t SwapChainDescBuilder::getWidth() const {
    return _width;
}

uint32_t SwapChainDescBuilder::getHeight() const {
    return _height;
}

SwapChainDesc SwapChainDescBuilder::build() const {
    SwapChainDesc desc;
    desc.width = _width;
    desc.height = _height;
    desc.format = _preferredFormat;
    desc.presentMode = _preferredPresentMode;
    desc.alphaMode = _preferredAlphaMode;
    desc.usage = _usage;
    desc.debugName = _debugName;
    return desc;
}

} // namespace pers