# Pers Graphics Engine V2 - Implementation Roadmap

## Development Philosophy: Interface First, Implementation Later

```
1. Define Interfaces → 2. Mock Implementation → 3. Write Tests → 4. Real Implementation
```

---

## Phase 0: Project Setup (3 days)

### Goal
Build project structure and build system

### Work Items
```
pers/
├── CMakeLists.txt
├── include/pers/graphics/
│   ├── core/
│   │   ├── IInstance.h
│   │   ├── IPhysicalDevice.h
│   │   ├── ILogicalDevice.h
│   │   └── IQueue.h
│   ├── resources/
│   │   ├── buffers/
│   │   │   ├── IBuffer.h
│   │   │   ├── IVertexBuffer.h
│   │   │   ├── IIndexBuffer.h
│   │   │   └── IUniformBuffer.h
│   │   ├── textures/
│   │   │   ├── ITexture.h
│   │   │   ├── ITexture2D.h
│   │   │   └── ITextureCube.h
│   │   └── IResourceFactory.h
│   └── Types.h
├── src/
│   └── TodoOrDie.cpp
└── tests/
    └── CMakeLists.txt
```

### Checklist
- [ ] CMake project configuration
- [ ] Create folder structure
- [ ] Define basic types (Types.h)
- [ ] TodoOrDie helper

---

## Phase 1: Core Interfaces (1 week)

### Goal
Define core interfaces and Mock implementation

### 1.1 Interface Definition
```cpp
// ILogicalDevice.h
class ILogicalDevice {
    virtual IQueue* GetQueue() = 0;
    virtual IResourceFactory* GetResourceFactory() = 0;
    virtual ICommandEncoder* CreateCommandEncoder() = 0;
};

// IResourceFactory.h
class IResourceFactory {
    virtual IVertexBuffer* CreateVertexBuffer(const VertexBufferDesc&) = 0;
    virtual IIndexBuffer* CreateIndexBuffer(const IndexBufferDesc&) = 0;
    // ... all resource type creation methods
};
```

### 1.2 Mock Implementation
```cpp
class MockLogicalDevice : public ILogicalDevice {
    IQueue* GetQueue() override {
        Logger::Info("MockLogicalDevice::GetQueue");
        return &mockQueue;
    }
};

class MockResourceFactory : public IResourceFactory {
    IVertexBuffer* CreateVertexBuffer(const VertexBufferDesc&) override {
        Logger::Info("Creating mock vertex buffer");
        return new MockVertexBuffer();
    }
};
```

### 1.3 First Tests
```cpp
TEST(DeviceTest, CreateLogicalDevice) {
    auto instance = CreateMockInstance();
    auto physicalDevice = instance->RequestPhysicalDevice();
    auto device = physicalDevice->CreateLogicalDevice();
    
    ASSERT_NE(device, nullptr);
    ASSERT_NE(device->GetQueue(), nullptr);
    ASSERT_NE(device->GetResourceFactory(), nullptr);
}

TEST(ResourceFactoryTest, CreateVertexBuffer) {
    auto factory = device->GetResourceFactory();
    auto vb = factory->CreateVertexBuffer({.size = 1024});
    
    ASSERT_NE(vb, nullptr);
    ASSERT_EQ(vb->GetSize(), 1024);
}
```

### Checklist
- [ ] Write all interface header files
- [ ] Write Mock implementations
- [ ] 10+ basic unit tests
- [ ] Documentation (Doxygen comments)

---

## Phase 2: Buffer System (1 week)

### Goal
Implement type-specific Buffer system

### 2.1 Buffer Type Implementation
```cpp
// Implementation for each buffer type
class WebGPUVertexBuffer : public IVertexBuffer {
    WGPUBuffer buffer;
    size_t vertexCount;
    size_t stride;
    VertexLayout layout;
};

class WebGPUIndexBuffer : public IIndexBuffer {
    WGPUBuffer buffer;
    size_t indexCount;
    IndexFormat format;
};

class WebGPUUniformBuffer : public IUniformBuffer {
    WGPUBuffer buffer;
    bool isDynamic;
    size_t alignment;
};
```

### 2.2 ResourceFactory Buffer Creation
```cpp
class WebGPUResourceFactory : public IResourceFactory {
    IVertexBuffer* CreateVertexBuffer(const VertexBufferDesc& desc) {
        WGPUBufferDescriptor bufferDesc{};
        bufferDesc.size = desc.size;
        bufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return new WebGPUVertexBuffer(buffer, desc);
    }
};
```

