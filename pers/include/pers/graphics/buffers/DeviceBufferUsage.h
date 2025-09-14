#pragma once

#include "BufferTypes.h"

namespace pers {

// Device buffer specific usage flags - excludes mapping flags
enum class DeviceBufferUsage : uint32_t {
    None = static_cast<uint32_t>(BufferUsage::None),
    Vertex = static_cast<uint32_t>(BufferUsage::Vertex),
    Index = static_cast<uint32_t>(BufferUsage::Index),
    Uniform = static_cast<uint32_t>(BufferUsage::Uniform),
    Storage = static_cast<uint32_t>(BufferUsage::Storage),
    CopySrc = static_cast<uint32_t>(BufferUsage::CopySrc),
    Indirect = static_cast<uint32_t>(BufferUsage::Indirect),
    QueryResolve = static_cast<uint32_t>(BufferUsage::QueryResolve),
};

inline DeviceBufferUsage operator|(DeviceBufferUsage a, DeviceBufferUsage b) {
    return static_cast<DeviceBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline DeviceBufferUsage operator&(DeviceBufferUsage a, DeviceBufferUsage b) {
    return static_cast<DeviceBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline DeviceBufferUsage& operator|=(DeviceBufferUsage& a, DeviceBufferUsage b) {
    a = a | b;
    return a;
}

inline DeviceBufferUsage& operator&=(DeviceBufferUsage& a, DeviceBufferUsage b) {
    a = a & b;
    return a;
}

inline bool operator!(DeviceBufferUsage a) {
    return static_cast<uint32_t>(a) == 0;
}

// Convert DeviceBufferUsage to BufferUsage (internal use)
inline BufferUsage toBufferUsage(DeviceBufferUsage usage) {
    return static_cast<BufferUsage>(usage) | BufferUsage::CopyDst;  // Always add CopyDst for device buffers
}

} // namespace pers