#pragma once

#include <cstdint>
#include <string>

namespace pers {

/**
 * Buffer state enum
 */
enum class BufferState : uint8_t {
    Uninitialized = 0,
    Ready = 1,
    Mapped = 2,
    MapPending = 3,
    Destroyed = 4
};

/**
 * Mapping mode for buffer access
 */
enum class MapMode : uint8_t {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3
};

/**
 * Buffer usage flags (bitmask)
 */
enum class BufferUsage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    CopySrc = 1 << 4,
    CopyDst = 1 << 5,
    MapRead = 1 << 6,
    MapWrite = 1 << 7,
    Indirect = 1 << 8,
    QueryResolve = 1 << 9,
    All = 0xFFFFFFFF
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferUsage operator&(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline BufferUsage& operator|=(BufferUsage& a, BufferUsage b) {
    a = a | b;
    return a;
}

inline BufferUsage& operator&=(BufferUsage& a, BufferUsage b) {
    a = a & b;
    return a;
}

inline bool operator!(BufferUsage a) {
    return static_cast<uint32_t>(a) == 0;
}

inline bool hasFlag(BufferUsage flags, BufferUsage flag) {
    return (flags & flag) == flag;
}

/**
 * Buffer memory location hint
 */
enum class MemoryLocation : uint8_t {
    Auto = 0,          // Let the system decide
    DeviceLocal = 1,   // VRAM (GPU only, fastest for GPU)
    HostVisible = 2,   // System RAM (CPU writable, GPU readable)
    HostCached = 3,    // System RAM (CPU readable, GPU writable)
    Unified = 4        // Shared memory (APU/UMA systems)
};

/**
 * Buffer access pattern hint
 */
enum class AccessPattern : uint8_t {
    Static = 0,        // Written once, read many times
    Dynamic = 1,       // Updated occasionally (per level/scene)
    Stream = 2,        // Updated frequently (per frame)
    Staging = 3        // Temporary transfer buffer
};

/**
 * Buffer alignment requirements
 */
struct BufferAlignment {
    static constexpr uint64_t UNIFORM_BUFFER_OFFSET = 256;
    static constexpr uint64_t STORAGE_BUFFER_OFFSET = 256;
    static constexpr uint64_t VERTEX_BUFFER_OFFSET = 4;
    static constexpr uint64_t INDEX_BUFFER_OFFSET = 4;
    static constexpr uint64_t COPY_BUFFER_OFFSET = 4;
    static constexpr uint64_t DYNAMIC_OFFSET = 256;
    static constexpr uint64_t DEFAULT = 16;
};

/**
 * Buffer limits
 */
struct BufferLimits {
    static constexpr uint64_t MAX_BUFFER_SIZE = 2147483648;        // 2GB
    static constexpr uint64_t MAX_UNIFORM_BUFFER_SIZE = 65536;     // 64KB
    static constexpr uint64_t MAX_STORAGE_BUFFER_SIZE = 134217728; // 128MB
    static constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 32;
    static constexpr uint32_t MAX_VERTEX_BUFFER_STRIDE = 2048;
};

/**
 * Buffer descriptor for creation
 */
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    MemoryLocation memoryLocation = MemoryLocation::Auto;
    AccessPattern accessPattern = AccessPattern::Static;
    bool mappedAtCreation = false;
    std::string debugName;
    
    // Validation
    bool isValid() const;
    uint64_t getAlignedSize(BufferUsage usage) const;
};

/**
 * Buffer copy descriptor
 */
struct BufferCopyDesc {
    uint64_t srcOffset = 0;
    uint64_t dstOffset = 0;
    uint64_t size = 0;
    
    static constexpr uint64_t WHOLE_SIZE = static_cast<uint64_t>(-1);
};

/**
 * Buffer mapping range
 */
struct BufferMapRange {
    uint64_t offset = 0;
    uint64_t size = 0;
    
    static constexpr uint64_t WHOLE_BUFFER = static_cast<uint64_t>(-1);
};

} // namespace pers