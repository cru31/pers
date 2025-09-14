#pragma once

#include <memory>
#include <string>
#include "pers/graphics/buffers/BufferTypes.h"
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

/**
 * Interface for native backend buffer implementations
 *
 * This interface is for internal use by graphics backend implementations
 * (WebGPU, Vulkan, D3D12, Metal, etc). It provides the base contract
 * that all native buffer implementations must follow.
 *
 * User-facing buffer classes (DeviceBuffer, ImmediateStagingBuffer, etc)
 * should NOT inherit from this interface directly.
 */
class INativeBuffer{
public:
    virtual ~INativeBuffer() = default;
    
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