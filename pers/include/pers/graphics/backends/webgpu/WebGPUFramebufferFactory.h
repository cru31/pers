#pragma once

#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUOffscreenFramebuffer.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

class WebGPULogicalDevice;

/**
 * @brief Factory class for creating WebGPU framebuffers
 * 
 * Provides static factory methods for creating different types of framebuffers.
 * All parameters are externally configurable, no hardcoding.
 */
class WebGPUFramebufferFactory final {
public:
    /**
     * @brief Create a surface framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param surface The WebGPU surface
     * @param width Initial width
     * @param height Initial height
     * @param format Surface format (externally configurable)
     * @return Shared pointer to ISurfaceFramebuffer
     */
    static std::shared_ptr<ISurfaceFramebuffer> createSurfaceFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        WGPUSurface surface,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::BGRA8Unorm);
    
    /**
     * @brief Create an offscreen framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param config Configuration for the framebuffer
     * @return Shared pointer to IResizableFramebuffer
     */
    static std::shared_ptr<IResizableFramebuffer> createOffscreenFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        const OffscreenFramebufferConfig& config);
    
    /**
     * @brief Create a depth-only framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param width Width of the depth texture
     * @param height Height of the depth texture
     * @param format Depth format (externally configurable)
     * @param sampleCount Sample count for MSAA
     * @param usage Texture usage flags
     * @return Shared pointer to IResizableFramebuffer
     */
    static std::shared_ptr<IResizableFramebuffer> createDepthOnlyFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::Depth24Plus,
        uint32_t sampleCount = 1,
        TextureUsage usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding);
    
    /**
     * @brief Create an MSAA framebuffer with resolve target
     * @param device The logical device (passed as const& per coding standards)
     * @param width Width
     * @param height Height
     * @param colorFormat Color format
     * @param depthFormat Depth format (Undefined for no depth)
     * @param sampleCount MSAA sample count (2, 4, or 8)
     * @return Shared pointer to IResizableFramebuffer
     * 
     * Creates an MSAA framebuffer suitable for resolving to a surface or
     * another single-sample framebuffer.
     */
    static std::shared_ptr<IResizableFramebuffer> createMSAAFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        uint32_t width,
        uint32_t height,
        TextureFormat colorFormat,
        TextureFormat depthFormat = TextureFormat::Undefined,
        uint32_t sampleCount = 4);
    
    /**
     * @brief Create a shadow map framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param size Shadow map size (width and height)
     * @param format Depth format for shadow map
     * @return Shared pointer to IResizableFramebuffer
     * 
     * Convenience method for creating a square depth-only framebuffer
     * suitable for shadow mapping.
     */
    static std::shared_ptr<IResizableFramebuffer> createShadowMapFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        uint32_t size,
        TextureFormat format = TextureFormat::Depth32Float);
    
    /**
     * @brief Create a multi-render-target (MRT) framebuffer
     * @param device The logical device (passed as const& per coding standards)
     * @param width Width
     * @param height Height
     * @param colorFormats Array of color formats for each attachment
     * @param colorCount Number of color attachments
     * @param depthFormat Depth format (Undefined for no depth)
     * @return Shared pointer to IResizableFramebuffer
     * 
     * Creates a framebuffer with multiple color attachments for
     * deferred rendering or other MRT techniques.
     */
    static std::shared_ptr<IResizableFramebuffer> createMRTFramebuffer(
        const std::shared_ptr<WebGPULogicalDevice>& device,
        uint32_t width,
        uint32_t height,
        const TextureFormat* colorFormats,
        uint32_t colorCount,
        TextureFormat depthFormat = TextureFormat::Depth24Plus);
    
private:
    // Static factory class, no instantiation
    WebGPUFramebufferFactory() = delete;
    ~WebGPUFramebufferFactory() = delete;
};

} // namespace pers