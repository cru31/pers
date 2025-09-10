# WebGPU Buffer System Complete Redesign

## 1. Current Problems

### Existing Issues
1. **Synchronous map() in asynchronous API**: IBuffer::map() is synchronous but WebGPU only supports async mapping
2. **No clear buffer purpose separation**: Same interface for GPU-only and CPU-accessible buffers
3. **mappedAtCreation confusion**: Not properly handled in current implementation
4. **No abstraction for common patterns**: Staging, readback, dynamic updates all handled ad-hoc
5. **Backend coupling**: Direct WebGPU usage in application code

## 2. Design Goals

### Primary Objectives
- **Backend Independence**: Complete abstraction from WebGPU/Vulkan/D3D12
- **Type Safety**: Prevent misuse through type system
- **Performance**: Zero-cost abstractions, optimal memory usage patterns
- **Ease of Use**: Simple API for common patterns
- **Extensibility**: Easy to add new backends and buffer types

### Design Principles
- **RAII**: Automatic resource management
- **Single Responsibility**: Each class has one clear purpose
- **Interface Segregation**: Separate interfaces for different capabilities
- **Dependency Inversion**: Depend on abstractions, not implementations

## 3. Architecture Overview

```
Application Layer
    ├── Uses high-level buffer classes (DeviceBuffer, StagingBuffer, etc.)
    └── Never touches backend directly

Abstraction Layer  
    ├── DeviceBuffer: GPU-only high-performance buffers
    ├── ImmediateStagingBuffer: Immediate mapping for uploads
    ├── DeferredStagingBuffer: Async mapping for uploads
    ├── ReadbackBuffer: GPU→CPU result reading
    └── DynamicBuffer: Ring buffer for per-frame updates

Interface Layer
    ├── IBuffer: Base buffer interface
    ├── IMappableBuffer: Buffers that support CPU mapping
    └── IBufferFactory: Buffer creation interface

Backend Layer
    ├── WebGPU Implementation
    │   ├── WebGPUBufferImpl: Implements IBuffer/IMappableBuffer
    │   └── WebGPUBufferFactory: Creates WebGPU buffers
    ├── Vulkan Implementation (future)
    └── D3D12 Implementation (future)
```

## 4. Interface Design

### Core Interfaces

```cpp
// Base buffer interface - all buffers implement this
class IBuffer {
public:
    virtual ~IBuffer() = default;
    
    // Core properties
    virtual uint64_t getSize() const = 0;
    virtual BufferUsage getUsage() const = 0;
    virtual const std::string& getDebugName() const = 0;
    
    // Backend handle for command encoding
    virtual void* getNativeHandle() const = 0;
    
    // State queries
    virtual bool isValid() const = 0;
    virtual BufferState getState() const = 0;
};

// Mappable buffer interface - for CPU-accessible buffers
class IMappableBuffer : public IBuffer {
public:
    // For mappedAtCreation=true buffers
    virtual void* getMappedData() = 0;
    virtual const void* getMappedData() const = 0;
    
    // For async mapping
    virtual std::future<void*> mapAsync(MapMode mode = MapMode::Write) = 0;
    virtual void unmap() = 0;
    
    // State queries
    virtual bool isMapped() const = 0;
    virtual bool isMapPending() const = 0;
};

// Factory interface - backend-specific creation
class IBufferFactory {
public:
    virtual ~IBufferFactory() = default;
    
    virtual std::unique_ptr<IBuffer> createBuffer(const BufferDesc& desc) = 0;
    virtual std::unique_ptr<IMappableBuffer> createMappableBuffer(const BufferDesc& desc) = 0;
};
```

### Supporting Types

```cpp
// Buffer state enum
enum class BufferState {
    Uninitialized,  // Not yet created
    Ready,          // Created and ready for use
    Mapped,         // Currently mapped to CPU
    MapPending,     // Async map in progress
    Destroyed       // Released
};

// Mapping mode
enum class MapMode {
    None = 0,
    Read = 1,       // GPU→CPU read
    Write = 2       // CPU→GPU write
};

// Buffer usage flags (bitmask)
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
    Indirect = 1 << 8
};

// Buffer descriptor
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    bool mappedAtCreation = false;
    std::string debugName;
};
```

