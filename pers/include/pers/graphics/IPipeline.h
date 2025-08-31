#pragma once

#include "pers/graphics/GraphicsTypes.h"

namespace pers {

/**
 * @brief Pipeline interface
 * 
 * Represents a render or compute pipeline.
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;
    
    /**
     * @brief Get native pipeline handle for backend-specific operations
     * @return Native pipeline handle (WGPURenderPipeline or WGPUComputePipeline for WebGPU)
     */
    virtual NativePipelineHandle getNativePipelineHandle() const = 0;
};

} // namespace pers