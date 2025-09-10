#pragma once

#include <memory>
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/IBufferFactory.h"

namespace pers {

class ICommandEncoder;

namespace graphics {

/**
 * GPU-only buffer for maximum performance
 * No CPU access, data upload via staging buffers
 * This is an abstract base class - actual implementation in backend
 */
class DeviceBuffer : public IBuffer, public std::enable_shared_from_this<DeviceBuffer> {
public:
    explicit DeviceBuffer(const BufferDesc& desc);
    ~DeviceBuffer() override;
    
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
    
    // IBuffer interface - derived classes must implement
    uint64_t getSize() const override = 0;
    BufferUsage getUsage() const override = 0;
    const std::string& getDebugName() const override;
    NativeBufferHandle getNativeHandle() const override = 0;
    bool isValid() const override = 0;
    BufferState getState() const override = 0;
    MemoryLocation getMemoryLocation() const override;
    AccessPattern getAccessPattern() const override;
    
protected:
    BufferDesc _desc;
    uint64_t _totalBytesTransferred;
    uint32_t _transferCount;
};

} // namespace graphics
} // namespace pers