### 2.3 Buffer Tests
```cpp
TEST(BufferTest, VertexBufferCreation) {
    VertexBufferDesc desc{
        .size = 1024,
        .stride = sizeof(Vertex),
        .data = vertices.data()
    };
    
    auto vb = factory->CreateVertexBuffer(desc);
    ASSERT_EQ(vb->GetSize(), 1024);
    ASSERT_EQ(vb->GetStride(), sizeof(Vertex));
}

TEST(BufferTest, BufferMapping) {
    auto buffer = factory->CreateUniformBuffer({.size = 256});
    
    void* ptr = buffer->Map();
    ASSERT_NE(ptr, nullptr);
    
    // Write data
    memcpy(ptr, &uniformData, sizeof(uniformData));
    
    buffer->Unmap();
}
```

### Checklist
- [ ] IVertexBuffer implementation
- [ ] IIndexBuffer implementation  
- [ ] IUniformBuffer implementation
- [ ] IStorageBuffer implementation
- [ ] IIndirectBuffer implementation
- [ ] Buffer mapping/unmapping tests
- [ ] Data upload tests

---

## Phase 3: Texture System (1 week)

### Goal
Implement type-specific Texture system

### 3.1 Texture Type Implementation
```cpp
class WebGPUTexture2D : public ITexture2D {
    WGPUTexture texture;
    WGPUTextureView view;
    uint32_t width, height;
    TextureFormat format;
};

class WebGPUTextureCube : public ITextureCube {
    WGPUTexture texture;
    WGPUTextureView views[6];  // View for each face
    uint32_t size;
};
```

### 3.2 Texture Creation and Update
```cpp
ITexture2D* CreateTexture2D(const Texture2DDesc& desc) {
    WGPUTextureDescriptor texDesc{};
    texDesc.dimension = WGPUTextureDimension_2D;
    texDesc.size = {desc.width, desc.height, 1};
    texDesc.format = ConvertFormat(desc.format);
    texDesc.usage = ConvertUsage(desc.usage);
    
    WGPUTexture texture = wgpuDeviceCreateTexture(device, &texDesc);
    return new WebGPUTexture2D(texture, desc);
}
```

### Checklist
- [ ] ITexture2D implementation
- [ ] ITexture3D implementation
- [ ] ITextureCube implementation
- [ ] ITextureArray implementation
- [ ] IRenderTexture implementation
- [ ] Texture upload tests
- [ ] Mipmap generation tests

---

## Phase 4: Pipeline System (2 weeks)

### Goal
Implement Graphics & Compute Pipeline

### 4.1 Shader Module
```cpp
class WebGPUShaderModule : public IShaderModule {
    WGPUShaderModule module;
    ShaderStage stage;
    std::string entryPoint;
};
```

### 4.2 Graphics Pipeline
```cpp
class WebGPUGraphicsPipeline : public IGraphicsPipeline {
    WGPURenderPipeline pipeline;
    std::shared_ptr<IPipelineLayout> layout;
    VertexLayout vertexLayout;
    RasterizationState rasterState;
    DepthStencilState depthStencilState;
    BlendState blendState;
};
```

### 4.3 Pipeline Creation Test
```cpp
TEST(PipelineTest, CreateGraphicsPipeline) {
    GraphicsPipelineDesc desc{
        .vertexShader = LoadShader("vertex.wgsl"),
        .fragmentShader = LoadShader("fragment.wgsl"),
        .vertexLayout = GetVertexLayout(),
        .primitiveTopology = PrimitiveTopology::TriangleList
    };
    
    auto pipeline = factory->CreateGraphicsPipeline(desc);
    ASSERT_NE(pipeline, nullptr);
}
```

### Checklist
- [ ] IShaderModule implementation
- [ ] IGraphicsPipeline implementation
- [ ] IComputePipeline implementation
- [ ] IPipelineLayout implementation
- [ ] IBindGroupLayout implementation
- [ ] Pipeline caching system
- [ ] Shader compilation error handling

---

## Phase 5: Command System (2 weeks)

### Goal
Implement Command Recording system

