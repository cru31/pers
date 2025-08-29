#pragma once

#include "pers/graphics/GraphicsTypes.h"

namespace pers {

/**
 * @brief Command buffer interface
 * 
 * Represents a recorded sequence of GPU commands ready for submission.
 */
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;
    
    /**
     * @brief Get native command buffer handle for backend-specific operations
     * @return Native command buffer handle (WGPUCommandBuffer for WebGPU)
     */
    virtual NativeCommandBufferHandle getNativeCommandBufferHandle() const = 0;
};

} // namespace pers