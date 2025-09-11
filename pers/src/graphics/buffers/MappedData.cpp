#include "pers/graphics/buffers/IMappableBuffer.h"

namespace pers {

MappedData::MappedData(void* data, uint64_t size, std::function<void()> unmapCallback)
    : _data(data)
    , _size(size)
    , _unmapCallback(std::move(unmapCallback))
    , _moved(false) {
}

MappedData::~MappedData() {
    if (!_moved && _unmapCallback) {
        _unmapCallback();
    }
}

MappedData::MappedData(MappedData&& other) noexcept
    : _data(other._data)
    , _size(other._size)
    , _unmapCallback(std::move(other._unmapCallback))
    , _moved(false) {
    other._moved = true;
}

MappedData& MappedData::operator=(MappedData&& other) noexcept {
    if (this != &other) {
        if (!_moved && _unmapCallback) {
            _unmapCallback();
        }
        
        _data = other._data;
        _size = other._size;
        _unmapCallback = std::move(other._unmapCallback);
        _moved = false;
        other._moved = true;
    }
    return *this;
}

void* MappedData::data() {
    return _data;
}

const void* MappedData::data() const {
    return _data;
}

uint64_t MappedData::size() const {
    return _size;
}

uint64_t MappedData::count() const {
    return _size;
}

} // namespace pers