### 5.1 Command Encoder
```cpp
class WebGPUCommandEncoder : public ICommandEncoder {
    WGPUCommandEncoder encoder;
    std::shared_ptr<WebGPUDevice> device;
    
    IRenderPassEncoder* BeginRenderPass(const RenderPassDesc& desc) {
        WGPURenderPassEncoder passEncoder = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
        return new WebGPURenderPassEncoder(passEncoder);
    }
};
```

### 5.2 Render Pass Encoder
```cpp
class WebGPURenderPassEncoder : public IRenderPassEncoder {
    void SetVertexBuffer(uint32_t slot, IVertexBuffer* buffer, size_t offset) {
        auto* webgpuBuffer = static_cast<WebGPUVertexBuffer*>(buffer);
        wgpuRenderPassEncoderSetVertexBuffer(encoder, slot, webgpuBuffer->GetHandle(), offset);
    }
    
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount) {
        wgpuRenderPassEncoderDrawIndexed(encoder, indexCount, instanceCount, 0, 0, 0);
    }
};
```

### Checklist
- [ ] ICommandEncoder implementation
- [ ] IRenderPassEncoder implementation
- [ ] IComputePassEncoder implementation
- [ ] ICommandBuffer implementation
- [ ] Draw command tests
- [ ] Resource binding tests
- [ ] Multi-pass rendering tests

---

## Phase 6: First Triangle (1 week)

### Goal
Render actual triangle

### 6.1 Triangle Sample
```cpp
void RenderTriangle() {
    // Create resources
    auto factory = device->GetResourceFactory();
    auto vertexBuffer = factory->CreateVertexBuffer(triangleVertices);
    auto pipeline = factory->CreateGraphicsPipeline(trianglePipelineDesc);
    
    // Rendering
    auto encoder = device->CreateCommandEncoder();
    auto renderPass = encoder->BeginRenderPass(renderPassDesc);
    
    renderPass->SetGraphicsPipeline(pipeline);
    renderPass->SetVertexBuffer(0, vertexBuffer);
    renderPass->Draw(3);
    renderPass->End();
    
    auto commands = encoder->Finish();
    device->GetQueue()->Submit(commands);
}
```

### Checklist
- [ ] Window/Surface creation
- [ ] SwapChain configuration
- [ ] Triangle vertex data
- [ ] Basic shader (WGSL)
- [ ] Rendering loop
- [ ] Display triangle on screen

---

## Phase 7: Advanced Features (3 weeks)

### Goal
Implement advanced features

### 7.1 Uniform & Binding
- [ ] Uniform buffer updates
- [ ] BindGroup system
- [ ] Dynamic offsets
- [ ] Push constants

### 7.2 Texturing
- [ ] Texture sampling
- [ ] Mipmap generation
- [ ] Cube mapping
- [ ] Texture arrays

### 7.3 Advanced Rendering
- [ ] Instanced rendering
- [ ] Indirect drawing
- [ ] Compute shaders
- [ ] RenderBundle

---

## Phase 8: Optimization (2 weeks)

### Goal
Performance optimization and stabilization

### 8.1 Optimization List
- [ ] CommandEncoder reuse pool
- [ ] Pipeline state caching
- [ ] Resource binding optimization
- [ ] Memory allocator implementation

### 8.2 Profiling
- [ ] GPU timer queries
- [ ] Memory usage tracking
- [ ] Draw call statistics
- [ ] Pipeline statistics

---

## Milestones

### Milestone 1: Mock System (2 weeks)
- [x] Project structure
- [ ] Interface definitions
- [ ] Mock implementation
- [ ] Basic tests

### Milestone 2: Resource System (2 weeks)
- [ ] Buffer system
- [ ] Texture system
- [ ] ResourceFactory

### Milestone 3: Pipeline System (2 weeks)
- [ ] Shader modules
- [ ] Graphics pipeline
- [ ] Compute pipeline

### Milestone 4: First Triangle (3 weeks)
- [ ] Command system
- [ ] Basic rendering
- [ ] SwapChain

### Milestone 5: Production Ready (3 weeks)
- [ ] Advanced features
- [ ] Optimization
- [ ] Documentation
- [ ] 50+ test passes

---

## Success Metrics

### Code Quality
- Test coverage > 80%
- All public APIs documented
- Zero compile warnings

### Performance
- Triangle: < 1ms CPU time
- 1000 draw calls: < 16ms
- Memory usage: < 100MB base

### Usability
- Triangle rendering in 10 lines
- Clear error messages
- Complete type safety

---

Following this roadmap, we can complete a Production-Ready renderer in 12 weeks.