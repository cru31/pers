#pragma once

#include <memory>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

// Forward declarations
class IRenderPipeline;
class IBindGroup;
class IBuffer;

/**
 * @brief Render pass encoder interface for recording draw commands
 * 
 * Records draw commands within a render pass.
 * Based on WebGPU RenderPassEncoder concept but abstracted for cross-platform use.
 */
class IRenderPassEncoder {
public:
    virtual ~IRenderPassEncoder() = default;
    
    /**
     * @brief Set the render pipeline
     * @param pipeline Render pipeline to use for drawing (compute pipelines not allowed)
     */
    virtual void setPipeline(const std::shared_ptr<IRenderPipeline>& pipeline) = 0;
    
    /**
     * @brief Set a bind group
     * @param index Bind group index
     * @param bindGroup Bind group to set
     */
    virtual void setBindGroup(uint32_t index, const std::shared_ptr<IBindGroup>& bindGroup) = 0;
    
    /**
     * @brief Set vertex buffer
     * @param slot Vertex buffer slot
     * @param buffer Vertex buffer
     * @param offset Offset in buffer
     * @param size Size of data to use
     */
    virtual void setVertexBuffer(uint32_t slot, const std::shared_ptr<IBuffer>& buffer, 
                                 uint64_t offset = 0, uint64_t size = 0) = 0;
    
    /**
     * @brief Set index buffer
     * @param buffer Index buffer
     * @param indexFormat Format of indices
     * @param offset Offset in buffer
     * @param size Size of data to use
     */
    virtual void setIndexBuffer(const std::shared_ptr<IBuffer>& buffer, 
                               IndexFormat indexFormat,
                               uint64_t offset = 0, uint64_t size = 0) = 0;
    
    /**
     * @brief Draw vertices
     * @param vertexCount Number of vertices to draw
     * @param instanceCount Number of instances
     * @param firstVertex First vertex index
     * @param firstInstance First instance index
     */
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                     uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    
    /**
     * @brief Draw indexed vertices
     * @param indexCount Number of indices to draw
     * @param instanceCount Number of instances
     * @param firstIndex First index
     * @param baseVertex Base vertex offset
     * @param firstInstance First instance index
     */
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                           uint32_t firstIndex = 0, int32_t baseVertex = 0,
                           uint32_t firstInstance = 0) = 0;
    
    /**
     * @brief End the render pass
     */
    virtual void end() = 0;
    
    /**
     * @brief Get native render pass encoder handle for backend-specific operations
     * @return Native render pass encoder handle (WGPURenderPassEncoder for WebGPU)
     */
    virtual NativeRenderPassEncoderHandle getNativeRenderPassEncoderHandle() const = 0;
};

} // namespace pers