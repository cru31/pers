# WebGPU Backend Architecture v2

## Design Philosophy

LearnWebGPU와 another_design의 핵심 개념을 결합하여 사용자 친화적이면서도 효율적인 WebGPU 백엔드를 설계합니다.

### Core Principles
1. **Progressive Disclosure**: 간단한 작업은 간단하게, 복잡한 작업은 가능하게
2. **Zero-Cost Abstraction**: 추상화가 성능에 영향을 주지 않음
3. **Automatic Resource Management**: RAII와 reference counting으로 자동 관리
4. **Declarative API**: 명령형이 아닌 선언적 인터페이스

## Layered Architecture

```
┌─────────────────────────────────────────┐
│         Application Layer                │  ← 사용자 코드
│    Scene, Entity, Component, Camera      │
├─────────────────────────────────────────┤
│         High-Level Renderer              │  ← 자동 최적화
│    SceneRenderer, RenderGraph, Culling   │
├─────────────────────────────────────────┤
│         Resource Management              │  ← 자동 관리
│    MaterialSystem, MeshManager, Loader   │
├─────────────────────────────────────────┤
│         Abstraction Layer                │  ← 플랫폼 독립
│    IDevice, CommandBuffer, Pipeline      │
├─────────────────────────────────────────┤
│         WebGPU Backend                   │  ← WebGPU 구현
│    WebGPUDevice, WebGPUCommandEncoder    │
├─────────────────────────────────────────┤
│         WebGPU Native API                │  ← wgpu-native
└─────────────────────────────────────────┘
```

## Application Layer API

### Simple Usage Example
```cpp
class MyApp : public prism::Application {
    void OnInitialize() override {
        // 1줄로 씬 설정
        scene = CreateScene();
        
        // 간단한 오브젝트 생성
        auto cube = scene->CreateEntity("Cube");
        cube->AddComponent<MeshRenderer>(PrimitiveMesh::Cube);
        cube->AddComponent<Transform>().position = {0, 0, 0};
        
        // 카메라 생성
        auto camera = scene->CreateCamera();
        camera->LookAt({5, 5, 5}, {0, 0, 0});
        
        // 조명 추가
        auto light = scene->CreateLight(LightType::Directional);
        light->direction = {-1, -1, -1};
    }
    
    void OnRender() override {
        // 자동으로 모든 것을 처리
        GetRenderer()->RenderScene(scene);
    }
};
```

### Advanced Usage Example
```cpp
class AdvancedApp : public prism::Application {
    void OnInitialize() override {
        // 커스텀 렌더 파이프라인 설정
        auto& renderer = GetRenderer();
        renderer.SetPipeline(CreateCustomPipeline());
        
        // 커스텀 머티리얼 생성
        auto material = Material::Create()
            ->SetShader("custom.wgsl")
            ->SetTexture("albedo", "texture.jpg")
            ->SetParameter("roughness", 0.5f);
        
        // 대량의 인스턴스 생성
        auto instances = scene->CreateInstancedMesh("grass.obj", 10000);
        instances->SetMaterial(material);
        instances->Scatter(terrain);  // 자동 배치
    }
    
    RenderPipelinePtr CreateCustomPipeline() {
        return RenderPipeline::Builder()
            ->AddPass<ShadowPass>()
            ->AddPass<GBufferPass>()
            ->AddPass<LightingPass>()
            ->AddPass<SSAOPass>()
            ->AddPass<PostProcessPass>()
            ->Build();
    }
};
```

## WebGPU Backend Implementation

### Device Abstraction
```cpp
class WebGPUDevice : public IDevice {
public:
    // 초기화는 내부에서 자동 처리
    bool Initialize(const DeviceConfig& config) override {
        // WebGPU instance 생성
        CreateInstance();
        
        // 최적의 adapter 선택 (자동)
        SelectBestAdapter();
        
        // Device와 Queue 생성
        CreateDeviceAndQueue();
        
        // SwapChain 설정
        CreateSwapChain(config.window);
        
        // 기본 리소스 생성
        InitializeDefaultResources();
        
        return true;
    }
    
    // 고수준 렌더링 인터페이스
    void RenderFrame(const FrameData& frameData) override {
        auto encoder = BeginFrame();
        
        // RenderGraph가 자동으로 패스 순서 결정
        renderGraph->Execute(encoder, frameData);
        
        EndFrame(encoder);
    }
    
private:
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::SwapChain swapChain;
    
    // 자동 관리 시스템
    std::unique_ptr<ResourcePool> resourcePool;
    std::unique_ptr<PipelineCache> pipelineCache;
    std::unique_ptr<BindGroupAllocator> bindGroupAllocator;
};
```

