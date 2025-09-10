#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <string>
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Note: SurfaceCapabilities is defined in SwapChainTypes.h (included via ISwapChain.h)

/**
 * @brief Represents the negotiation result for swap chain configuration
 */
struct SwapChainNegotiationResult {
    bool formatSupported = false;
    bool presentModeSupported = false;
    bool alphaModeSupported = false;
    
    TextureFormat negotiatedFormat = TextureFormat::Undefined;
    PresentMode negotiatedPresentMode = PresentMode::Fifo;
    CompositeAlphaMode negotiatedAlphaMode = CompositeAlphaMode::Opaque;
    uint32_t negotiatedBufferCount = 2;
    
    std::vector<TextureFormat> availableFormats;
    std::vector<PresentMode> availablePresentModes;
    std::vector<CompositeAlphaMode> availableAlphaModes;
    
    std::string failureReason;
};

/**
 * @brief Builder for SwapChain descriptor with capability negotiation
 * 
 * This builder is backend-agnostic and handles negotiation between
 * user preferences and surface capabilities.
 */
class SwapChainDescBuilder {
public:
    SwapChainDescBuilder();
    ~SwapChainDescBuilder();
    
    /**
     * @brief Set the surface dimensions
     */
    SwapChainDescBuilder& withDimensions(uint32_t width, uint32_t height);
    
    /**
     * @brief Set preferred texture format
     * @param format Preferred format
     * @param fallbacks Optional ordered list of fallback formats
     */
    SwapChainDescBuilder& withFormat(TextureFormat format, 
                                     const std::vector<TextureFormat>& fallbacks = {});
    
    /**
     * @brief Set preferred present mode
     * @param mode Preferred present mode
     * @param fallbacks Optional ordered list of fallback modes
     */
    SwapChainDescBuilder& withPresentMode(PresentMode mode,
                                          const std::vector<PresentMode>& fallbacks = {});
    
    /**
     * @brief Set preferred alpha compositing mode
     * @param mode Preferred alpha mode
     * @param fallbacks Optional ordered list of fallback modes
     */
    SwapChainDescBuilder& withAlphaMode(CompositeAlphaMode mode,
                                        const std::vector<CompositeAlphaMode>& fallbacks = {});
    
    /**
     * @brief Set debug name for the swap chain
     */
    SwapChainDescBuilder& withDebugName(const std::string& name);
    
    /**
     * @brief Set surface capabilities for automatic negotiation during build
     * @param capabilities The surface capabilities reported by backend
     */
    SwapChainDescBuilder& withSurfaceCapabilities(const SurfaceCapabilities& capabilities);
    
    /**
     * @brief Negotiate configuration with surface capabilities
     * @param capabilities The surface capabilities reported by backend
     * @return Negotiation result with success/failure and selected values
     */
    SwapChainNegotiationResult negotiate(const SurfaceCapabilities& capabilities) const;
    
    /**
     * @brief Build the final descriptor after successful negotiation
     * @param negotiationResult Result from negotiate() call
     * @return Final swap chain descriptor
     */
    SwapChainDesc build(const SwapChainNegotiationResult& negotiationResult) const;
    
    /**
     * @brief Get current configured width
     */
    uint32_t getWidth() const;
    
    /**
     * @brief Get current configured height
     */
    uint32_t getHeight() const;
    
    /**
     * @brief Get negotiation logs from last build operation
     * @return Vector of negotiation log messages (one per item negotiated)
     */
    const std::vector<std::string>& getNegotiationLogs() const;
    
    /**
     * @brief Clear negotiation logs
     */
    void clearNegotiationLogs();
    
    /**
     * @brief Check if surface capabilities have been set
     * @return true if capabilities are available for auto-negotiation
     */
    bool hasCapabilities() const;
    
    // Legacy interface for compatibility
    SwapChainDescBuilder& setSize(uint32_t width, uint32_t height) { return withDimensions(width, height); }
    SwapChainDescBuilder& setFormat(TextureFormat format) { return withFormat(format); }
    SwapChainDescBuilder& setPresentMode(PresentMode mode) { return withPresentMode(mode); }
    SwapChainDescBuilder& setUsage(TextureUsage usage) { _usage = usage; return *this; }
    SwapChainDescBuilder& setDebugName(const std::string& name) { return withDebugName(name); }
    SwapChainDescBuilder& setDesiredBufferCount(uint32_t count) { _desiredBufferCount = count; return *this; }
    SwapChainDesc build() const;  // Simple build without negotiation
    
private:
    uint32_t _width = 0;
    uint32_t _height = 0;
    
    TextureFormat _preferredFormat = TextureFormat::BGRA8Unorm;
    std::vector<TextureFormat> _formatFallbacks;
    
    PresentMode _preferredPresentMode = PresentMode::Fifo;
    std::vector<PresentMode> _presentModeFallbacks;
    
    CompositeAlphaMode _preferredAlphaMode = CompositeAlphaMode::Opaque;
    std::vector<CompositeAlphaMode> _alphaModeFallbacks;
    
    std::string _debugName;
    TextureUsage _usage = TextureUsage::RenderAttachment;
    uint32_t _desiredBufferCount = 3;  // Default to triple buffering (will be negotiated)
    
    // Surface capabilities for auto-negotiation
    std::optional<SurfaceCapabilities> _surfaceCapabilities;
    
    // Negotiation logs for debugging
    mutable std::vector<std::string> _negotiationLogs;
    mutable SwapChainNegotiationResult _lastNegotiationResult;
};

} // namespace pers