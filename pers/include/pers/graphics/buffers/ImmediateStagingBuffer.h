#pragma once

#include <memory>
#include "pers/graphics/buffers/IMappableBuffer.h"
#include "pers/graphics/buffers/IBufferFactory.h"

namespace pers {

class ICommandEncoder;

namespace graphics {

class DeviceBuffer;

/**
 * Staging buffer with immediate CPU access (mappedAtCreation=true)
 * Ideal for initial data upload and static resources
 * This is an abstract base class - actual implementation in backend
 */
class ImmediateStagingBuffer : public IMappableBuffer, public std::enable_shared_from_this<ImmediateStagingBuffer> {
public:
    explicit ImmediateStagingBuffer(const BufferDesc& desc);
    ~ImmediateStagingBuffer() override;
    
    // No copy
    ImmediateStagingBuffer(const ImmediateStagingBuffer&) = delete;
    ImmediateStagingBuffer& operator=(const ImmediateStagingBuffer&) = delete;
    
    // Move only
    ImmediateStagingBuffer(ImmediateStagingBuffer&& other) noexcept;
    ImmediateStagingBuffer& operator=(ImmediateStagingBuffer&& other) noexcept;
    
    /**
     * Write data to buffer
     */
    template<typename T>
    void write(const T* data, size_t count, size_t offsetElements = 0);
    
    void writeBytes(const void* data, uint64_t size, uint64_t offset = 0);
    
    /**
     * Finalize buffer (unmap and prepare for GPU transfer)
     */
    void finalize();
    
    /**
     * Upload to device buffer
     */
    bool uploadTo(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<DeviceBuffer>& target,
                  const BufferCopyDesc& copyDesc = {0, 0, BufferCopyDesc::WHOLE_SIZE});
    
    /**
     * Upload to another buffer
     */
    bool uploadTo(const std::shared_ptr<ICommandEncoder>& encoder, const std::shared_ptr<IBuffer>& target,
                  const BufferCopyDesc& copyDesc = {0, 0, BufferCopyDesc::WHOLE_SIZE});
    
    /**
     * Check if buffer is finalized
     */
    bool isFinalized() const;
    
    /**
     * Get bytes written so far
     */
    uint64_t getBytesWritten() const;
    
    // IBuffer interface - derived classes must implement
    uint64_t getSize() const override = 0;
    BufferUsage getUsage() const override = 0;
    const std::string& getDebugName() const override;
    NativeBufferHandle getNativeHandle() const override = 0;
    bool isValid() const override = 0;
    BufferState getState() const override;
    MemoryLocation getMemoryLocation() const override;
    AccessPattern getAccessPattern() const override;
    
    // IMappableBuffer interface - derived classes must implement
    void* getMappedData() override = 0;
    const void* getMappedData() const override = 0;
    std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, 
                                     const BufferMapRange& range = {0, BufferMapRange::WHOLE_BUFFER}) override = 0;
    void unmap() override = 0;
    bool isMapped() const override = 0;
    
protected:
    BufferDesc _desc;
    void* _mappedData;
    bool _finalized;
    uint64_t _bytesWritten;
};

// Template implementation
template<typename T>
void ImmediateStagingBuffer::write(const T* data, size_t count, size_t offsetElements) {
    if (_finalized || !_mappedData) {
        return;
    }
    
    uint64_t byteOffset = offsetElements * sizeof(T);
    uint64_t byteSize = count * sizeof(T);
    
    if (byteOffset + byteSize > _desc.size) {
        return;
    }
    
    writeBytes(data, byteSize, byteOffset);
}

} // namespace graphics
} // namespace pers