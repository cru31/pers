#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

// Forward declarations
class ITextureView;

/**
 * @brief Present mode for swap chain
 */
enum class PresentMode {
    Fifo = 0,       // VSync enabled (default)
    Immediate = 1,  // No VSync
    Mailbox = 2,    // Triple buffering with VSync
    FifoRelaxed = 3 // Adaptive VSync
};

/**
 * @brief Alpha compositing mode for swap chain
 */
enum class CompositeAlphaMode {
    Auto = 0,        // Automatically select
    Opaque = 1,      // Alpha channel ignored
    PreMultiplied = 2,   // Alpha premultiplied
    PostMultiplied = 3,  // Alpha post-multiplied
    Inherit = 4      // Inherit from system
};

/**
 * @brief Swap chain descriptor
 */
struct SwapChainDesc {
    // Required values - must be set by user
    uint32_t width = 0;
    uint32_t height = 0;
    
    // Values determined by negotiation - set by builder
    TextureFormat format = TextureFormat::BGRA8Unorm;
    PresentMode presentMode = PresentMode::Fifo;
    CompositeAlphaMode alphaMode = CompositeAlphaMode::Opaque;
    TextureUsage usage = TextureUsage::RenderAttachment;
    
    // Optional
    std::string debugName;
};

/**
 * @brief Swap chain interface for frame presentation
 * 
 * Manages a chain of textures for presenting rendered frames to a surface.
 * Based on WebGPU SwapChain/Surface concepts but abstracted for cross-platform use.
 */
class ISwapChain {
public:
    virtual ~ISwapChain() = default;
    
    /**
     * @brief Get the current texture view for rendering
     * 
     * The returned texture view is valid until present() is called.
     * After present(), a new texture view must be acquired.
     * 
     * @return Shared pointer to current texture view or nullptr if failed
     */
    virtual std::shared_ptr<ITextureView> getCurrentTextureView() = 0;
    
    /**
     * @brief Present the current frame to the surface
     * 
     * After calling present(), getCurrentTextureView() must be called
     * again to get the next frame's texture.
     */
    virtual void present() = 0;
    
    /**
     * @brief Resize the swap chain
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    /**
     * @brief Get current swap chain width
     * @return Width in pixels
     */
    virtual uint32_t getWidth() const = 0;
    
    /**
     * @brief Get current swap chain height
     * @return Height in pixels
     */
    virtual uint32_t getHeight() const = 0;
    
    /**
     * @brief Get the present mode
     * @return Current present mode
     */
    virtual PresentMode getPresentMode() const = 0;
    
    /**
     * @brief Get the texture format
     * @return Current texture format
     */
    virtual TextureFormat getFormat() const = 0;
};

} // namespace pers