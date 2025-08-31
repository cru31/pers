#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"
#include <algorithm>

namespace pers {

class SwapChainDescBuilder::Impl {
public:
    uint32_t width = 0;
    uint32_t height = 0;
    
    TextureFormat preferredFormat = TextureFormat::BGRA8Unorm;
    std::vector<TextureFormat> formatFallbacks;
    
    PresentMode preferredPresentMode = PresentMode::Fifo;
    std::vector<PresentMode> presentModeFallbacks;
    
    CompositeAlphaMode preferredAlphaMode = CompositeAlphaMode::Opaque;
    std::vector<CompositeAlphaMode> alphaModeFallbacks;
    
    std::string debugName;
    
    template<typename T>
    std::optional<T> selectFromCapabilities(const T& preferred,
                                           const std::vector<T>& fallbacks,
                                           const std::vector<T>& available) const {
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
};

SwapChainDescBuilder::SwapChainDescBuilder() 
    : _impl(std::make_unique<Impl>()) {
}

SwapChainDescBuilder::~SwapChainDescBuilder() = default;

SwapChainDescBuilder& SwapChainDescBuilder::withDimensions(uint32_t width, uint32_t height) {
    _impl->width = width;
    _impl->height = height;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withFormat(TextureFormat format,
                                                       const std::vector<TextureFormat>& fallbacks) {
    _impl->preferredFormat = format;
    _impl->formatFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withPresentMode(PresentMode mode,
                                                            const std::vector<PresentMode>& fallbacks) {
    _impl->preferredPresentMode = mode;
    _impl->presentModeFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withAlphaMode(CompositeAlphaMode mode,
                                                          const std::vector<CompositeAlphaMode>& fallbacks) {
    _impl->preferredAlphaMode = mode;
    _impl->alphaModeFallbacks = fallbacks;
    return *this;
}

SwapChainDescBuilder& SwapChainDescBuilder::withDebugName(const std::string& name) {
    _impl->debugName = name;
    return *this;
}

SwapChainNegotiationResult SwapChainDescBuilder::negotiate(const SurfaceCapabilities& capabilities) const {
    SwapChainNegotiationResult result;
    
    // Store available options for transparency
    result.availableFormats = capabilities.formats;
    result.availablePresentModes = capabilities.presentModes;
    result.availableAlphaModes = capabilities.alphaModes;
    
    // Validate dimensions
    if (_impl->width == 0 || _impl->height == 0) {
        result.failureReason = "Invalid dimensions: width and height must be non-zero";
        return result;
    }
    
    if (_impl->width < capabilities.minWidth || _impl->width > capabilities.maxWidth ||
        _impl->height < capabilities.minHeight || _impl->height > capabilities.maxHeight) {
        result.failureReason = "Dimensions out of supported range";
        return result;
    }
    
    // Negotiate format
    auto selectedFormat = _impl->selectFromCapabilities(
        _impl->preferredFormat, 
        _impl->formatFallbacks,
        capabilities.formats
    );
    
    if (!selectedFormat.has_value()) {
        result.formatSupported = false;
        result.failureReason = "No supported texture format found";
        
        // Log detailed information for debugging
        Logger::Instance().Log(LogLevel::Warning, "SwapChainDescBuilder",
                              "SwapChain format negotiation failed - Preferred format not available, no fallback formats matched",
                              PERS_SOURCE_LOC);
        
        return result;
    }
    
    result.formatSupported = true;
    result.negotiatedFormat = selectedFormat.value();
    
    // Negotiate present mode
    auto selectedPresentMode = _impl->selectFromCapabilities(
        _impl->preferredPresentMode,
        _impl->presentModeFallbacks,
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
    auto selectedAlphaMode = _impl->selectFromCapabilities(
        _impl->preferredAlphaMode,
        _impl->alphaModeFallbacks,
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
    
    desc.width = _impl->width;
    desc.height = _impl->height;
    desc.format = negotiationResult.negotiatedFormat;
    desc.presentMode = negotiationResult.negotiatedPresentMode;
    desc.alphaMode = negotiationResult.negotiatedAlphaMode;
    desc.debugName = _impl->debugName;
    
    return desc;
}

uint32_t SwapChainDescBuilder::getWidth() const {
    return _impl->width;
}

uint32_t SwapChainDescBuilder::getHeight() const {
    return _impl->height;
}

} // namespace pers