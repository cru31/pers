#pragma once

#include <memory>
#include <cstdint>
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

// Forward declarations
class ITextureView;
class IFramebuffer;

/**
 * @brief Base interface for all framebuffers
 * 
 * A framebuffer represents a collection of render targets (color and depth/stencil)
 * that can be rendered to. This abstraction unifies surface (SwapChain) and 
 * offscreen render targets under a common interface.
 * 
 * Based on Vulkan's VkFramebuffer and WebGPU's render pass attachments concept.
 */
class IFramebuffer {
public:
    virtual ~IFramebuffer() = default;
    
    /**
     * @brief Get a color attachment view by index
     * @param index The color attachment index (0 for single target, 0-7 for MRT)
     * @return Shared pointer to texture view, or nullptr if index is invalid
     */
    virtual std::shared_ptr<ITextureView> getColorAttachment(uint32_t index = 0) const = 0;
    
    /**
     * @brief Get the depth/stencil attachment view
     * @return Shared pointer to depth texture view, or nullptr if no depth attachment
     */
    virtual std::shared_ptr<ITextureView> getDepthStencilAttachment() const = 0;
    
    /**
     * @brief Get the framebuffer width
     * @return Width in pixels
     */
    virtual uint32_t getWidth() const = 0;
    
    /**
     * @brief Get the framebuffer height  
     * @return Height in pixels
     */
    virtual uint32_t getHeight() const = 0;
    
    /**
     * @brief Get the sample count for MSAA
     * @return Sample count (1 for no MSAA, 2/4/8/16 for MSAA)
     */
    virtual uint32_t getSampleCount() const = 0;
    
    /**
     * @brief Get the color attachment format
     * @param index The color attachment index
     * @return Texture format, or Undefined if index is invalid
     */
    virtual TextureFormat getColorFormat(uint32_t index = 0) const = 0;
    
    /**
     * @brief Get the depth/stencil attachment format
     * @return Texture format, or Undefined if no depth attachment
     */
    virtual TextureFormat getDepthFormat() const = 0;
    
    /**
     * @brief Get the number of color attachments
     * @return Number of color attachments (1 for single, up to 8 for MRT)
     */
    virtual uint32_t getColorAttachmentCount() const = 0;
    
    /**
     * @brief Check if this framebuffer has a depth/stencil attachment
     * @return True if depth/stencil attachment exists
     */
    virtual bool hasDepthStencilAttachment() const = 0;
};

/**
 * @brief Interface for resizable framebuffers
 * 
 * Extends IFramebuffer with resize capability. Not all framebuffers
 * need to be resizable (e.g., fixed-size render targets).
 */
class IResizableFramebuffer : public IFramebuffer {
public:
    /**
     * @brief Resize the framebuffer
     * @param width New width in pixels
     * @param height New height in pixels
     * @return True if resize succeeded, false otherwise
     */
    virtual bool resize(uint32_t width, uint32_t height) = 0;
};


} // namespace pers