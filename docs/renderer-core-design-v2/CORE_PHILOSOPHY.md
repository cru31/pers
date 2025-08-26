# Pers Graphics Engine V2 - Core Philosophy

## 핵심 설계 원칙

### 1. 타입 안정성 최우선 (Type Safety First)
```cpp
// ✅ 용도별로 명확한 타입 정의
IVertexBuffer* vertexBuffer;    // 정점 데이터 전용
IIndexBuffer* indexBuffer;       // 인덱스 데이터 전용
IUniformBuffer* uniformBuffer;   // 유니폼 데이터 전용

// 컴파일 타임에 실수 방지
renderPass->SetVertexBuffer(0, indexBuffer);  // 컴파일 에러!
```

### 2. 단일 책임 분리 (Single Responsibility)
```cpp
// IDevice: 디바이스 관리와 기본 기능만
interface IDevice {
    IQueue* GetQueue();
    IResourceFactory* GetResourceFactory();
    ICommandEncoder* CreateCommandEncoder();
}

// IResourceFactory: 모든 리소스 생성 전담
interface IResourceFactory {
    IVertexBuffer* CreateVertexBuffer(const VertexBufferDesc&);
    IIndexBuffer* CreateIndexBuffer(const IndexBufferDesc&);
    ITexture2D* CreateTexture2D(const Texture2DDesc&);
    // ... 모든 리소스 생성
}
```

### 3. 명시적 제어 (Explicit Control)
```cpp
// 모든 GPU 동작이 명시적
auto encoder = device->CreateCommandEncoder();
auto renderPass = encoder->BeginRenderPass(target);
renderPass->SetPipeline(pipeline);
renderPass->SetVertexBuffer(0, vertexBuffer);
renderPass->Draw(vertexCount);
renderPass->End();
auto commands = encoder->Finish();
queue->Submit(commands);
```

### 4. 의미론적 명확성 (Semantic Clarity)
```cpp
// 각 타입이 명확한 의미를 가짐
IVertexBuffer      // 정점 데이터를 담는 버퍼
IIndexBuffer       // 인덱스를 담는 버퍼  
IUniformBuffer     // 유니폼 변수를 담는 버퍼
IStorageBuffer     // 읽기/쓰기 가능한 저장소 버퍼

ITexture2D         // 2D 텍스처
ITexture3D         // 3D 볼륨 텍스처
ITextureCube       // 큐브맵 텍스처
ITextureArray      // 텍스처 배열
```

## GPU 실행 모델

### Triple Buffering Pipeline
```
Frame N-2          Frame N-1          Frame N
[Presenting] ----> [GPU Executing] ----> [CPU Recording]
     ↑                   ↑                     ↑
  화면 표시중        GPU가 그리는 중       CPU가 명령 기록 중
```

### 명령 기록 및 제출 플로우
```cpp
// 1. Device에서 CommandEncoder 생성
auto encoder = device->CreateCommandEncoder();

// 2. RenderPass 시작 및 명령 기록
{
    auto renderPass = encoder->BeginRenderPass(desc);
    renderPass->SetGraphicsPipeline(pipeline);
    renderPass->SetVertexBuffer(0, vertexBuffer);
    renderPass->SetIndexBuffer(indexBuffer);
    renderPass->DrawIndexed(indexCount);
    renderPass->End();
}

// 3. 명령 완성
auto commandBuffer = encoder->Finish();

// 4. Queue에 제출
device->GetQueue()->Submit(commandBuffer);
```

## 리소스 관리 체계

### 리소스 생성 계층
```
Device
  └─> ResourceFactory (리소스 생성 전담)
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

### 리소스 생명주기
```cpp
// 생성: ResourceFactory가 담당
auto factory = device->GetResourceFactory();
auto vertexBuffer = factory->CreateVertexBuffer(desc);

// 사용: 명시적 타입으로 안전하게 사용
renderPass->SetVertexBuffer(0, vertexBuffer);

// 해제: shared_ptr 자동 관리
// vertexBuffer가 scope를 벗어나면 자동 해제
```

## 백엔드 추상화 전략

### 인터페이스와 구현 분리
```cpp
// 인터페이스 (백엔드 독립적)
interface IVertexBuffer {
    virtual size_t GetSize() = 0;
    virtual size_t GetStride() = 0;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
}

// WebGPU 구현
class WebGPUVertexBuffer : public IVertexBuffer {
    WGPUBuffer buffer;  // WebGPU는 통합 버퍼
    BufferUsage usage = BufferUsage::Vertex;
}

// Vulkan 구현
class VulkanVertexBuffer : public IVertexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}
```

## 에러 처리 철학

### Fail-Fast with Clear Messages
```cpp
void RenderPassEncoder::SetVertexBuffer(uint32_t slot, IVertexBuffer* buffer) {
    if (!buffer) {
        throw std::invalid_argument("Vertex buffer cannot be null");
    }
    if (slot >= MAX_VERTEX_BUFFERS) {
        throw std::out_of_range("Vertex buffer slot exceeds maximum");
    }
    // 타입은 이미 컴파일 타임에 검증됨
}
```

### 디버그 빌드 추가 검증
```cpp
#ifdef DEBUG
void ValidatePipeline(IGraphicsPipeline* pipeline) {
    // 파이프라인 상태 검증
    // 셰이더 호환성 검증
    // 리소스 바인딩 검증
}
#endif
```

## 성능 최적화 원칙

### 1. 리소스 생성 최적화
- ResourceFactory에서 중앙 집중 캐싱
- 동일한 Descriptor로 재생성 방지
- 리소스 풀링 및 재사용

### 2. 타입별 최적화
```cpp
// VertexBuffer는 GPU 읽기 최적화
class WebGPUVertexBuffer {
    // GPU_READ 최적화된 메모리 할당
}

// UniformBuffer는 CPU 쓰기 최적화  
class WebGPUUniformBuffer {
    // CPU_WRITE 최적화된 메모리 할당
    // Dynamic offset 지원
}
```

### 3. 명령 기록 최적화
- CommandEncoder 재사용
- RenderBundle로 반복 명령 최적화
- Draw call 배칭

## 확장성 고려

### 새 리소스 타입 추가
```cpp
// 새로운 버퍼 타입 추가 시
interface IIndirectBuffer : public IBuffer {
    virtual uint32_t GetDrawCount() = 0;
}

// ResourceFactory에 추가
interface IResourceFactory {
    // ... 기존 메서드들 ...
    IIndirectBuffer* CreateIndirectBuffer(const IndirectBufferDesc&);
}
```

### 새 백엔드 추가
```cpp
// 새 백엔드는 모든 인터페이스 구현
class MetalDevice : public IDevice { }
class MetalResourceFactory : public IResourceFactory { }
class MetalVertexBuffer : public IVertexBuffer { }
// ... 모든 리소스 타입별 구현
```

---

이 철학은 **타입 안정성**, **명확한 책임 분리**, **의미론적 명확성**을 최우선으로 합니다.