# Pers Graphics Engine V2 - Architecture Overview

## Overall System Architecture

```
┌─────────────────────────────────────────┐
│         Application Layer               │
├─────────────────────────────────────────┤
│         Pers Engine Layer               │
├─────────────────────────────────────────┤
│    Graphics Abstraction Layer (GAL)     │
│  ┌─────────────────────────────────┐   │
│  │  Core (Device, Queue, Instance) │   │
│  ├─────────────────────────────────┤   │
│  │     ResourceFactory             │   │
│  ├─────────────────────────────────┤   │
│  │    Command Recording            │   │
│  ├─────────────────────────────────┤   │
│  │     Presentation                │   │
│  └─────────────────────────────────┘   │
├─────────────────────────────────────────┤
│    Backend Implementation Layer         │
│  (WebGPU / Vulkan / Metal / D3D12)     │
├─────────────────────────────────────────┤
│         GPU Hardware                    │
└─────────────────────────────────────────┘
```

## Core Component Structure

### 1. Core System
```
IInstance
  └─> IPhysicalDevice
        └─> ILogicalDevice
              ├─> IQueue (command submission)
              ├─> IResourceFactory (resource creation)
              └─> ICommandEncoder (command recording)
```

### 2. Resource Factory System
```
IResourceFactory
  ├─> Buffer Creation
  │     ├─> CreateVertexBuffer()
  │     ├─> CreateIndexBuffer()
  │     ├─> CreateUniformBuffer()
  │     ├─> CreateStorageBuffer()
  │     └─> CreateIndirectBuffer()
  │
  ├─> Texture Creation
  │     ├─> CreateTexture2D()
  │     ├─> CreateTexture3D()
  │     ├─> CreateTextureCube()
  │     ├─> CreateTextureArray()
  │     └─> CreateRenderTexture()
  │
  ├─> Pipeline Creation
  │     ├─> CreateGraphicsPipeline()
  │     ├─> CreateComputePipeline()
  │     ├─> CreatePipelineLayout()
  │     └─> CreateBindGroupLayout()
  │
  ├─> Binding Creation
  │     ├─> CreateBindGroup()
  │     └─> CreateSampler()
  │
  └─> Shader Creation
        └─> CreateShaderModule()
```

### 3. Command System
```
ICommandEncoder
  ├─> beginRenderPass() → IRenderPassEncoder
  │     ├─> cmdSetGraphicsPipeline()
  │     ├─> cmdSetVertexBuffer()
  │     ├─> cmdSetIndexBuffer()
  │     ├─> cmdSetUniformBuffer()
  │     ├─> cmdSetBindGroup()
  │     ├─> cmdDraw()
  │     ├─> cmdDrawIndexed()
  │     └─> end()
  │
  ├─> beginComputePass() → IComputePassEncoder
  │     ├─> cmdSetComputePipeline()
  │     ├─> cmdSetStorageBuffer()
  │     ├─> cmdSetBindGroup()
  │     ├─> cmdDispatch()
  │     └─> end()
  │
  ├─> cmdCopyBufferToBuffer()
  ├─> cmdCopyBufferToTexture()
  ├─> cmdCopyTextureToBuffer()
  │
  └─> finish() → ICommandBuffer
```

## Resource Type Hierarchy

### Buffer Type Hierarchy
```
IBuffer (abstract base)
  ├─> IVertexBuffer      // Vertex data
  ├─> IIndexBuffer       // Index data
  ├─> IUniformBuffer     // Uniform data
  ├─> IStorageBuffer     // Read/write storage
  └─> IIndirectBuffer    // Indirect draw commands
```

### Texture Type Hierarchy
```
ITexture (abstract base)
  ├─> ITexture2D         // 2D texture
  ├─> ITexture3D         // 3D volume texture
  ├─> ITextureCube       // Cubemap texture
  ├─> ITextureArray      // Texture array
  └─> IRenderTexture     // Render target texture
```

### Pipeline Type Hierarchy
```
IPipeline (abstract base)
  ├─> IGraphicsPipeline  // Graphics pipeline
  └─> IComputePipeline   // Compute pipeline
```

## Data Flow

### Initialization Flow
```
1. Create Instance
     IInstance* instance = CreateInstance(desc);
     
2. Enumerate and select Physical Device
     IPhysicalDevice* physicalDevice = instance->RequestPhysicalDevice(options);
     
3. Create Logical Device
     ILogicalDevice* device = physicalDevice->CreateLogicalDevice(desc);
     
4. Get ResourceFactory
     IResourceFactory* factory = device->GetResourceFactory();
     
5. Get Queue
     IQueue* queue = device->GetQueue();
     
6. Create Surface and SwapChain
     ISurface* surface = instance->CreateSurface(window);
     ISwapChain* swapChain = device->CreateSwapChain(surface, desc);
```

