# Pers Graphics Engine V2 - Core Philosophy

## Core Design Principles

### 1. Type Safety First
```cpp
// ✅ Clear type definitions for each purpose
IVertexBuffer* vertexBuffer;    // For vertex data only
IIndexBuffer* indexBuffer;       // For index data only
IUniformBuffer* uniformBuffer;   // For uniform data only

// Prevent mistakes at compile time
renderPass->SetVertexBuffer(0, indexBuffer);  // Compile error!
```

### 2. Single Responsibility
```cpp
// IDevice: Device management and basic functionality only
interface IDevice {
    IQueue* GetQueue();
    IResourceFactory* GetResourceFactory();
    ICommandEncoder* CreateCommandEncoder();
}

// IResourceFactory: Dedicated to all resource creation
interface IResourceFactory {
    IVertexBuffer* CreateVertexBuffer(const VertexBufferDesc&);
    IIndexBuffer* CreateIndexBuffer(const IndexBufferDesc&);
    ITexture2D* CreateTexture2D(const Texture2DDesc&);
    // ... all resource creation
}
```

### 3. Explicit Control
```cpp
// All GPU operations are explicit
auto encoder = device->CreateCommandEncoder();
auto renderPass = encoder->BeginRenderPass(target);
renderPass->SetPipeline(pipeline);
renderPass->SetVertexBuffer(0, vertexBuffer);
renderPass->Draw(vertexCount);
renderPass->End();
auto commands = encoder->Finish();
queue->Submit(commands);
```

### 4. Semantic Clarity
```cpp
// Each type has clear semantics
IVertexBuffer      // Buffer for vertex data
IIndexBuffer       // Buffer for indices  
IUniformBuffer     // Buffer for uniform variables
IStorageBuffer     // Read/write storage buffer

ITexture2D         // 2D texture
ITexture3D         // 3D volume texture
ITextureCube       // Cubemap texture
ITextureArray      // Texture array
```

## GPU Execution Model

### Triple Buffering Pipeline
```
Frame N-2          Frame N-1          Frame N
[Presenting] ----> [GPU Executing] ----> [CPU Recording]
     ↑                   ↑                     ↑
  Displaying        GPU Rendering      CPU Recording Commands
```

### Command Recording and Submission Flow
```cpp
// 1. Create CommandEncoder from Device
auto encoder = device->CreateCommandEncoder();

// 2. Begin RenderPass and record commands
{
    auto renderPass = encoder->BeginRenderPass(desc);
    renderPass->SetGraphicsPipeline(pipeline);
    renderPass->SetVertexBuffer(0, vertexBuffer);
    renderPass->SetIndexBuffer(indexBuffer);
    renderPass->DrawIndexed(indexCount);
    renderPass->End();
}

// 3. Finish commands
auto commandBuffer = encoder->Finish();

// 4. Submit to Queue
device->GetQueue()->Submit(commandBuffer);
```

## Resource Management System

### Resource Creation Hierarchy
```
Device
  └─> ResourceFactory (Dedicated to resource creation)
        ├─> BufferFactory
        │     ├─> CreateVertexBuffer()
        │     ├─> CreateIndexBuffer()
        │     ├─> CreateUniformBuffer()
        │     └─> CreateStorageBuffer()
        │
        ├─> TextureFactory  
        │     ├─> CreateTexture2D()
        │     ├─> CreateTexture3D()
        │     ├─> CreateTextureCube()
        │     └─> CreateTextureArray()
        │
        └─> PipelineFactory
              ├─> CreateGraphicsPipeline()
              ├─> CreateComputePipeline()
              └─> CreateRenderBundle()
```

### Resource Lifecycle
```cpp
// Creation: Handled by ResourceFactory
auto factory = device->GetResourceFactory();
auto vertexBuffer = factory->CreateVertexBuffer(desc);

// Usage: Safe with explicit types
renderPass->SetVertexBuffer(0, vertexBuffer);

// Destruction: Automatic management via shared_ptr
// vertexBuffer is automatically released when out of scope
```

## Backend Abstraction Strategy

### Interface and Implementation Separation
```cpp
// Interface (backend-independent)
interface IVertexBuffer {
    virtual size_t GetSize() = 0;
    virtual size_t GetStride() = 0;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
}

// WebGPU Implementation
class WebGPUVertexBuffer : public IVertexBuffer {
    WGPUBuffer buffer;  // WebGPU uses unified buffers
    BufferUsage usage = BufferUsage::Vertex;
}

// Vulkan Implementation
class VulkanVertexBuffer : public IVertexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}
```

## Error Handling Philosophy

### Fail-Fast with Clear Messages
```cpp
void RenderPassEncoder::SetVertexBuffer(uint32_t slot, IVertexBuffer* buffer) {
    if (!buffer) {
        throw std::invalid_argument("Vertex buffer cannot be null");
    }
    if (slot >= MAX_VERTEX_BUFFERS) {
        throw std::out_of_range("Vertex buffer slot exceeds maximum");
    }
    // Type is already validated at compile time
}
```

### Debug Build Additional Validation
```cpp
#ifdef DEBUG
void ValidatePipeline(IGraphicsPipeline* pipeline) {
    // Validate pipeline state
    // Validate shader compatibility
    // Validate resource bindings
}
#endif
```

## Performance Optimization Principles

### 1. Resource Creation Optimization
- Centralized caching in ResourceFactory
- Prevent recreation with same Descriptor
- Resource pooling and reuse

### 2. Type-specific Optimization
```cpp
// VertexBuffer optimized for GPU reading
class WebGPUVertexBuffer {
    // GPU_READ optimized memory allocation
}

// UniformBuffer optimized for CPU writing  
class WebGPUUniformBuffer {
    // CPU_WRITE optimized memory allocation
    // Dynamic offset support
}
```

### 3. Command Recording Optimization
- CommandEncoder reuse
- Optimize repeated commands with RenderBundle
- Draw call batching

## Extensibility Considerations

### Adding New Resource Types
```cpp
// When adding new buffer types
interface IIndirectBuffer : public IBuffer {
    virtual uint32_t GetDrawCount() = 0;
}

// Add to ResourceFactory
interface IResourceFactory {
    // ... existing methods ...
    IIndirectBuffer* CreateIndirectBuffer(const IndirectBufferDesc&);
}
```

### Adding New Backends
```cpp
// New backends must implement all interfaces
class MetalDevice : public IDevice { }
class MetalResourceFactory : public IResourceFactory { }
class MetalVertexBuffer : public IVertexBuffer { }
// ... implementation for all resource types
```

---

This philosophy prioritizes **type safety**, **clear separation of responsibilities**, and **semantic clarity**.