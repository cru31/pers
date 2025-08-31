#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <string>
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

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
    
    std::vector<TextureFormat> availableFormats;
    std::vector<PresentMode> availablePresentModes;
    std::vector<CompositeAlphaMode> availableAlphaModes;
    
    std::string failureReason;
};

/**
 * @brief Surface capabilities that backends report
 */
struct SurfaceCapabilities {
    std::vector<TextureFormat> formats;
    std::vector<PresentMode> presentModes;
    std::vector<CompositeAlphaMode> alphaModes;
    
    uint32_t minImageCount = 2;
    uint32_t maxImageCount = 3;
    
    uint32_t currentWidth = 0;
    uint32_t currentHeight = 0;
    uint32_t minWidth = 1;
    uint32_t maxWidth = 8192;
    uint32_t minHeight = 1;
    uint32_t maxHeight = 8192;
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
    
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace pers