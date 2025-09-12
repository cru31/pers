#pragma once

#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/BufferTypes.h"
#include <memory>

namespace pers {

class IResourceFactory;

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
     * @param desc Buffer description
     * @param initialData Data to write immediately
     * @param dataSize Size of initial data in bytes
     */
    ImmediateDeviceBuffer(const std::shared_ptr<IResourceFactory>& resourceFactory,
                         const BufferDesc& desc,
                         const void* initialData,
                         size_t dataSize);
    
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
    std::unique_ptr<IBuffer> _buffer;
    BufferDesc _desc;
    bool _initialized;
};

} // namespace pers