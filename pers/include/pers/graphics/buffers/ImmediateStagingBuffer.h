#pragma once

#include <memory>
#include "pers/graphics/buffers/IMappableBuffer.h"
#include "pers/graphics/buffers/IBufferFactory.h"

namespace pers {

class ICommandEncoder;
class DeviceBuffer;

/**
 * Staging buffer with immediate CPU access (mappedAtCreation=true)
 * Ideal for initial data upload and static resources
 */
class ImmediateStagingBuffer final : public IMappableBuffer, public std::enable_shared_from_this<ImmediateStagingBuffer> {
public:
    ImmediateStagingBuffer(const BufferDesc& desc, const std::shared_ptr<IBufferFactory>& factory);
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
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    const std::string& getDebugName() const override;
    NativeBufferHandle getNativeHandle() const override;
    bool isValid() const override;
    BufferState getState() const override;
    MemoryLocation getMemoryLocation() const override;
    AccessPattern getAccessPattern() const override;
    
    // IMappableBuffer interface
    void* getMappedData() override;
    const void* getMappedData() const override;
    std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, 
                                     const BufferMapRange& range = {0, BufferMapRange::WHOLE_BUFFER}) override;
    void unmap() override;
    bool isMapped() const override;
    bool isMapPending() const override;
    void flushMappedRange(uint64_t offset, uint64_t size) override;
    void invalidateMappedRange(uint64_t offset, uint64_t size) override;
    
private:
    std::unique_ptr<IMappableBuffer> _buffer;  // Internal WebGPU mappable buffer
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

} // namespace pers