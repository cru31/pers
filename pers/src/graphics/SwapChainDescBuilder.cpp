#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/graphics/GraphicsEnumStrings.h"
#include "pers/utils/Logger.h"
#include <algorithm>
#include <sstream>

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

SwapChainDescBuilder& SwapChainDescBuilder::withSurfaceCapabilities(const SurfaceCapabilities& capabilities) {
    _surfaceCapabilities = capabilities;
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
    
    // Clear previous logs
    _negotiationLogs.clear();
    
    // Store available options for transparency
    result.availableFormats = capabilities.formats;
    result.availablePresentModes = capabilities.presentModes;
    result.availableAlphaModes = capabilities.alphaModes;
    
    // Validate dimensions
    if (_width == 0 || _height == 0) {
        result.failureReason = "Invalid dimensions: width and height must be non-zero";
        _negotiationLogs.push_back("[FAILED] Dimensions: Invalid - width and height must be non-zero");
        return result;
    }
    
    if (_width < capabilities.minWidth || _width > capabilities.maxWidth ||
        _height < capabilities.minHeight || _height > capabilities.maxHeight) {
        std::stringstream ss;
        ss << "[FAILED] Dimensions: Requested " << _width << "x" << _height 
           << " is out of supported range (" << capabilities.minWidth << "x" << capabilities.minHeight
           << " to " << capabilities.maxWidth << "x" << capabilities.maxHeight << ")";
        result.failureReason = "Dimensions out of supported range";
        _negotiationLogs.push_back(ss.str());
        return result;
    }
    
    _negotiationLogs.push_back("[OK] Dimensions: " + std::to_string(_width) + "x" + std::to_string(_height));
    
    // Negotiate format
    auto selectedFormat = selectFromCapabilities(
        _preferredFormat, 
        _formatFallbacks,
        capabilities.formats
    );
    
    if (!selectedFormat.has_value()) {
        result.formatSupported = false;
        result.failureReason = "No supported texture format found";
        
        std::stringstream ss;
        ss << "[FAILED] Format: Preferred " << GraphicsEnumStrings::toString(_preferredFormat) << " not supported. ";
        ss << "Available formats: ";
        for (size_t i = 0; i < capabilities.formats.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << GraphicsEnumStrings::toString(capabilities.formats[i]);
        }
        _negotiationLogs.push_back(ss.str());
        
        LOG_WARNING("SwapChainDescBuilder",
                              "SwapChain format negotiation failed - Preferred format not available, no fallback formats matched");
        
        return result;
    }
    
    result.formatSupported = true;
    result.negotiatedFormat = selectedFormat.value();
    
    // Log format negotiation
    if (selectedFormat.value() == _preferredFormat) {
        _negotiationLogs.push_back("[OK] Format: Using preferred " + GraphicsEnumStrings::toString(_preferredFormat));
    } else {
        _negotiationLogs.push_back("[FALLBACK] Format: Preferred " + GraphicsEnumStrings::toString(_preferredFormat) + 
                                  " not available, using fallback " + GraphicsEnumStrings::toString(selectedFormat.value()));
    }
    
    // Negotiate present mode
    auto selectedPresentMode = selectFromCapabilities(
        _preferredPresentMode,
        _presentModeFallbacks,
        capabilities.presentModes
    );
    
    if (!selectedPresentMode.has_value()) {
        result.presentModeSupported = false;
        result.failureReason = "No supported present mode found";
        
        std::stringstream ss;
        ss << "[FAILED] PresentMode: Preferred " << GraphicsEnumStrings::toString(_preferredPresentMode) << " not supported. ";
        ss << "Available modes: ";
        for (size_t i = 0; i < capabilities.presentModes.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << GraphicsEnumStrings::toString(capabilities.presentModes[i]);
        }
        _negotiationLogs.push_back(ss.str());
        
        LOG_WARNING("SwapChainDescBuilder",
                              "SwapChain present mode negotiation failed");
        
        return result;
    }
    
    result.presentModeSupported = true;
    result.negotiatedPresentMode = selectedPresentMode.value();
    
    // Log present mode negotiation
    if (selectedPresentMode.value() == _preferredPresentMode) {
        _negotiationLogs.push_back("[OK] PresentMode: Using preferred " + GraphicsEnumStrings::toString(_preferredPresentMode));
    } else {
        _negotiationLogs.push_back("[FALLBACK] PresentMode: Preferred " + GraphicsEnumStrings::toString(_preferredPresentMode) + 
                                  " not available, using fallback " + GraphicsEnumStrings::toString(selectedPresentMode.value()));
    }
    
    // Negotiate alpha mode
    auto selectedAlphaMode = selectFromCapabilities(
        _preferredAlphaMode,
        _alphaModeFallbacks,
        capabilities.alphaModes
    );
    
    if (!selectedAlphaMode.has_value()) {
        result.alphaModeSupported = false;
        result.failureReason = "No supported alpha mode found";
        
        std::stringstream ss;
        ss << "[FAILED] AlphaMode: Preferred " << GraphicsEnumStrings::toString(_preferredAlphaMode) << " not supported. ";
        ss << "Available modes: ";
        for (size_t i = 0; i < capabilities.alphaModes.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << GraphicsEnumStrings::toString(capabilities.alphaModes[i]);
        }
        _negotiationLogs.push_back(ss.str());
        
        LOG_WARNING("SwapChainDescBuilder",
                              "SwapChain alpha mode negotiation failed");
        
        return result;
    }
    
    result.alphaModeSupported = true;
    result.negotiatedAlphaMode = selectedAlphaMode.value();
    
    // Log alpha mode negotiation
    if (selectedAlphaMode.value() == _preferredAlphaMode) {
        _negotiationLogs.push_back("[OK] AlphaMode: Using preferred " + GraphicsEnumStrings::toString(_preferredAlphaMode));
    } else {
        _negotiationLogs.push_back("[FALLBACK] AlphaMode: Preferred " + GraphicsEnumStrings::toString(_preferredAlphaMode) + 
                                  " not available, using fallback " + GraphicsEnumStrings::toString(selectedAlphaMode.value()));
    }
    
    // Store the result for later use
    _lastNegotiationResult = result;
    
    // Log successful negotiation
    LOG_INFO("SwapChainDescBuilder",
                          "SwapChain negotiation successful");
    
    return result;
}