## 5. Buffer Type Specifications

### DeviceBuffer
- **Purpose**: GPU-only buffers for maximum performance
- **CPU Access**: None (only through staging)
- **GPU Access**: Direct, cached, highest bandwidth
- **Usage**: Vertex, Index, Uniform, Storage buffers
- **Memory**: VRAM (device local)

### ImmediateStagingBuffer  
- **Purpose**: Upload data to GPU with immediate CPU write
- **CPU Access**: Write-only, immediate (mappedAtCreation=true)
- **GPU Access**: Read-only for copying
- **Usage**: Initial resource loading, static data
- **Memory**: Host-visible, write-combined

### DeferredStagingBuffer
- **Purpose**: Upload data to GPU with async CPU write
- **CPU Access**: Write-only, async (mapAsync)
- **GPU Access**: Read-only for copying
- **Usage**: Runtime resource streaming, dynamic loading
- **Memory**: Host-visible, write-combined

### ReadbackBuffer
- **Purpose**: Read GPU results back to CPU
- **CPU Access**: Read-only, async (mapAsync)
- **GPU Access**: Write-only for copying
- **Usage**: Compute results, queries, screenshots
- **Memory**: Host-visible, cached

### DynamicBuffer
- **Purpose**: Per-frame updates without stalls
- **CPU Access**: Write-only, ring buffer
- **GPU Access**: Direct read
- **Usage**: Frame uniforms, per-frame data
- **Memory**: Host-visible or BAR, triple buffered

## 6. Usage Patterns

### Pattern 1: Static Mesh Loading
```cpp
// Small mesh (< 1MB): direct initialization
BufferDesc desc{sizeof(vertices), BufferUsage::Vertex, true};
auto buffer = factory->createMappableBuffer(desc);
memcpy(buffer->getMappedData(), vertices, sizeof(vertices));
buffer->unmap();

// Large mesh (> 1MB): staging buffer
ImmediateStagingBuffer staging(desc, factory);
staging.write(meshData, meshSize);
DeviceBuffer gpuBuffer(desc, factory);
staging.uploadTo(encoder, gpuBuffer);
```

### Pattern 2: Texture Streaming
```cpp
DeferredStagingBuffer staging(desc, factory);
staging.mapAsync().then([](void* ptr) {
    loadTextureData(ptr);
}).then([&](void*) {
    staging.uploadTo(encoder, textureBuffer);
});
```

### Pattern 3: Per-Frame Updates
```cpp
DynamicBuffer frameUniforms(desc, factory);
auto handle = frameUniforms.beginUpdate().get();
updateFrameData(handle.as<FrameData>());
renderPass->setBuffer(0, handle.getBuffer());
```

### Pattern 4: GPU Readback
```cpp
ReadbackBuffer readback(desc, factory);
encoder->copyBufferToBuffer(gpuBuffer, readback);
readback.readAsync().then([](const MappedData& data) {
    processResults(data.as<ResultType>());
});
```

## 7. Implementation Strategy

### Phase 1: Core Infrastructure
1. Define all interfaces (IBuffer, IMappableBuffer, IBufferFactory)
2. Define supporting types (enums, descriptors)
3. Create folder structure for implementations

### Phase 2: WebGPU Backend
1. Implement WebGPUBufferImpl
2. Implement WebGPUBufferFactory
3. Handle mappedAtCreation and mapAsync properly

### Phase 3: Abstraction Classes
1. DeviceBuffer (GPU-only)
2. ImmediateStagingBuffer (mappedAtCreation=true)
3. DeferredStagingBuffer (mapAsync)
4. ReadbackBuffer (GPU→CPU)
5. DynamicBuffer (ring buffer)

### Phase 4: Migration
1. Update TriangleRenderer to use new system
2. Update StressTestRenderer to use new system
3. Remove old WebGPUBuffer class

