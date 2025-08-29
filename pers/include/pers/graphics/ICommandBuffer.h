#pragma once

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
     * @brief Get native handle for backend-specific operations
     * @return Native command buffer handle (WGPUCommandBuffer for WebGPU)
     */
    virtual void* getNativeHandle() const = 0;
};

} // namespace pers