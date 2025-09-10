#pragma once

#include "pers/graphics/RenderPassTypes.h"
#include "pers/graphics/IFramebuffer.h"
#include <vector>
#include <memory>

namespace pers {

/**
 * @brief Configuration for a render pass
 * 
 * Defines HOW to render (load/store operations, clear values) but not WHERE (framebuffer).
 * This configuration can be created once and reused with different framebuffers.
 * Based on Vulkan's VkRenderPass and WebGPU's render pass descriptor pattern.
 */
class RenderPassConfig {
public:
    /**
     * @brief Configuration for a color attachment
     */
    struct ColorConfig {
        LoadOp loadOp = LoadOp::Clear;
        StoreOp storeOp = StoreOp::Store;
        Color clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };
    
    /**
     * @brief Configuration for depth/stencil attachment
     */
    struct DepthStencilConfig {
        LoadOp depthLoadOp = LoadOp::Clear;
        StoreOp depthStoreOp = StoreOp::Store;
        float depthClearValue = 1.0f;
        bool depthReadOnly = false;
        
        LoadOp stencilLoadOp = LoadOp::Clear;
        StoreOp stencilStoreOp = StoreOp::Discard;
        uint32_t stencilClearValue = 0;
        bool stencilReadOnly = false;
    };
    
public:
    RenderPassConfig() = default;
    ~RenderPassConfig() = default;
    
    /**
     * @brief Add a color attachment configuration
     * @param config Color attachment configuration
     */
    void addColorAttachment(const ColorConfig& config);
    
    /**
     * @brief Set depth/stencil configuration
     * @param config Depth/stencil configuration
     */
    void setDepthStencilConfig(const DepthStencilConfig& config);
    
    /**
     * @brief Set debug label
     * @param label Debug label for the render pass
     */
    void setLabel(const std::string& label);
    
    /**
     * @brief Check if depth/stencil is configured
     * @return True if depth/stencil configuration exists
     */
    bool hasDepthStencil() const;
    
    /**
     * @brief Get number of color attachments
     * @return Number of configured color attachments
     */
    uint32_t getColorAttachmentCount() const;
    
    /**
     * @brief Make a RenderPassDesc for a specific framebuffer
     * @param framebuffer The framebuffer to apply this configuration to
     * @return RenderPassDesc with texture views from framebuffer and config from this object
     */
    RenderPassDesc makeDescriptor(const std::shared_ptr<IFramebuffer>& framebuffer) const;
    
private:
    std::vector<ColorConfig> _colorConfigs;
    std::shared_ptr<DepthStencilConfig> _depthStencilConfig;
    std::string _label;
};

} // namespace pers