## 8. File Structure

```
pers/include/pers/graphics/
├── buffers/
│   ├── IBuffer.h                    # Base interface
│   ├── IMappableBuffer.h            # Mappable interface
│   ├── IBufferFactory.h             # Factory interface
│   ├── BufferTypes.h                # Enums and descriptors
│   ├── DeviceBuffer.h               # GPU-only buffer
│   ├── ImmediateStagingBuffer.h    # Immediate staging
│   ├── DeferredStagingBuffer.h     # Async staging
│   ├── ReadbackBuffer.h             # GPU readback
│   └── DynamicBuffer.h              # Ring buffer
└── backends/webgpu/buffers/
    ├── WebGPUBufferImpl.h           # WebGPU implementation
    └── WebGPUBufferFactory.h        # WebGPU factory

pers/src/graphics/
├── buffers/
│   ├── DeviceBuffer.cpp
│   ├── ImmediateStagingBuffer.cpp
│   ├── DeferredStagingBuffer.cpp
│   ├── ReadbackBuffer.cpp
│   └── DynamicBuffer.cpp
└── backends/webgpu/buffers/
    ├── WebGPUBufferImpl.cpp
    └── WebGPUBufferFactory.cpp
```

## 9. Migration Plan

### Step 1: Parallel Development
- Keep existing IBuffer/WebGPUBuffer working
- Develop new system in parallel under buffers/ namespace

### Step 2: Test with Simple Cases
- Create unit tests for each buffer type
- Test with TriangleRenderer (simple case)

### Step 3: Full Migration
- Update all renderers to use new system
- Remove old IBuffer/WebGPUBuffer

### Step 4: Documentation
- API documentation for each buffer type
- Usage examples and best practices
- Performance guidelines

## 10. Success Criteria

### Functional Requirements
- [x] All buffer types properly abstract WebGPU
- [x] mappedAtCreation handled correctly
- [x] mapAsync handled correctly  
- [x] No direct WebGPU usage in application code
- [x] Type-safe buffer usage

### Performance Requirements
- [x] Zero-cost abstractions (no runtime overhead)
- [x] Optimal memory usage patterns
- [x] No unnecessary copies or allocations
- [x] Proper GPU/CPU synchronization

### Quality Requirements  
- [x] RAII for all resources
- [x] Clear error messages
- [x] Comprehensive logging
- [x] Thread-safe where needed

## 11. Risk Analysis

### Technical Risks
1. **Async complexity**: std::future may not be ideal for all patterns
   - Mitigation: Consider callback-based API as alternative
   
2. **Backend differences**: Vulkan/D3D12 have different mapping semantics
   - Mitigation: Design interfaces to support all backends
   
3. **Performance overhead**: Abstraction may add overhead
   - Mitigation: Use templates and inline functions

### Schedule Risks
1. **Large refactoring**: Affects all existing rendering code
   - Mitigation: Parallel development, gradual migration
   
2. **Testing complexity**: Many buffer types and usage patterns
   - Mitigation: Comprehensive unit tests from start

## 12. Open Questions

1. Should we use std::future or callback-based async API?
2. How to handle buffer resizing?
3. Should DynamicBuffer support more than 3 frames?
4. How to integrate with command encoder abstraction?
5. Memory allocation strategy (pooling vs direct)?

## 13. Decision Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2025-01-11 | Use std::future for async | Modern C++ standard, composable |
| 2025-01-11 | Triple buffering for DynamicBuffer | Balance between latency and memory |
| 2025-01-11 | RAII for all resources | Automatic cleanup, exception safety |
| 2025-01-11 | Separate staging buffer types | Clear intent, optimized for use case |

## 14. References

- WebGPU Specification: https://www.w3.org/TR/webgpu/
- Dawn Buffer Implementation: https://dawn.googlesource.com/dawn
- Vulkan Memory Management: https://gpuopen.com/vulkan-memory-management/
- D3D12 Resource Barriers: https://docs.microsoft.com/directx/d3d12-resource-barriers