### Automatic Resource Management
```cpp
class ResourcePool {
public:
    // 자동 버퍼 관리
    BufferHandle GetUniformBuffer(size_t size) {
        // 링 버퍼에서 자동 할당
        auto allocation = uniformRingBuffer.Allocate(size);
        return BufferHandle(allocation);
    }
    
    // 자동 텍스처 캐싱
    TextureHandle GetTexture(const std::string& path) {
        if (auto cached = textureCache.Get(path)) {
            return cached;
        }
        
        // 비동기 로딩 및 업로드
        auto future = LoadTextureAsync(path);
        return TextureHandle(future);
    }
    
    // 프레임별 자동 정리
    void BeginFrame(uint32_t frameIndex) {
        // 이전 프레임 리소스 재활용
        transientPools[frameIndex].Reset();
    }
    
private:
    RingBuffer uniformRingBuffer;
    LRUCache<std::string, Texture> textureCache;
    std::array<TransientPool, 3> transientPools;  // Triple buffering
};
```

### Material System
```cpp
class Material {
public:
    // Fluent API for easy setup
    static MaterialPtr Create(const std::string& shader = "default") {
        return std::make_shared<Material>(shader);
    }
    
    MaterialPtr SetTexture(const std::string& name, const std::string& path) {
        textures[name] = ResourceManager::LoadTexture(path);
        return shared_from_this();
    }
    
    MaterialPtr SetParameter(const std::string& name, float value) {
        parameters[name] = value;
        UpdateUniformBuffer();
        return shared_from_this();
    }
    
    // 내부에서 자동으로 처리
    void Bind(CommandEncoder& encoder) {
        // Pipeline 자동 선택 및 캐싱
        encoder.SetPipeline(GetOrCreatePipeline());
        
        // BindGroup 자동 생성 및 바인딩
        encoder.SetBindGroup(0, GetOrCreateBindGroup());
    }
    
private:
    PipelineHandle GetOrCreatePipeline() {
        // 현재 상태에 맞는 파이프라인 자동 생성/캐싱
        PipelineKey key = ComputePipelineKey();
        return PipelineCache::Get(key);
    }
};
```

### RenderGraph System
```cpp
class RenderGraph {
public:
    // 선언적 패스 정의
    template<typename PassType>
    PassBuilder& AddPass(const std::string& name) {
        auto pass = std::make_unique<PassType>();
        passes.push_back(std::move(pass));
        return PassBuilder(*this, passes.back().get());
    }
    
    // 자동 의존성 해결 및 실행
    void Execute(CommandEncoder& encoder, const FrameData& frameData) {
        // 자동으로 패스 순서 결정
        auto sortedPasses = TopologicalSort();
        
        // 자동 리소스 배리어 삽입
        for (auto& pass : sortedPasses) {
            InsertBarriers(pass);
            pass->Execute(encoder, frameData);
        }
    }
    
private:
    std::vector<std::unique_ptr<RenderPass>> passes;
    ResourceDependencyGraph dependencyGraph;
};

// 사용 예
renderGraph.AddPass<ShadowPass>("Shadow")
    .Output("ShadowMap")
    .Camera(shadowCamera);

renderGraph.AddPass<GBufferPass>("GBuffer")
    .Output("GBuffer.Albedo", "GBuffer.Normal", "GBuffer.Depth")
    .Camera(mainCamera);

renderGraph.AddPass<LightingPass>("Lighting")
    .Input("GBuffer.*", "ShadowMap")
    .Output("HDRColor");
```

