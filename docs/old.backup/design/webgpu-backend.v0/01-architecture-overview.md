# WebGPU Backend Architecture Overview

## 1. High-Level Architecture

WebGPU provides a modern, safe, and portable graphics API that maps well to Vulkan, Metal, and D3D12. Based on the LearnWebGPU guide principles, our architecture follows these key concepts:

### Core Principles
1. **Explicit Resource Management**: All resources are explicitly created and destroyed
2. **Command Buffer Architecture**: Commands are recorded into buffers before submission
3. **Async Pipeline**: Operations are asynchronous with explicit synchronization
4. **Descriptor-based Binding**: Resources are bound through bind groups (descriptor sets)

## 2. System Layers

### Layer 1: WebGPU Core Abstraction
```cpp
namespace prism::webgpu {

// Top-level device management
class WebGPUDevice {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    
    // Capabilities and limits
    WGPULimits limits;
    WGPUSupportedFeatures features;
};

// Surface for presentation
class WebGPUSurface {
    WGPUSurface surface;
    WGPUSwapChain swapChain;
    WGPUTextureFormat preferredFormat;
    WGPUPresentMode presentMode;
};

}
```

### Layer 2: Resource Management
```cpp
// Buffer abstraction
class WebGPUBuffer {
    WGPUBuffer buffer;
    WGPUBufferUsageFlags usage;
    size_t size;
    bool mapped;
};

// Texture abstraction  
class WebGPUTexture {
    WGPUTexture texture;
    WGPUTextureView defaultView;
    WGPUTextureFormat format;
    WGPUExtent3D extent;
    uint32_t mipLevelCount;
};

// Sampler abstraction
class WebGPUSampler {
    WGPUSampler sampler;
    WGPUFilterMode minFilter;
    WGPUFilterMode magFilter;
    WGPUAddressMode addressMode;
};
```

### Layer 3: Pipeline Management
```cpp
// Shader modules
class WebGPUShaderModule {
    WGPUShaderModule module;
    std::string entryPoint;
    WGPUShaderStageFlags stage;
};

// Render pipeline
class WebGPURenderPipeline {
    WGPURenderPipeline pipeline;
    WGPUPipelineLayout layout;
    std::vector<WGPUBindGroupLayout> bindGroupLayouts;
};

// Compute pipeline
class WebGPUComputePipeline {
    WGPUComputePipeline pipeline;
    WGPUPipelineLayout layout;
};
```

### Layer 4: Command Recording
```cpp
// Command encoder wrapper
class WebGPUCommandEncoder {
    WGPUCommandEncoder encoder;
    
    // Begin render pass
    WebGPURenderPassEncoder BeginRenderPass(const RenderPassDescriptor& desc);
    
    // Begin compute pass
    WebGPUComputePassEncoder BeginComputePass(const ComputePassDescriptor& desc);
    
    // Resource operations
    void CopyBufferToBuffer(Buffer src, Buffer dst, size_t size);
    void CopyBufferToTexture(Buffer src, Texture dst);
    void CopyTextureToTexture(Texture src, Texture dst);
};

// Render pass encoder
class WebGPURenderPassEncoder {
    WGPURenderPassEncoder encoder;
    
    void SetPipeline(RenderPipeline pipeline);
    void SetBindGroup(uint32_t index, BindGroup group);
    void SetVertexBuffer(uint32_t slot, Buffer buffer);
    void SetIndexBuffer(Buffer buffer, IndexFormat format);
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1);
};
```

## 3. WebGPU-Specific Concepts

### Bind Groups (Descriptor Sets)
WebGPU uses bind groups to organize resource bindings:

```cpp
// Bind group layout describes the structure
class WebGPUBindGroupLayout {
    WGPUBindGroupLayout layout;
    std::vector<WGPUBindGroupLayoutEntry> entries;
};

// Bind group contains actual resources
class WebGPUBindGroup {
    WGPUBindGroup bindGroup;
    WGPUBindGroupLayout layout;
    std::vector<WGPUBindGroupEntry> entries;
};
```

### Render Bundles
Pre-recorded command sequences for efficiency:

```cpp
class WebGPURenderBundle {
    WGPURenderBundle bundle;
    
    // Record commands once
    void Record(std::function<void(RenderBundleEncoder&)> recorder);
    
    // Execute many times
    void ExecuteInPass(RenderPassEncoder& encoder);
};
```

### Query Sets
For GPU timing and occlusion queries:

```cpp
class WebGPUQuerySet {
    WGPUQuerySet querySet;
    WGPUQueryType type;
    uint32_t count;
    
    void BeginQuery(uint32_t index);
    void EndQuery(uint32_t index);
    void ResolveQueries(Buffer destination);
};
```

## 4. Frame Structure

### Typical Frame Flow
1. **Acquire swapchain image**
2. **Create command encoder**
3. **Record render passes**
   - Set pipeline
   - Set bind groups
   - Set vertex/index buffers
   - Draw calls
4. **Finish encoding**
5. **Submit to queue**
6. **Present swapchain**

### Multi-threaded Recording
WebGPU supports parallel command recording:

