#include "pers/graphics/buffers/BufferTypes.h"

namespace pers {
namespace graphics {

BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

BufferUsage operator&(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

BufferUsage& operator|=(BufferUsage& a, BufferUsage b) {
    a = a | b;
    return a;
}

BufferUsage& operator&=(BufferUsage& a, BufferUsage b) {
    a = a & b;
    return a;
}

bool operator!(BufferUsage a) {
    return static_cast<uint32_t>(a) == 0;
}

bool hasFlag(BufferUsage flags, BufferUsage flag) {
    return (flags & flag) == flag;
}

bool BufferDesc::isValid() const {
    if (size == 0) {
        return false;
    }
    
    if (size > BufferLimits::MAX_BUFFER_SIZE) {
        return false;
    }
    
    if (usage == BufferUsage::None) {
        return false;
    }
    
    if (hasFlag(usage, BufferUsage::Uniform) && size > BufferLimits::MAX_UNIFORM_BUFFER_SIZE) {
        return false;
    }
    
    if (hasFlag(usage, BufferUsage::Storage) && size > BufferLimits::MAX_STORAGE_BUFFER_SIZE) {
        return false;
    }
    
    if (mappedAtCreation) {
        if (!hasFlag(usage, BufferUsage::MapWrite) && !hasFlag(usage, BufferUsage::CopySrc)) {
            return false;
        }
    }
    
    return true;
}

uint64_t BufferDesc::getAlignedSize(BufferUsage usage) const {
    uint64_t alignment = BufferAlignment::DEFAULT;
    
    if (hasFlag(usage, BufferUsage::Uniform)) {
        alignment = BufferAlignment::UNIFORM_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Storage)) {
        alignment = BufferAlignment::STORAGE_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Vertex)) {
        alignment = BufferAlignment::VERTEX_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Index)) {
        alignment = BufferAlignment::INDEX_BUFFER_OFFSET;
    }
    
    return ((size + alignment - 1) / alignment) * alignment;
}

} // namespace graphics
} // namespace pers