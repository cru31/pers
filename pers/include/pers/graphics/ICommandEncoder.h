#pragma once

#include <memory>
#include "pers/graphics/RenderPassTypes.h"

namespace pers {

// Forward declarations
class ICommandBuffer;
class IRenderPassEncoder;

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