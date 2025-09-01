#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

/**
 * @brief Buffer descriptor
 */
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    bool mappedAtCreation = false;
    std::string debugName;  // Optional debug name
};

/**
 * @brief Buffer interface for GPU memory
 * 
 * Represents a buffer in GPU memory that can be used for various purposes
 * such as vertex data, index data, uniform data, etc.
 */
class IBuffer {
public:
    virtual ~IBuffer() = default;
    
    /**
     * @brief Get buffer size in bytes
     * @return Buffer size
     */
    virtual uint64_t getSize() const = 0;
    
    /**
     * @brief Get buffer usage flags
     * @return Usage flags
     */
    virtual BufferUsage getUsage() const = 0;
    
    /**
     * @brief Map buffer for CPU access
     * @param offset Byte offset into buffer
     * @param size Number of bytes to map (0 = entire buffer)
     * @return Mapped memory pointer or nullptr on failure
     */
    virtual void* map(uint64_t offset = 0, uint64_t size = 0) = 0;
    
    /**
     * @brief Unmap buffer
     */
    virtual void unmap() = 0;
    
    /**
     * @brief Get native buffer handle for backend-specific operations
     * @return Native buffer handle (WGPUBuffer for WebGPU)
     */
    virtual NativeBufferHandle getNativeBufferHandle() const = 0;
};

} // namespace pers