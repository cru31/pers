#pragma once

#include <memory>
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/IBufferFactory.h"

namespace pers {

class ICommandEncoder;

/**
 * GPU-only buffer for maximum performance
 * No CPU access, data upload via staging buffers
 */
class DeviceBuffer final : public IBuffer, public std::enable_shared_from_this<DeviceBuffer> {
public:
    DeviceBuffer(const BufferDesc& desc, const std::shared_ptr<IBufferFactory>& factory);
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
    std::unique_ptr<IBuffer> _buffer;  // Internal WebGPU buffer
    BufferDesc _desc;
    uint64_t _totalBytesTransferred;
    uint32_t _transferCount;
};

} // namespace pers