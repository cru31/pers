#pragma once

#include <memory>
#include "pers/graphics/buffers/IBuffer.h"

namespace pers {

class ICommandEncoder;
class ILogicalDevice;

/**
 * GPU-only buffer for maximum performance
 * No CPU access, data upload via staging buffers
 */
class DeviceBuffer final : public IBuffer, public std::enable_shared_from_this<DeviceBuffer> {
public:
    DeviceBuffer();
    ~DeviceBuffer() override;
    
    /**
     * Create and initialize the device buffer
     * @param desc Buffer description
     * @param device Logical device to create resources
     * @return true if creation succeeded
     */
    bool create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device);
    
    /**
     * Destroy the device buffer and release resources
     */
    void destroy();
    
    // No copy
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    
    // Move only
    DeviceBuffer(DeviceBuffer&& other) noexcept;
    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept;
    
    /**
     * Copy data from source buffer
     */
    bool copyFrom(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<IBuffer>& source,
                  const BufferCopyDesc& copyDesc = {0, 0, BufferCopyDesc::WHOLE_SIZE});
    
    /**
     * Copy data to destination buffer
     */
    bool copyTo(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<IBuffer>& destination,
                const BufferCopyDesc& copyDesc = {0, 0, BufferCopyDesc::WHOLE_SIZE});
    
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
    std::shared_ptr<IBuffer> _buffer;  // Internal WebGPU buffer
    BufferDesc _desc;
    uint64_t _totalBytesTransferred;
    uint32_t _transferCount;
    bool _created;
};

} // namespace pers