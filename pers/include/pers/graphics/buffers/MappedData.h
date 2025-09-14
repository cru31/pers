#pragma once
#include <functional>

namespace pers{

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

} // namespace pers