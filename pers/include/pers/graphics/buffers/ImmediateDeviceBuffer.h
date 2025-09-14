#pragma once

#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/BufferTypes.h"
#include <memory>

namespace pers {

class IResourceFactory;
class INativeBuffer;

/**
 * @brief Device buffer with immediate data initialization
 * 
 * Creates a device buffer with initial data written synchronously at creation time.
 * Uses mappedAtCreation to write data immediately without staging buffers.
 * Ideal for small, static buffers that need immediate GPU availability.
 */
class ImmediateDeviceBuffer final : public IBuffer {
public:
    /**
     * @brief Create a device buffer with immediate data
     * @param resourceFactory Factory for creating the underlying buffer
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags (Vertex, Index, Uniform, etc.)
     * @param initialData Data to write immediately
     * @param dataSize Size of initial data in bytes
     * @param debugName Optional debug name
     */
    ImmediateDeviceBuffer(const std::shared_ptr<IResourceFactory>& resourceFactory,
                         uint64_t size,
                         BufferUsage usage,
                         const void* initialData,
                         size_t dataSize,
                         const std::string& debugName = "");
    
    ~ImmediateDeviceBuffer() override;
    
    // No copy
    ImmediateDeviceBuffer(const ImmediateDeviceBuffer&) = delete;
    ImmediateDeviceBuffer& operator=(const ImmediateDeviceBuffer&) = delete;
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    const std::string& getDebugName() const override;
    NativeBufferHandle getNativeHandle() const override;
    bool isValid() const override;
    BufferState getState() const override;
    MemoryLocation getMemoryLocation() const override;
    AccessPattern getAccessPattern() const override;
    
private:
    std::shared_ptr<INativeBuffer> _buffer;
    uint64_t _size;
    BufferUsage _usage;
    std::string _debugName;
    bool _initialized;
};

} // namespace pers
