#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/SwapChainTypes.h"
#include <memory>

namespace pers {

/**
 * @brief Interface for surface framebuffers
 * 
 * Surface framebuffers represent the final render target that can be presented
 * to the screen. They have an acquire/present lifecycle for managing the
 * swap chain's image availability.
 */
class ISurfaceFramebuffer : public IResizableFramebuffer {
public:
    virtual ~ISurfaceFramebuffer() = default;
    
    /**
     * @brief Create the swap chain with given description
     * @param surface The native surface handle
     * @param desc The swap chain description
     * @param depthFormat The depth/stencil format to use
     * @return True if successful, false otherwise
     */
    virtual bool create(const NativeSurfaceHandle& surface, const SwapChainDesc& desc,
                       TextureFormat depthFormat = TextureFormat::Depth24PlusStencil8) = 0;
    
    /**
     * @brief Destroy the swap chain and release resources
     */
    virtual void destroy() = 0;
    
    /**
     * @brief Acquire the next image for rendering
     * @return True if successful, false otherwise
     * 
     * Must be called before accessing color attachments.
     * After acquire, the framebuffer is ready for rendering.
     */
    virtual bool acquireNextImage() = 0;
    
    /**
     * @brief Present the rendered image to the surface
     * 
     * After present, acquireNextImage must be called again
     * before the next frame can be rendered.
     */
    virtual void present() = 0;
    
    /**
     * @brief Check if the framebuffer is ready for rendering
     * @return True if an image has been acquired and not yet presented
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Set an external depth framebuffer
     * @param depthFramebuffer The depth framebuffer to use
     * 
     * Allows sharing depth buffers between multiple surface framebuffers
     * or using a custom depth buffer configuration.
     */
    virtual void setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer) = 0;
};

} // namespace pers