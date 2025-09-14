#pragma once

#include <future>
#include <functional>
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/MappedData.h"

namespace pers {


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

} // namespace pers