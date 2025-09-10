#pragma once

#include <future>
#include <functional>
#include "pers/graphics/buffers/IBuffer.h"

namespace pers {
namespace graphics {

/**
 * Mapped data wrapper with RAII auto-unmap
 */
class MappedData {
public:
    MappedData(void* data, uint64_t size, std::function<void()> unmapCallback);
    ~MappedData();
    
    // Move only, no copy
    MappedData(const MappedData&) = delete;
    MappedData& operator=(const MappedData&) = delete;
    MappedData(MappedData&& other) noexcept;
    MappedData& operator=(MappedData&& other) noexcept;
    
    void* data();
    const void* data() const;
    uint64_t size() const;
    
    template<typename T>
    T* as();
    
    template<typename T>
    const T* as() const;
    
    template<typename T>
    T* asArray();
    
    template<typename T>
    const T* asArray() const;
    
    uint64_t count() const;
    
private:
    void* _data;
    uint64_t _size;
    std::function<void()> _unmapCallback;
    bool _moved;
};

/**
 * Interface for CPU-mappable buffers
 */
class IMappableBuffer : public IBuffer {
public:
    virtual ~IMappableBuffer() = default;
    
    /**
     * Get mapped data for buffers created with mappedAtCreation=true
     */
    virtual void* getMappedData() = 0;
    virtual const void* getMappedData() const = 0;
    
    /**
     * Asynchronously map buffer for CPU access
     */
    virtual std::future<MappedData> mapAsync(MapMode mode = MapMode::Write, const BufferMapRange& range = {0, BufferMapRange::WHOLE_BUFFER}) = 0;
    
    /**
     * Unmap buffer (if mapped)
     */
    virtual void unmap() = 0;
    
    /**
     * Check if buffer is currently mapped
     */
    virtual bool isMapped() const = 0;
    
    /**
     * Check if async map is pending
     */
    virtual bool isMapPending() const = 0;
    
    /**
     * Flush mapped range (make CPU writes visible to GPU)
     */
    virtual void flushMappedRange(uint64_t offset, uint64_t size) = 0;
    
    /**
     * Invalidate mapped range (make GPU writes visible to CPU)
     */
    virtual void invalidateMappedRange(uint64_t offset, uint64_t size) = 0;
};

// Template implementations
template<typename T>
T* MappedData::as() {
    return static_cast<T*>(_data);
}

template<typename T>
const T* MappedData::as() const {
    return static_cast<const T*>(_data);
}

template<typename T>
T* MappedData::asArray() {
    return static_cast<T*>(_data);
}

template<typename T>
const T* MappedData::asArray() const {
    return static_cast<const T*>(_data);
}

} // namespace graphics
} // namespace pers