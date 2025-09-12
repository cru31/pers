#pragma once

#include "pers/graphics/buffers/IMappableBuffer.h"

#include <memory>
#include <future>

namespace pers {

class ICommandEncoder;
class DeviceBuffer;
class ILogicalDevice;

/**
 * Deferred mapping staging buffer for asynchronous CPU access
 * 
 * Deferred mapping buffers use mapAsync for asynchronous CPU access.
 * These are typically used for:
 * - GPU -> CPU readback operations (reading rendered results)
 * - Large data uploads that don't need immediate CPU access
 * - Asynchronous data transfers
 */
class DeferredStagingBuffer final : public IMappableBuffer, public std::enable_shared_from_this<DeferredStagingBuffer> {
public:
    DeferredStagingBuffer();
    ~DeferredStagingBuffer() override;
    /**
     * Create and initialize the staging buffer
     * @param desc Buffer description
     * @param device Logical device to create resources
     * @return true if creation succeeded
     */
    bool create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device);

    /**
     * Destroy the staging buffer and release resources
     */
    void destroy();
    
    // No copy
    DeferredStagingBuffer(const DeferredStagingBuffer&) = delete;
    DeferredStagingBuffer& operator=(const DeferredStagingBuffer&) = delete;
    
    // Move only
    DeferredStagingBuffer(DeferredStagingBuffer&& other) noexcept;
    DeferredStagingBuffer& operator=(DeferredStagingBuffer&& other) noexcept;
    
    // IMappableBuffer interface
    void* getMappedData() override;
    const void* getMappedData() const override;
    std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, const BufferMapRange& range = {}) override;
    void unmap() override;
    bool isMapped() const override;
    bool isMapPending() const override;
    void flushMappedRange(uint64_t offset, uint64_t size) override;
    void invalidateMappedRange(uint64_t offset, uint64_t size) override;
    
    // IBuffer interface
    uint64_t getSize() const override;
    BufferUsage getUsage() const override;
    NativeBufferHandle getNativeHandle() const override;
    const std::string& getDebugName() const override;
    bool isValid() const override;
    BufferState getState() const override;
    MemoryLocation getMemoryLocation() const override;
    AccessPattern getAccessPattern() const override;
    
    // Convenience methods for data transfer
    bool writeBytes(const void* data, uint64_t size, uint64_t offset = 0);
    bool readBytes(void* data, uint64_t size, uint64_t offset = 0) const;
    
    // Template convenience methods
    template<typename T>
    bool write(const T* data, size_t count, size_t offsetElements = 0);
    
    template<typename T>
    bool read(T* data, size_t count, size_t offsetElements = 0) const;
    
    // Transfer operations - DeferredStagingBuffer is primarily for readback
    bool downloadFrom(const std::shared_ptr<ICommandEncoder>& encoder, 
                     const std::shared_ptr<DeviceBuffer>& source,
                     const BufferCopyDesc& copyDesc = {});
    
private:
    std::shared_ptr<IMappableBuffer> _buffer;  // Internal WebGPU mappable buffer
    BufferDesc _desc;
    mutable MappedData _currentMapping;
    mutable std::future<MappedData> _mappingFuture;
    mutable bool _mappingPending;
    bool _created;
};

// Template implementation
template<typename T>
bool DeferredStagingBuffer::write(const T* data, size_t count, size_t offsetElements) {
    uint64_t byteOffset = offsetElements * sizeof(T);
    uint64_t byteSize = count * sizeof(T);
    return writeBytes(data, byteSize, byteOffset);
}

template<typename T>
bool DeferredStagingBuffer::read(T* data, size_t count, size_t offsetElements) const {
    uint64_t byteOffset = offsetElements * sizeof(T);
    uint64_t byteSize = count * sizeof(T);
    return readBytes(data, byteSize, byteOffset);
}

} // namespace pers