#pragma once

#include <cstdint>

namespace pers {

/**
 * @brief Comparison functions for depth/stencil tests
 */
enum class CompareFunction : uint32_t {
    Undefined = 0,
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief Texture format enumeration
 * Common texture formats supported across major graphics APIs
 * 
 * Platform-specific format support details:
 * 
 * COMPRESSED FORMATS:
 * - BC formats (BC1-BC7): Supported on Desktop (D3D, Vulkan on Windows/Linux, Metal on macOS with extension)
 *   NOT supported on: Mobile, WebGPU (optional feature)
 * 
 * - ETC2/EAC formats: Supported on Mobile (OpenGL ES 3.0+, Vulkan, Metal)
 *   NOT supported on: D3D (any version), WebGPU (optional feature)
 *   Formats not included: ETC2_RGB8, ETC2_RGBA8, ETC2_RGB8_SRGB, ETC2_RGBA8_SRGB,
 *                        EAC_R11, EAC_RG11, ETC2_RGB8A1, ETC2_RGB8A1_SRGB
 * 
 * - ASTC formats: Supported on Mobile (newer devices), Metal, Vulkan (with extension)
 *   NOT supported on: D3D, WebGPU (optional feature)
 *   Formats not included: ASTC_4x4, ASTC_5x4, ASTC_5x5, ASTC_6x5, ASTC_6x6,
 *                        ASTC_8x5, ASTC_8x6, ASTC_8x8, ASTC_10x5, ASTC_10x6,
 *                        ASTC_10x8, ASTC_10x10, ASTC_12x10, ASTC_12x12 (all with UNORM/SRGB variants)
 * 
 * - PVRTC formats: Supported ONLY on iOS/Metal (PowerVR GPUs)
 *   NOT supported on: Any other platform
 *   Formats not included: PVRTC_RGB_2BPP, PVRTC_RGB_4BPP, PVRTC_RGBA_2BPP, PVRTC_RGBA_4BPP
 * 
 * PLATFORM-SPECIFIC UNCOMPRESSED FORMATS:
 * - RGB formats (no alpha): Limited support, RGBA preferred for portability
 *   Formats not included: RGB8, RGB16F, RGB32F, BGR8
 * 
 * - R10G10B10A2 formats: Supported on most modern APIs but not WebGPU core
 *   Formats not included: RGB10A2Unorm, RGB10A2Uint, RG11B10Float
 * 
 * - Packed formats: Platform-specific byte ordering issues
 *   Formats not included: RGB565, RGB5A1, RGBA4444
 * 
 * SPECIAL FORMATS:
 * - YUV formats: Video-specific, require special handling
 *   Not included: NV12, YUY2, etc.
 * 
 * - sRGB formats: Only including most common (RGBA8/BGRA8)
 *   Other sRGB variants exist but less commonly used
 */
enum class TextureFormat {
    // 8-bit formats (universally supported)
    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,
    
    // 16-bit formats (universally supported)
    R16Uint,
    R16Sint,
    R16Float,
    R16Unorm,     // WebGPU supported
    R16Snorm,     // WebGPU supported
    RG8Unorm,
    RG8Snorm,
    RG8Uint,
    RG8Sint,
    
    // 32-bit formats (universally supported)
    R32Uint,
    R32Sint,
    R32Float,
    RG16Uint,
    RG16Sint,
    RG16Float,
    RG16Unorm,       // WebGPU supported
    RG16Snorm,       // WebGPU supported  
    RGBA8Unorm,      // Most common format across all platforms
    RGBA8UnormSrgb,  // sRGB variant
    RGBA8Snorm,
    RGBA8Uint,
    RGBA8Sint,
    BGRA8Unorm,      // Preferred swapchain format on some platforms (macOS/iOS)
    BGRA8UnormSrgb,  // sRGB swapchain
    
    // Packed 32-bit formats (WebGPU supported)
    RGB9E5Ufloat,    // Shared exponent format
    RGB10A2Unorm,    // 10-bit RGB, 2-bit alpha
    RG11B10Ufloat,   // 11-bit RG, 10-bit B unsigned float
    
    // 64-bit formats (universally supported)
    RG32Uint,
    RG32Sint,
    RG32Float,
    RGBA16Uint,
    RGBA16Sint,
    RGBA16Float,     // HDR rendering
    RGBA16Unorm,     // WebGPU supported
    RGBA16Snorm,     // WebGPU supported
    
    // 128-bit formats (universally supported)
    RGBA32Uint,
    RGBA32Sint,
    RGBA32Float,     // High precision compute
    
    // Depth/stencil formats (common across platforms)
    Depth16Unorm,          // Mobile-friendly
    Depth24Plus,           // Portable (maps to D24 or D32 based on platform)
    Depth24PlusStencil8,   // Most common depth-stencil
    Depth32Float,          // High precision depth
    Depth32FloatStencil8,  // Less common, not on all mobile
    Stencil8,              // Stencil only
    
    // BC/DXT compressed formats (Desktop only - Windows, Linux, macOS with extension)
    BC1RGBAUnorm,     // DXT1 - 4bpp, 1-bit alpha
    BC1RGBAUnormSrgb,
    BC2RGBAUnorm,     // DXT3 - 8bpp, explicit alpha
    BC2RGBAUnormSrgb,
    BC3RGBAUnorm,     // DXT5 - 8bpp, interpolated alpha
    BC3RGBAUnormSrgb,
    BC4RUnorm,        // Single channel compression
    BC4RSnorm,
    BC5RGUnorm,       // Two channel compression (normal maps)
    BC5RGSnorm,
    BC6HRGBUfloat,    // HDR compression (unsigned)
    BC6HRGBFloat,     // HDR compression (signed)
    BC7RGBAUnorm,     // High quality compression
    BC7RGBAUnormSrgb,
    
    // Default/undefined
    Undefined
};

/**
 * @brief Vertex format enumeration
 * Defines the format of vertex attributes
 */
enum class VertexFormat {
    // 8-bit formats
    Uint8x2,
    Uint8x4,
    Sint8x2,
    Sint8x4,
    Unorm8x2,
    Unorm8x4,
    Snorm8x2,
    Snorm8x4,
    
    // 16-bit formats
    Uint16x2,
    Uint16x4,
    Sint16x2,
    Sint16x4,
    Unorm16x2,
    Unorm16x4,
    Snorm16x2,
    Snorm16x4,
    Float16x2,
    Float16x4,
    
    // 32-bit formats
    Float32,
    Float32x2,
    Float32x3,
    Float32x4,
    Uint32,
    Uint32x2,
    Uint32x3,
    Uint32x4,
    Sint32,
    Sint32x2,
    Sint32x3,
    Sint32x4
};

/**
 * @brief Index format enumeration
 */
enum class IndexFormat {
    Undefined,
    Uint16,
    Uint32
};

/**
 * @brief Buffer usage flags
 */
enum class BufferUsage : uint32_t {
    None = 0,
    MapRead = 1 << 0,
    MapWrite = 1 << 1,
    CopySrc = 1 << 2,
    CopyDst = 1 << 3,
    Index = 1 << 4,
    Vertex = 1 << 5,
    Uniform = 1 << 6,
    Storage = 1 << 7,
    Indirect = 1 << 8,
    QueryResolve = 1 << 9
};

// Enable bitwise operations for BufferUsage
inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferUsage operator&(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool operator!(BufferUsage usage) {
    return static_cast<uint32_t>(usage) == 0;
}

inline BufferUsage& operator&=(BufferUsage& a, BufferUsage b) {
    return a = a & b;
}

} // namespace pers