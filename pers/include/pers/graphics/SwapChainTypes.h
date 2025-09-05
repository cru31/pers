#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Note: TextureFormat is defined in GraphicsFormats.h
// Note: TextureUsage is defined in GraphicsTypes.h

/**
 * @brief Presentation mode for SwapChain
 */
enum class PresentMode {
    Fifo,        // VSync - guaranteed to be supported
    Immediate,   // No VSync - minimal latency
    Mailbox,     // Triple buffering
    FifoRelaxed, // Adaptive VSync
};

/**
 * @brief Alpha compositing mode
 */
enum class CompositeAlphaMode {
    Auto,           // Automatically select
    Opaque,         // Alpha channel ignored
    Premultiplied,  // Alpha premultiplied
    Unpremultiplied,// Alpha not premultiplied  
    Inherit,        // Inherit from system
    PostMultiplied, // Alpha post-multiplied
    PreMultiplied = Premultiplied, // Alias for consistency
};

// For backward compatibility - TextureUsage is now an enum class in GraphicsTypes.h
using TextureUsageFlags = uint32_t;

/**
 * @brief Surface capabilities returned by device
 */
struct SurfaceCapabilities {
    std::vector<TextureFormat> formats;
    std::vector<PresentMode> presentModes;
    std::vector<CompositeAlphaMode> alphaModes;
    TextureUsageFlags usages;
    uint32_t minImageCount;
    uint32_t maxImageCount;
    uint32_t currentWidth;
    uint32_t currentHeight;
    uint32_t minWidth;
    uint32_t minHeight;
    uint32_t maxWidth;
    uint32_t maxHeight;
};

/**
 * @brief SwapChain descriptor
 */
struct SwapChainDesc {
    // Required values
    uint32_t width = 0;
    uint32_t height = 0;
    
    // Values determined by negotiation
    TextureFormat format = TextureFormat::Undefined;
    PresentMode presentMode = PresentMode::Fifo;
    TextureUsageFlags usage = static_cast<TextureUsageFlags>(TextureUsage::RenderAttachment);
    CompositeAlphaMode alphaMode = CompositeAlphaMode::Opaque;
    
    // Optional
    std::string debugName;
};

} // namespace pers