SwapChainDesc SwapChainDescBuilder::build(const SwapChainNegotiationResult& negotiationResult) const {
    SwapChainDesc desc;
    
    if (!negotiationResult.formatSupported || 
        !negotiationResult.presentModeSupported || 
        !negotiationResult.alphaModeSupported) {
        LOG_ERROR("SwapChainDescBuilder",
                              "Cannot build SwapChainDesc from failed negotiation");
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
    // If surface capabilities are available, auto-negotiate
    if (_surfaceCapabilities.has_value()) {
        SwapChainNegotiationResult result = negotiate(_surfaceCapabilities.value());
        
        if (result.formatSupported && result.presentModeSupported && result.alphaModeSupported) {
            // Build with negotiation result
            return build(result);
        } else {
            // Log failure and return empty descriptor
            LOG_ERROR("SwapChainDescBuilder",
                                 "Auto-negotiation failed: " + result.failureReason);
            return SwapChainDesc();
        }
    }
    
    // No capabilities set, use simple build
    SwapChainDesc desc;
    desc.width = _width;
    desc.height = _height;
    desc.format = _preferredFormat;
    desc.presentMode = _preferredPresentMode;
    desc.alphaMode = _preferredAlphaMode;
    desc.usage = static_cast<TextureUsageFlags>(_usage);
    desc.debugName = _debugName;
    return desc;
}

const std::vector<std::string>& SwapChainDescBuilder::getNegotiationLogs() const {
    return _negotiationLogs;
}

void SwapChainDescBuilder::clearNegotiationLogs() {
    _negotiationLogs.clear();
}

bool SwapChainDescBuilder::hasCapabilities() const {
    return _surfaceCapabilities.has_value();
}

} // namespace pers