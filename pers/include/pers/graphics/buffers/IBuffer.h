#pragma once

#include <memory>
#include <string>
#include "pers/graphics/buffers/BufferTypes.h"
#include "pers/graphics/GraphicsTypes.h"

namespace pers {
/**
 * Base buffer interface for all GPU buffers
 */
class IBuffer {
public:
    virtual ~IBuffer() = default;
    
    /**
     * Get buffer size in bytes
     */
    virtual uint64_t getSize() const = 0;
    
    /**
     * Get buffer usage flags
     */
    virtual BufferUsage getUsage() const = 0;
    
    /**
     * Get debug name
     */
    virtual const std::string& getDebugName() const = 0;
    
    /**
     * Get native backend handle (WGPUBuffer, VkBuffer, etc.)
     */
    virtual NativeBufferHandle getNativeHandle() const = 0;
    
    /**
     * Check if buffer is valid and usable
     */
    virtual bool isValid() const = 0;
    
    /**
     * Get current buffer state
     */
    virtual BufferState getState() const = 0;
    
    /**
     * Get memory location
     */
    virtual MemoryLocation getMemoryLocation() const = 0;
    
    /**
     * Get access pattern
     */
    virtual AccessPattern getAccessPattern() const = 0;
};

} // namespace pers