#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace pers {

/**
 * @brief Texture format enumeration
 * 
 * Comprehensive list of texture formats supported by modern graphics APIs.
 * Not all formats may be available on all platforms/devices.
 */
enum class TextureFormat {
    Undefined = 0,
    
    // 8-bit formats
    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,
    
    // 16-bit formats
    R16Uint,
    R16Sint,
    R16Float,
    RG8Unorm,
    RG8Snorm,
    RG8Uint,
    RG8Sint,
    
    // 32-bit formats
    R32Uint,
    R32Sint,
    R32Float,
    RG16Uint,
    RG16Sint,
    RG16Float,
    RGBA8Unorm,
    RGBA8UnormSrgb,
    RGBA8Snorm,
    RGBA8Uint,
    RGBA8Sint,
    BGRA8Unorm,
    BGRA8UnormSrgb,
    RGB10A2Unorm,
    RG11B10Float,
    
    // 64-bit formats
    RG32Uint,
    RG32Sint,
    RG32Float,
    RGBA16Uint,
    RGBA16Sint,
    RGBA16Float,
    
    // 128-bit formats
    RGBA32Uint,
    RGBA32Sint,
    RGBA32Float,
    
    // Depth/stencil formats
    Stencil8,
    Depth16Unorm,
    Depth24Plus,
    Depth24PlusStencil8,
    Depth32Float,
    Depth32FloatStencil8,
    
    // BC compressed formats (desktop)
    BC1RGBAUnorm,
    BC1RGBAUnormSrgb,
    BC2RGBAUnorm,
    BC2RGBAUnormSrgb,
    BC3RGBAUnorm,
    BC3RGBAUnormSrgb,
    BC4RUnorm,
    BC4RSnorm,
    BC5RGUnorm,
    BC5RGSnorm,
    BC6HRGBUfloat,
    BC6HRGBFloat,
    BC7RGBAUnorm,
    BC7RGBAUnormSrgb,
    
    // ETC2 compressed formats (mobile)
    ETC2RGB8Unorm,
    ETC2RGB8UnormSrgb,
    ETC2RGB8A1Unorm,
    ETC2RGB8A1UnormSrgb,
    ETC2RGBA8Unorm,
    ETC2RGBA8UnormSrgb,
    EACR11Unorm,
    EACR11Snorm,
    EACRG11Unorm,
    EACRG11Snorm,
    
    // ASTC compressed formats
    ASTC4x4Unorm,
    ASTC4x4UnormSrgb,
    ASTC5x4Unorm,
    ASTC5x4UnormSrgb,
    ASTC5x5Unorm,
    ASTC5x5UnormSrgb,
    ASTC6x5Unorm,
    ASTC6x5UnormSrgb,
    ASTC6x6Unorm,
    ASTC6x6UnormSrgb,
    ASTC8x5Unorm,
    ASTC8x5UnormSrgb,
    ASTC8x6Unorm,
    ASTC8x6UnormSrgb,
    ASTC8x8Unorm,
    ASTC8x8UnormSrgb,
    ASTC10x5Unorm,
    ASTC10x5UnormSrgb,
    ASTC10x6Unorm,
    ASTC10x6UnormSrgb,
    ASTC10x8Unorm,
    ASTC10x8UnormSrgb,
    ASTC10x10Unorm,
    ASTC10x10UnormSrgb,
    ASTC12x10Unorm,
    ASTC12x10UnormSrgb,
    ASTC12x12Unorm,
    ASTC12x12UnormSrgb,
};

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
};

/**
 * @brief Texture usage flags
 */
enum TextureUsage : uint32_t {
    None            = 0,
    CopySrc         = 0x01,
    CopyDst         = 0x02,
    TextureBinding  = 0x04,
    StorageBinding  = 0x08,
    RenderAttachment = 0x10,
};

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
    TextureUsageFlags usage = TextureUsage::RenderAttachment;
    CompositeAlphaMode alphaMode = CompositeAlphaMode::Opaque;
    
    // Optional
    std::string debugName;
};

} // namespace pers