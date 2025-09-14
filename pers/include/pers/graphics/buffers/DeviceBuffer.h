#pragma once

#include <memory>
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/DeviceBufferUsage.h"

namespace pers {

class ICommandEncoder;
class ILogicalDevice;
class INativeBuffer;

/**
 * GPU-only buffer for maximum performance
 * No CPU access, data upload via staging buffers
 */
class DeviceBuffer final : public IBuffer {
public:
    DeviceBuffer();
    ~DeviceBuffer() override;
    
    /**
     * Create and initialize the device buffer
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags (excluding internal flags like CopyDst which are added automatically)
     * @param device Logical device to create resources
     * @param debugName Optional debug name
     * @return true if creation succeeded
     */
    bool create(uint64_t size,
                DeviceBufferUsage usage,
                const std::shared_ptr<ILogicalDevice>& device,
                const std::string& debugName = "");
    
    /**
     * Destroy the device buffer and release resources
     */
    void destroy();
    
    
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
    std::shared_ptr<INativeBuffer> _buffer;  // Internal WebGPU buffer
    uint64_t _size;
    BufferUsage _usage;
    std::string _debugName;
    uint64_t _totalBytesTransferred;
    uint32_t _transferCount;
    bool _created;
};

} // namespace pers

