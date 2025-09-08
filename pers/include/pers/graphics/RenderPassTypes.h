#pragma once

#include <memory>
#include <string>
#include <vector>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Forward declarations
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

} // namespace pers