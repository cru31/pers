#pragma once

#include <memory>
#include "pers/graphics/buffers/BufferTypes.h"

namespace pers {

class IBuffer;
class IMappableBuffer;

/**
 * Factory interface for creating buffers
 */
class IBufferFactory {
public:
    virtual ~IBufferFactory() = default;
    
    /**
     * Create a non-mappable buffer (GPU-only)
     */
    virtual std::unique_ptr<IBuffer> createBuffer(const BufferDesc& desc) = 0;
    
    /**
     * Create a mappable buffer (CPU-accessible)
     */
    virtual std::unique_ptr<IMappableBuffer> createMappableBuffer(const BufferDesc& desc) = 0;
    
    /**
     * Create a buffer with initial data written synchronously
     * Uses mappedAtCreation internally for efficient initialization
     */
    virtual std::unique_ptr<IBuffer> createBufferWithSyncWrite(
        const BufferDesc& desc,
        const void* initialData,
        size_t dataSize) = 0;
    /**
     * Check if buffer description is supported
     */
    virtual bool isSupported(const BufferDesc& desc) const = 0;
    
    /**
     * Get maximum buffer size supported
     */
    virtual uint64_t getMaxBufferSize() const = 0;
    
    /**
     * Get alignment requirements for buffer usage
     */
    virtual uint64_t getAlignment(BufferUsage usage) const = 0;
};

} // namespace pers