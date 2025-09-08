#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/SwapChainTypes.h"
#include "pers/graphics/RenderPassTypes.h"  // For RenderPassDesc, Color

namespace pers {

// Forward declarations
class ITextureView;
class IPhysicalDevice;

// Note: PresentMode, CompositeAlphaMode, and SwapChainDesc are defined in SwapChainTypes.h

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
    
    /**
     * @brief Query surface capabilities
     * @param physicalDevice The physical device to query capabilities from
     * @return Surface capabilities including supported formats, present modes, and dimensions
     */
    virtual SurfaceCapabilities querySurfaceCapabilities(
        const std::shared_ptr<IPhysicalDevice>& physicalDevice) const = 0;
    
    /**
     * @brief Enable or disable automatic depth buffer creation
     * @param enabled True to enable depth buffer (default), false to disable
     * 
     * When enabled, the swap chain automatically creates and manages a depth buffer
     * that matches the swap chain dimensions and is recreated on resize.
     */
    virtual void setDepthBufferEnabled(bool enabled) = 0;
    
    /**
     * @brief Get the depth texture view for the swap chain
     * @return Shared pointer to depth texture view, or nullptr if depth buffer is disabled
     * 
     * The depth buffer is automatically created on first access if enabled.
     * The same depth buffer is reused across frames (cleared each frame).
     */
    virtual std::shared_ptr<ITextureView> getDepthTextureView() = 0;
    
    /**
     * @brief Options for depth stencil attachment configuration
     */
    struct DepthStencilOptions {
        LoadOp depthLoadOp = LoadOp::Clear;
        StoreOp depthStoreOp = StoreOp::Store;
        float depthClearValue = 1.0f;
        bool depthReadOnly = false;
        LoadOp stencilLoadOp = LoadOp::Clear;
        StoreOp stencilStoreOp = StoreOp::Discard;
        uint32_t stencilClearValue = 0;
        bool stencilReadOnly = false;
    };
    
    /**
     * @brief Get a configured depth stencil attachment for this SwapChain
     * 
     * Helper function that returns a properly configured depth attachment
     * if depth buffer is enabled. Returns nullptr if depth is disabled.
     * 
     * @param options Configuration options for the depth stencil attachment
     * @return Configured depth stencil attachment or nullptr
     */
    virtual std::shared_ptr<RenderPassDepthStencilAttachment> getDepthStencilAttachment(
        const DepthStencilOptions& options = {}) = 0;
};

} // namespace pers