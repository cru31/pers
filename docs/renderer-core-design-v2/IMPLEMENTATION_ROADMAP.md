# Pers Graphics Engine V2 - Implementation Roadmap

## 개발 철학: Interface First, Implementation Later

```
1. 인터페이스 정의 → 2. Mock 구현 → 3. 테스트 작성 → 4. 실제 구현
```

---

## Phase 0: Project Setup (3일)

### 목표
프로젝트 구조 및 빌드 시스템 구축

### 작업 내용
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

### 체크리스트
- [ ] CMake 프로젝트 구성
- [ ] 폴더 구조 생성
- [ ] 기본 타입 정의 (Types.h)
- [ ] TodoOrDie 헬퍼

---

## Phase 1: Core Interfaces (1주)

### 목표
핵심 인터페이스 정의 및 Mock 구현

### 1.1 인터페이스 정의
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
    // ... 모든 리소스 타입별 생성 메서드
};
```

### 1.2 Mock 구현
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

### 1.3 첫 번째 테스트
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

### 체크리스트
- [ ] 모든 인터페이스 헤더 파일 작성
- [ ] Mock 구현체 작성
- [ ] 기본 단위 테스트 10개 이상
- [ ] 문서화 (Doxygen 주석)

---

## Phase 2: Buffer System (1주)

### 목표
타입별 Buffer 시스템 구현

### 2.1 Buffer 타입 구현
```cpp
// 각 버퍼 타입별 구현
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

### 2.2 ResourceFactory Buffer 생성
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

### 2.3 Buffer 테스트
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

### 체크리스트
- [ ] IVertexBuffer 구현
- [ ] IIndexBuffer 구현  
- [ ] IUniformBuffer 구현
- [ ] IStorageBuffer 구현
- [ ] IIndirectBuffer 구현
- [ ] Buffer 매핑/언매핑 테스트
- [ ] 데이터 업로드 테스트

---

## Phase 3: Texture System (1주)

### 목표
타입별 Texture 시스템 구현

### 3.1 Texture 타입 구현
```cpp
class WebGPUTexture2D : public ITexture2D {
    WGPUTexture texture;
    WGPUTextureView view;
    uint32_t width, height;
    TextureFormat format;
};

class WebGPUTextureCube : public ITextureCube {
    WGPUTexture texture;
    WGPUTextureView views[6];  // 각 면별 view
    uint32_t size;
};
```

### 3.2 Texture 생성 및 업데이트
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

### 체크리스트
- [ ] ITexture2D 구현
- [ ] ITexture3D 구현
- [ ] ITextureCube 구현
- [ ] ITextureArray 구현
- [ ] IRenderTexture 구현
- [ ] 텍스처 업로드 테스트
- [ ] Mipmap 생성 테스트

---

## Phase 4: Pipeline System (2주)

### 목표
Graphics & Compute Pipeline 구현

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

### 4.3 Pipeline 생성 테스트
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

### 체크리스트
- [ ] IShaderModule 구현
- [ ] IGraphicsPipeline 구현
- [ ] IComputePipeline 구현
- [ ] IPipelineLayout 구현
- [ ] IBindGroupLayout 구현
- [ ] Pipeline 캐싱 시스템
- [ ] Shader 컴파일 에러 처리

---

## Phase 5: Command System (2주)

### 목표
Command Recording 시스템 구현

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

### 체크리스트
- [ ] ICommandEncoder 구현
- [ ] IRenderPassEncoder 구현
- [ ] IComputePassEncoder 구현
- [ ] ICommandBuffer 구현
- [ ] Draw 명령 테스트
- [ ] Resource 바인딩 테스트
- [ ] Multi-pass 렌더링 테스트

---

## Phase 6: First Triangle (1주)

### 목표
실제 삼각형 렌더링

### 6.1 Triangle 샘플
```cpp
void RenderTriangle() {
    // 리소스 생성
    auto factory = device->GetResourceFactory();
    auto vertexBuffer = factory->CreateVertexBuffer(triangleVertices);
    auto pipeline = factory->CreateGraphicsPipeline(trianglePipelineDesc);
    
    // 렌더링
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

### 체크리스트
- [ ] Window/Surface 생성
- [ ] SwapChain 구성
- [ ] Triangle vertex data
- [ ] Basic shader (WGSL)
- [ ] 렌더링 루프
- [ ] 화면에 삼각형 표시

---

## Phase 7: Advanced Features (3주)

### 목표
고급 기능 구현

### 7.1 Uniform & Binding
- [ ] Uniform buffer 업데이트
- [ ] BindGroup 시스템
- [ ] Dynamic offsets
- [ ] Push constants

### 7.2 Texturing
- [ ] 텍스처 샘플링
- [ ] Mipmap generation
- [ ] Cube mapping
- [ ] Texture arrays

### 7.3 Advanced Rendering
- [ ] Instanced rendering
- [ ] Indirect drawing
- [ ] Compute shaders
- [ ] RenderBundle

---

## Phase 8: Optimization (2주)

### 목표
성능 최적화 및 안정화

### 8.1 최적화 목록
- [ ] CommandEncoder 재사용 풀
- [ ] Pipeline state 캐싱
- [ ] Resource 바인딩 최적화
- [ ] Memory allocator 구현

### 8.2 프로파일링
- [ ] GPU 타이머 쿼리
- [ ] 메모리 사용량 추적
- [ ] Draw call 통계
- [ ] Pipeline 통계

---

## 마일스톤

### Milestone 1: Mock System (2주)
- [x] 프로젝트 구조
- [ ] 인터페이스 정의
- [ ] Mock 구현
- [ ] 기본 테스트

### Milestone 2: Resource System (2주)
- [ ] Buffer 시스템
- [ ] Texture 시스템
- [ ] ResourceFactory

### Milestone 3: Pipeline System (2주)
- [ ] Shader modules
- [ ] Graphics pipeline
- [ ] Compute pipeline

### Milestone 4: First Triangle (3주)
- [ ] Command system
- [ ] Basic rendering
- [ ] SwapChain

### Milestone 5: Production Ready (3주)
- [ ] Advanced features
- [ ] Optimization
- [ ] Documentation
- [ ] 50+ 테스트 통과

---

## 성공 지표

### 코드 품질
- 테스트 커버리지 > 80%
- 모든 public API 문서화
- 컴파일 경고 0

### 성능
- Triangle: < 1ms CPU time
- 1000 draw calls: < 16ms
- Memory usage: < 100MB base

### 사용성
- 10줄로 Triangle 렌더링
- 명확한 에러 메시지
- 완전한 타입 안정성

---

이 로드맵을 따라 12주 안에 Production-Ready 렌더러를 완성할 수 있습니다.