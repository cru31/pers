#pragma once

#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"
#include <memory>

namespace pers {

/**
 * @brief Interface for texture view
 * Represents a view into a texture that can be used for rendering
 */
class ITextureView {
public:
    virtual ~ITextureView() = default;
    
    /**
     * @brief Get native texture view handle
     * @return Native handle to the texture view
     */
    virtual NativeTextureViewHandle getNativeTextureViewHandle() const = 0;
    
    /**
     * @brief Get the dimensions of the texture view
     * @param width Output width of the texture
     * @param height Output height of the texture
     */
    virtual void getDimensions(uint32_t& width, uint32_t& height) const = 0;
    
    /**
     * @brief Get the format of the texture view
     * @return Texture format enum
     */
    virtual TextureFormat getFormat() const = 0;
    
    /**
     * @brief Check if this texture view is from a SwapChain
     * @return True if this is a SwapChain texture view
     */
    virtual bool isSwapChainTexture() const { return false; }
};

} // namespace pers