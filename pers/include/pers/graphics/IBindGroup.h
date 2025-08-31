#pragma once

#include "pers/graphics/GraphicsTypes.h"

namespace pers {

/**
 * @brief Bind group interface
 * 
 * Represents a collection of resources that are bound together.
 */
class IBindGroup {
public:
    virtual ~IBindGroup() = default;
    
    /**
     * @brief Get native bind group handle for backend-specific operations
     * @return Native bind group handle (WGPUBindGroup for WebGPU)
     */
    virtual NativeBindGroupHandle getNativeBindGroupHandle() const = 0;
};

} // namespace pers