#pragma once

#include <memory>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

// TextureUsage and TextureDimension are defined in GraphicsTypes.h

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool operator!(TextureUsage usage) {
    return static_cast<uint32_t>(usage) == 0;
}

/**
 * @brief Texture descriptor
 */
struct TextureDesc {
    TextureDimension dimension = TextureDimension::D2;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depthOrArrayLayers = 1;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUsage usage = TextureUsage::None;
};

/**
 * @brief Texture interface for GPU textures
 * 
 * Represents a texture in GPU memory that can be used for various purposes
 * such as rendering targets, texture sampling, storage, etc.
 */
class ITexture {
public:
    virtual ~ITexture() = default;
    
    /**
     * @brief Get texture width
     * @return Width in pixels
     */
    virtual uint32_t getWidth() const = 0;
    
    /**
     * @brief Get texture height
     * @return Height in pixels
     */
    virtual uint32_t getHeight() const = 0;
    
    /**
     * @brief Get texture depth or array layer count
     * @return Depth for 3D textures, array layers for 2D arrays
     */
    virtual uint32_t getDepthOrArrayLayers() const = 0;
    
    /**
     * @brief Get mip level count
     * @return Number of mip levels
     */
    virtual uint32_t getMipLevelCount() const = 0;
    
    /**
     * @brief Get sample count
     * @return Number of samples (1 for non-multisampled)
     */
    virtual uint32_t getSampleCount() const = 0;
    
    /**
     * @brief Get texture dimension
     * @return Texture dimension
     */
    virtual TextureDimension getDimension() const = 0;
    
    /**
     * @brief Get texture format
     * @return Texture format
     */
    virtual TextureFormat getFormat() const = 0;
    
    /**
     * @brief Get texture usage flags
     * @return Usage flags
     */
    virtual TextureUsage getUsage() const = 0;
    
    /**
     * @brief Get native texture handle for backend-specific operations
     * @return Native texture handle (WGPUTexture for WebGPU)
     */
    virtual NativeTextureHandle getNativeTextureHandle() const = 0;
};

} // namespace pers