```cpp
class FrameRecorder {
    // Main thread creates encoder
    WebGPUCommandEncoder mainEncoder;
    
    // Worker threads create render bundles
    std::vector<WebGPURenderBundle> workerBundles;
    
    void RecordFrame() {
        // Parallel bundle recording
        parallel_for(workerBundles, [](auto& bundle) {
            bundle.Record(RecordWorkerCommands);
        });
        
        // Main thread combines bundles
        auto renderPass = mainEncoder.BeginRenderPass(desc);
        for (auto& bundle : workerBundles) {
            renderPass.ExecuteBundle(bundle);
        }
        renderPass.End();
    }
};
```

## 5. Memory Management

### Buffer Mapping
WebGPU has explicit buffer mapping:

```cpp
class MappedBuffer {
    WebGPUBuffer buffer;
    void* mappedData = nullptr;
    
    void Map(MapMode mode, size_t offset, size_t size) {
        wgpuBufferMapAsync(buffer, mode, offset, size, 
            [](WGPUBufferMapAsyncStatus status, void* userdata) {
                // Mapping complete callback
            }, this);
    }
    
    void Unmap() {
        wgpuBufferUnmap(buffer);
        mappedData = nullptr;
    }
};
```

### Staging Buffers
For efficient GPU uploads:

```cpp
class StagingBufferPool {
    struct StagingBuffer {
        WebGPUBuffer buffer;
        size_t size;
        bool inUse;
    };
    
    std::vector<StagingBuffer> pool;
    
    StagingBuffer& Acquire(size_t size) {
        // Find or create staging buffer
        for (auto& buffer : pool) {
            if (!buffer.inUse && buffer.size >= size) {
                buffer.inUse = true;
                return buffer;
            }
        }
        // Create new buffer if needed
        return CreateStagingBuffer(size);
    }
    
    void Release(StagingBuffer& buffer) {
        buffer.inUse = false;
    }
};
```

## 6. Synchronization

### Fences and Callbacks
WebGPU uses callbacks for async operations:

```cpp
class AsyncOperation {
    std::promise<void> promise;
    std::future<void> future;
    
    void WaitForCompletion() {
        future.wait();
    }
    
    static void OnComplete(WGPUQueueWorkDoneStatus status, void* userdata) {
        auto* op = static_cast<AsyncOperation*>(userdata);
        op->promise.set_value();
    }
};
```

### Frame Synchronization
```cpp
class FrameSynchronization {
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    
    struct FrameData {
        WebGPUCommandEncoder encoder;
        std::vector<WebGPUBuffer> dynamicBuffers;
        AsyncOperation fence;
    };
    
    FrameData frames[FRAMES_IN_FLIGHT];
    uint32_t currentFrame = 0;
    
    void NextFrame() {
        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
        frames[currentFrame].fence.WaitForCompletion();
    }
};
```

## 7. Error Handling

WebGPU provides detailed error callbacks:

```cpp
class ErrorHandler {
    static void OnDeviceError(WGPUErrorType type, const char* message, void* userdata) {
        switch (type) {
            case WGPUErrorType_Validation:
                LOG_ERROR("Validation Error: {}", message);
                break;
            case WGPUErrorType_OutOfMemory:
                LOG_ERROR("Out of Memory: {}", message);
                break;
            case WGPUErrorType_DeviceLost:
                LOG_ERROR("Device Lost: {}", message);
                // Trigger device recreation
                break;
        }
    }
    
    static void OnDeviceLost(WGPUDeviceLostReason reason, const char* message, void* userdata) {
        LOG_ERROR("Device lost: {} - {}", reason, message);
        // Handle device loss
    }
};
```

## 8. Platform Integration

### Native Window Integration
```cpp
class PlatformSurface {
#ifdef _WIN32
    WGPUSurface CreateFromHWND(HWND hwnd) {
        WGPUSurfaceDescriptorFromWindowsHWND desc = {};
        desc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
        desc.hinstance = GetModuleHandle(nullptr);
        desc.hwnd = hwnd;
        return wgpuInstanceCreateSurface(instance, &desc);
    }
#elif __APPLE__
    WGPUSurface CreateFromMetalLayer(CAMetalLayer* layer) {
        WGPUSurfaceDescriptorFromMetalLayer desc = {};
        desc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
        desc.layer = layer;
        return wgpuInstanceCreateSurface(instance, &desc);
    }
#elif __linux__
    WGPUSurface CreateFromXlib(Display* display, Window window) {
        WGPUSurfaceDescriptorFromXlibWindow desc = {};
        desc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
        desc.display = display;
        desc.window = window;
        return wgpuInstanceCreateSurface(instance, &desc);
    }
#endif
};
```

## 9. Implementation Strategy

### Phase 1: Core Infrastructure
1. Device and adapter initialization
2. Basic buffer and texture creation
3. Simple render pipeline
4. Command recording and submission

### Phase 2: Resource Management
1. Buffer and texture pools
2. Bind group caching
3. Pipeline state caching
4. Dynamic resource updates

### Phase 3: Advanced Features
1. Compute pipelines
2. Render bundles
3. Multi-threaded recording
4. Query sets and timestamps

### Phase 4: Optimizations
1. Memory suballocation
2. Command buffer reuse
3. Descriptor set pooling
4. Pipeline derivatives

## 10. Advantages of WebGPU

1. **Portability**: Single API for web and native
2. **Safety**: Validation layers built-in
3. **Modern**: Designed for current GPU architectures
4. **Explicit**: Clear resource ownership and synchronization
5. **Performant**: Low overhead, multi-threaded friendly
6. **Future-proof**: Extensible design for new features