### Automatic Batching & Instancing
```cpp
class DrawCallOptimizer {
public:
    void SubmitDraw(const DrawCall& draw) {
        // 자동 배칭 검사
        if (CanBatch(lastDraw, draw)) {
            BatchDraw(draw);
        }
        // 자동 인스턴싱 검사
        else if (CanInstance(draw)) {
            instanceBuffer.Add(draw);
        }
        else {
            FlushBatches();
            ExecuteDraw(draw);
        }
        lastDraw = draw;
    }
    
    void Flush() {
        // 인스턴스 버퍼 플러시
        if (!instanceBuffer.Empty()) {
            ExecuteInstanced(instanceBuffer);
            instanceBuffer.Clear();
        }
    }
    
private:
    bool CanBatch(const DrawCall& a, const DrawCall& b) {
        return a.pipeline == b.pipeline && 
               a.material == b.material &&
               a.vertexBuffer == b.vertexBuffer;
    }
};
```

## Memory Management Strategy

### Triple Buffering
```cpp
class FrameManager {
    static constexpr uint32_t FRAMES_IN_FLIGHT = 3;
    
    struct FrameData {
        wgpu::CommandEncoder encoder;
        UniformBuffer uniformBuffer;
        TransientPool transientPool;
        Fence fence;
    };
    
    FrameData frames[FRAMES_IN_FLIGHT];
    uint32_t currentFrame = 0;
    
    void BeginFrame() {
        // 2프레임 전 완료 대기
        frames[currentFrame].fence.Wait();
        
        // 현재 프레임 리소스 재설정
        frames[currentFrame].uniformBuffer.Reset();
        frames[currentFrame].transientPool.Reset();
    }
    
    void EndFrame() {
        // 제출 및 펜스 시그널
        queue.Submit(frames[currentFrame].encoder.Finish());
        frames[currentFrame].fence.Signal();
        
        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
    }
};
```

## Performance Optimizations

### 1. Automatic LOD Selection
```cpp
class LODSystem {
    void SelectLOD(const Entity& entity, const Camera& camera) {
        float distance = Distance(entity.position, camera.position);
        
        // 자동 LOD 선택
        if (distance < 10) entity.SetLOD(0);      // High detail
        else if (distance < 50) entity.SetLOD(1); // Medium
        else if (distance < 100) entity.SetLOD(2); // Low
        else entity.Cull();                        // Too far
    }
};
```

### 2. Automatic Culling
```cpp
class CullingSystem {
    void PerformCulling(const Scene& scene, const Camera& camera) {
        auto frustum = camera.GetFrustum();
        
        // Hierarchical culling with BVH
        scene.bvh.Traverse([&](const Node& node) {
            if (!frustum.Intersects(node.bounds)) {
                return false;  // Skip children
            }
            
            // GPU occlusion queries for large objects
            if (node.IsMassive()) {
                SubmitOcclusionQuery(node);
            }
            
            return true;  // Continue traversal
        });
    }
};
```

### 3. Shader Variant System
```cpp
class ShaderVariantSystem {
    wgpu::ShaderModule GetOrCreateVariant(
        const std::string& source,
        const ShaderFeatures& features) {
        
        // 자동 변형 생성
        uint64_t key = HashFeatures(features);
        if (auto cached = variantCache[key]) {
            return cached;
        }
        
        // 전처리 및 컴파일
        std::string processed = PreprocessShader(source, features);
        auto module = CompileWGSL(processed);
        
        variantCache[key] = module;
        return module;
    }
};
```

## Error Handling

### Graceful Degradation
```cpp
class ErrorHandler {
    void HandleDeviceLost() {
        // 자동 복구 시도
        Logger::Warning("Device lost, attempting recovery...");
        
        // 중요 상태 저장
        SaveCriticalState();
        
        // 디바이스 재생성
        RecreateDevice();
        
        // 리소스 복원
        RestoreResources();
        
        // 사용자에게 알림
        NotifyUser("Graphics device was reset");
    }
    
    void HandleOutOfMemory() {
        // 자동 메모리 정리
        ResourceCache::Clear();
        TextureCache::ReduceQuality();
        MeshCache::UnloadDistant();
        
        // 재시도
        RetryLastOperation();
    }
};
```

## Summary

이 설계는:
1. **사용자 친화적**: 간단한 작업은 몇 줄의 코드로 가능
2. **자동 최적화**: 배칭, 인스턴싱, 컬링 자동 처리
3. **확장 가능**: 필요시 로우레벨 접근 가능
4. **성능 중심**: Zero-cost abstraction, 자동 리소스 관리
5. **WebGPU 네이티브**: Learn WebGPU의 베스트 프랙티스 적용