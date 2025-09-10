#pragma once

#include "pers/graphics/buffers/IMappableBuffer.h"
#include <memory>
#include <future>

namespace pers {

class ICommandEncoder;

namespace graphics {

class DeviceBuffer;

/**
 * Abstract base class for deferred mapping staging buffers
 * 
 * Deferred mapping buffers use mapAsync for asynchronous CPU access.
 * These are typically used for:
 * - GPU -> CPU readback operations (reading rendered results)
 * - Large data uploads that don't need immediate CPU access
 * - Asynchronous data transfers
 * 
 * Backend implementations (WebGPU, Vulkan, etc.) should inherit from this class.
 */
class DeferredStagingBuffer : public IMappableBuffer, public std::enable_shared_from_this<DeferredStagingBuffer> {
public:
    explicit DeferredStagingBuffer(const BufferDesc& desc);
    virtual ~DeferredStagingBuffer();
    
    // No copy
    DeferredStagingBuffer(const DeferredStagingBuffer&) = delete;
    DeferredStagingBuffer& operator=(const DeferredStagingBuffer&) = delete;
    
    // Move only
    DeferredStagingBuffer(DeferredStagingBuffer&& other) noexcept;
    DeferredStagingBuffer& operator=(DeferredStagingBuffer&& other) noexcept;
    
    // IMappableBuffer interface
    virtual void* getMappedData() override { return _currentMapping.data(); }
    virtual const void* getMappedData() const override { return _currentMapping.data(); }
    virtual std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, const BufferMapRange& range = {}) override = 0;
    virtual void unmap() override = 0;
    virtual bool isMapped() const override = 0;
    virtual bool isMapPending() const override { return _mappingPending; }
    virtual void flushMappedRange(uint64_t offset, uint64_t size) override = 0;
    virtual void invalidateMappedRange(uint64_t offset, uint64_t size) override = 0;
    
    // IBuffer interface
    virtual uint64_t getSize() const override;
    virtual BufferUsage getUsage() const override;
    virtual NativeBufferHandle getNativeHandle() const override = 0;
    virtual const std::string& getDebugName() const override { return _desc.debugName; }
    virtual bool isValid() const override { return getSize() > 0; }
    virtual BufferState getState() const override { return isMapped() ? BufferState::Mapped : BufferState::Ready; }
    virtual MemoryLocation getMemoryLocation() const override { return _desc.memoryLocation; }
    virtual AccessPattern getAccessPattern() const override { return _desc.accessPattern; }
    
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
    
protected:
    BufferDesc _desc;
    mutable MappedData _currentMapping;
    mutable std::future<MappedData> _mappingFuture;
    mutable bool _mappingPending;
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

} // namespace graphics
} // namespace pers