#pragma once

#include <memory>
#include <string>
#include <vector>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Forward declarations
class ICommandBuffer;
class IRenderPassEncoder;
class ITextureView;

/**
 * @brief Color for clear operations
 */
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

/**
 * @brief Render pass color attachment
 */
struct RenderPassColorAttachment {
    std::shared_ptr<ITextureView> view;
    std::shared_ptr<ITextureView> resolveTarget = nullptr;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    Color clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};

/**
 * @brief Render pass depth stencil attachment
 */
struct RenderPassDepthStencilAttachment {
    std::shared_ptr<ITextureView> view;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
    float depthClearValue = 1.0f;
    bool depthReadOnly = false;
    LoadOp stencilLoadOp = LoadOp::Clear;
    StoreOp stencilStoreOp = StoreOp::Store;
    uint32_t stencilClearValue = 0;
    bool stencilReadOnly = false;
};

/**
 * @brief Render pass descriptor
 */
struct RenderPassDesc {
    std::vector<RenderPassColorAttachment> colorAttachments;
    std::shared_ptr<RenderPassDepthStencilAttachment> depthStencilAttachment = nullptr;
    std::string label;
};

/**
 * @brief Command encoder interface for recording GPU commands
 * 
 * Encodes GPU commands into a command buffer that can be submitted to a queue.
 * Based on WebGPU CommandEncoder concept but abstracted for cross-platform use.
 */
class ICommandEncoder {
public:
    virtual ~ICommandEncoder() = default;
    
    /**
     * @brief Begin a render pass
     * @param desc Render pass descriptor
     * @return Render pass encoder for recording draw commands
     */
    virtual std::shared_ptr<IRenderPassEncoder> beginRenderPass(const RenderPassDesc& desc) = 0;
    
    /**
     * @brief Finish recording and create command buffer
     * @return Command buffer ready for submission
     */
    virtual std::shared_ptr<ICommandBuffer> finish() = 0;
    
    /**
     * @brief Get native encoder handle for backend-specific operations
     * @return Native encoder handle (WGPUCommandEncoder for WebGPU)
     */
    virtual NativeEncoderHandle getNativeEncoderHandle() const = 0;
};

} // namespace pers