### Rendering Flow
```
1. Resource Creation (at initialization)
     auto vertexBuffer = factory->CreateVertexBuffer(vertices);
     auto indexBuffer = factory->CreateIndexBuffer(indices);
     auto pipeline = factory->CreateGraphicsPipeline(pipelineDesc);
     
2. Frame Rendering (every frame)
     // Begin command recording
     auto encoder = device->CreateCommandEncoder();
     
     // Render pass
     auto renderPass = encoder->beginRenderPass(renderPassDesc);
     renderPass->cmdSetGraphicsPipeline(pipeline);
     renderPass->cmdSetVertexBuffer(0, vertexBuffer);
     renderPass->cmdSetIndexBuffer(indexBuffer);
     renderPass->cmdDrawIndexed(indexCount);
     renderPass->end();
     
     // Finish commands and submit
     auto commandBuffer = encoder->finish();
     queue->submit(commandBuffer);
     
     // Present to screen
     swapChain->present();
```

## Backend Implementation Mapping

### WebGPU Backend
```cpp
// Type-specific implementation but uses unified buffer internally
class WebGPUVertexBuffer : public IVertexBuffer {
    WGPUBuffer buffer;  // Internally WGPUBuffer
    // At creation: usage = WGPUBufferUsage_Vertex
}

class WebGPUIndexBuffer : public IIndexBuffer {
    WGPUBuffer buffer;  // Same WGPUBuffer
    // At creation: usage = WGPUBufferUsage_Index
}
```

### Vulkan Backend
```cpp
// Vulkan also has type-specific implementation
class VulkanVertexBuffer : public IVertexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    // At creation: usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
}
```

## Memory Management Strategy

### 1. Factory Pattern Benefits
- **Centralized Management**: ResourceFactory manages all resource creation
- **Caching Optimization**: Reuse identical resources
- **Memory Pooling**: Type-specific memory pool management
- **Statistics Collection**: Track resource usage

### 2. Lifecycle Management
```cpp
// Automatic management with shared_ptr
std::shared_ptr<IVertexBuffer> vertexBuffer;

// Factory returns shared_ptr on creation
auto factory = device->GetResourceFactory();
vertexBuffer = factory->CreateVertexBuffer(desc);

// Automatic release via reference counting
// Automatically deleted when last reference is gone
```

### 3. Frame Resources
- **Triple Buffering**: Cycle through 3 frame resources
- **Per-Frame Pools**: Temporary resources per frame
- **Automatic Recycling**: CommandEncoder reuse

## Synchronization Mechanisms

### CPU-GPU Synchronization
WebGPU uses callback-based synchronization instead of fences:
```cpp
// In IQueue interface
virtual void onSubmittedWorkDone(std::function<void()> callback) = 0;
```

### Frame Synchronization
```cpp
class FrameManager {
    static constexpr size_t FRAMES_IN_FLIGHT = 3;
    
    struct FrameData {
        ICommandEncoder* encoder;
        uint64_t submissionId;
        // Per-frame resources
    };
    
    FrameData frames[FRAMES_IN_FLIGHT];
    size_t currentFrame = 0;
}
```

## Error Handling Layers

### 1. Compile-Time Validation
```cpp
// Type system prevents mistakes
renderPass->cmdSetVertexBuffer(0, indexBuffer);  // Compile error!
```

### 2. Runtime Validation
```cpp
// Clear error messages
if (!vertexBuffer) {
    throw std::invalid_argument("VertexBuffer cannot be null");
}
```

### 3. Debug Layer
```cpp
#ifdef DEBUG
    ValidateRenderState();
    CheckResourceBinding();
#endif
```

## Extension Points

### 1. Adding New Resource Types
```cpp
// 1. Define interface
interface IIndirectBuffer : public IBuffer {
    virtual uint32_t GetDrawCount() = 0;
};

// 2. Add method to Factory
interface IResourceFactory {
    virtual IIndirectBuffer* CreateIndirectBuffer(const IndirectBufferDesc&) = 0;
};

// 3. Backend-specific implementation
class WebGPUIndirectBuffer : public IIndirectBuffer { };
```

### 2. Adding New Backends
```cpp
// Implement all interfaces
class MetalDevice : public IDevice { }
class MetalResourceFactory : public IResourceFactory { }
class MetalVertexBuffer : public IVertexBuffer { }
// ... All type-specific implementations
```

---

This architecture is designed with **type safety**, **separation of concerns**, and **extensibility** as core principles.