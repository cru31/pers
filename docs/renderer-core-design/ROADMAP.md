# Pers Graphics Engine - Development Roadmap & Test Plan

## 개발 철학: Top-Down Approach

**핵심**: 먼저 인터페이스를 정의하고, 사용 예제를 작성한 후, 구현을 채워나간다.

```
1. API 설계 → 2. 사용 예제 작성 → 3. 테스트 작성 → 4. 구현
```

---

## Phase 0: Foundation (1주)
> 목표: 프로젝트 구조와 빌드 시스템 구축

### 작업 내용
```
pers/
├── CMakeLists.txt
├── pers/
│   ├── include/pers/
│   │   ├── core/
│   │   │   └── Types.h         # 기본 타입 정의
│   │   └── renderer/
│   │       ├── IRenderer.h     # 인터페이스만
│   │       └── ICommandRecorder.h
│   └── src/
│       └── core/
│           └── NotImplemented.cpp
├── samples/
│   └── 00-basic-setup/         # 빌드 테스트용
└── tests/
    └── CMakeLists.txt
```

### 검증
- [ ] CMake 빌드 성공
- [ ] 빈 프로그램 실행
- [ ] NotImplemented 로그 출력

---

## Phase 1: Core Interfaces (1주)
> 목표: 핵심 인터페이스 정의와 Mock 구현

### 1.1 인터페이스 정의
```cpp
// IRenderer.h
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual std::shared_ptr<ICommandRecorder> BeginFrame() = 0;
    virtual void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) = 0;
    virtual std::shared_ptr<RenderTarget> GetCurrentBackbuffer() const = 0;
};

// ICommandRecorder.h  
class ICommandRecorder {
public:
    virtual void BeginRenderPass(const std::shared_ptr<RenderTarget>& target,
                                 const ClearValue& clear = {}) = 0;
    virtual void EndRenderPass() = 0;
    virtual bool IsInRenderPass() const = 0;
};
```

### 1.2 Mock 구현
```cpp
class MockRenderer : public IRenderer {
    std::shared_ptr<ICommandRecorder> BeginFrame() override {
        Logger::Info("BeginFrame called");
        return std::make_shared<MockCommandRecorder>();
    }
};
```

### 1.3 첫 번째 샘플
```cpp
// samples/01-clear-screen/main.cpp
int main() {
    auto renderer = CreateRenderer(RendererType::Mock);
    
    // 3프레임 렌더링
    for (int i = 0; i < 3; ++i) {
        auto cmd = renderer->BeginFrame();
        cmd->BeginRenderPass(nullptr, ClearValue{0.2f, 0.3f, 0.4f, 1.0f});
        cmd->EndRenderPass();
        renderer->EndFrame(cmd);
    }
}
```

### 테스트
```cpp
TEST(RendererTest, BeginEndFrame) {
    auto renderer = CreateMockRenderer();
    auto cmd = renderer->BeginFrame();
    ASSERT_NE(cmd, nullptr);
    renderer->EndFrame(cmd);
    // 에러 없이 완료되면 성공
}

TEST(RendererTest, RenderPassMustBeEnded) {
    auto renderer = CreateMockRenderer();
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr);
    // EndRenderPass 호출 안 함
    EXPECT_THROW(renderer->EndFrame(cmd), std::runtime_error);
}
```

---

## Phase 2: Resource Management (2주)
> 목표: 리소스 관리 시스템 구축

### 2.1 리소스 타입 정의
```cpp
class Pipeline;      // 셰이더 + 상태
class BindGroup;     // 리소스 묶음
class Buffer;        // 정점/인덱스/유니폼
class Texture;       // 텍스처
class RenderTarget;  // 렌더 타겟
```

### 2.2 리소스 생성 API
```cpp
class IRenderer {
    virtual std::shared_ptr<Pipeline> CreatePipeline(
        const PipelineDesc& desc) = 0;
    virtual std::shared_ptr<Buffer> CreateBuffer(
        const BufferDesc& desc) = 0;
};
```

### 2.3 Triangle 샘플
```cpp
// samples/02-triangle/main.cpp
auto pipeline = renderer->CreatePipeline(trianglePipelineDesc);
auto vertexBuffer = renderer->CreateBuffer(vertices);

auto cmd = renderer->BeginFrame();
cmd->BeginRenderPass(nullptr);
cmd->SetPipeline(pipeline);
cmd->SetVertexBuffer(vertexBuffer);
cmd->Draw(3);
cmd->EndRenderPass();
renderer->EndFrame(cmd);
```

### 테스트
```cpp
TEST(ResourceTest, CreatePipeline) {
    auto pipeline = renderer->CreatePipeline(validDesc);
    ASSERT_NE(pipeline, nullptr);
}

TEST(ResourceTest, BufferUpload) {
    std::vector<float> data = {0, 1, 2, 3};
    auto buffer = renderer->CreateBuffer(BufferDesc{
        .size = data.size() * sizeof(float),
        .data = data.data()
    });
    ASSERT_NE(buffer, nullptr);
}
```

---

## Phase 3: WebGPU Backend (3주)
> 목표: 실제 WebGPU 구현

### 3.1 WebGPU 초기화
```cpp
class WebGPURenderer : public IRenderer {
    wgpu::Device _device;
    wgpu::Queue _queue;
    std::shared_ptr<SwapChain> _swapChain;
};
```

### 3.2 Command Recording
```cpp
class WebGPUCommandRecorder : public CommandRecorderBase {
    wgpu::CommandEncoder _encoder;
    wgpu::RenderPassEncoder _currentPass;
};
```

### 3.3 실제 Triangle 렌더링
- WebGPU 디바이스 생성
- SwapChain 설정
- 실제 GPU 명령 제출

### 테스트
```cpp
TEST(WebGPUTest, DeviceCreation) {
    auto renderer = CreateWebGPURenderer();
    ASSERT_NE(renderer, nullptr);
}

TEST(WebGPUTest, ActualTriangle) {
    // 실제 삼각형이 렌더링되는지 픽셀 검증
    auto pixels = CaptureFramebuffer();
    EXPECT_NE(pixels[centerX][centerY], backgroundColor);
}
```

---

## Phase 4: Scene System (2주)
> 목표: Scene, Camera, Mesh 시스템

### 4.1 Scene 구조
```cpp
class Scene {
    std::vector<std::shared_ptr<Entity>> _entities;
    std::shared_ptr<Camera> _mainCamera;
};

class Entity {
    Transform transform;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};
```

### 4.2 Cube 샘플
```cpp
// samples/03-rotating-cube/main.cpp
auto scene = std::make_shared<Scene>();
scene->AddEntity(CreateCube());
scene->SetCamera(CreatePerspectiveCamera());

// 렌더 루프
cmd->RenderScene(scene);
```

### 테스트
```cpp
TEST(SceneTest, EntityTransform) {
    auto entity = CreateCube();
    entity->SetPosition({1, 2, 3});
    EXPECT_EQ(entity->GetPosition(), glm::vec3(1, 2, 3));
}
```

---

## Phase 5: Multi-Pass Rendering (2주)
> 목표: Shadow Mapping, Post-processing

### 5.1 Shadow Pass
```cpp
// Shadow pass
cmd->BeginRenderPass(shadowMap, depthClear);
cmd->RenderScene(scene, lightCamera);
cmd->EndRenderPass();

// Main pass
cmd->BeginRenderPass(nullptr);
cmd->SetTexture(0, shadowMap);
cmd->RenderScene(scene, mainCamera);
cmd->EndRenderPass();
```

### 테스트
```cpp
TEST(MultiPassTest, ShadowMapGeneration) {
    auto shadowMap = renderer->CreateRenderTarget(1024, 1024, Format::Depth32);
    // Shadow pass 실행 후 깊이 값 검증
}
```

---

## Phase 6: Optimization (2주)
> 목표: 성능 최적화

### 6.1 최적화 목록
- Command Buffer 재사용 (CommandPool)
- Pipeline State Cache
- Dynamic Uniform Buffer
- Instanced Rendering

### 6.2 성능 테스트
```cpp
TEST(PerformanceTest, Draw1000Cubes) {
    auto startTime = GetTime();
    RenderManyCubes(1000);
    auto elapsed = GetTime() - startTime;
    EXPECT_LT(elapsed, 16.0); // 60 FPS
}
```

---

## 테스트 전략

### 1. Unit Tests (매일)
```bash
# 모든 유닛 테스트 실행
ctest --test-dir build
```

### 2. Integration Tests (Phase 완료시)
```bash
# 전체 파이프라인 테스트
./tests/integration/full_pipeline_test
```

### 3. Visual Tests (수동)
```bash
# 각 샘플 실행하여 시각적 검증
./samples/02-triangle/triangle
./samples/03-rotating-cube/cube
```

### 4. Performance Tests (주 1회)
```bash
# 성능 벤치마크
./tests/benchmark/renderer_benchmark
```

---

## 마일스톤 & 검증

### Milestone 1: Mock Renderer (2주)
- [x] 인터페이스 정의
- [x] Mock 구현  
- [x] Clear Screen 샘플
- [ ] 10개 유닛 테스트 통과

### Milestone 2: Triangle (4주)
- [ ] 리소스 관리
- [ ] WebGPU 초기화
- [ ] Triangle 렌더링
- [ ] 20개 테스트 통과

### Milestone 3: 3D Scene (6주)
- [ ] Scene 시스템
- [ ] Camera 
- [ ] Rotating Cube
- [ ] 30개 테스트 통과

### Milestone 4: Production Ready (10주)
- [ ] Shadow Mapping
- [ ] Post-processing
- [ ] 최적화 완료
- [ ] 50개 테스트 통과
- [ ] 60 FPS @ 1000 objects

---

## 성공 지표

1. **코드 품질**
   - 테스트 커버리지 > 80%
   - 모든 public API 문서화
   - 컴파일 경고 0

2. **성능**
   - 1000개 오브젝트 @ 60 FPS
   - Frame time < 16ms
   - GPU 메모리 사용 < 1GB

3. **사용성**
   - 10줄 이내로 Triangle 렌더링
   - 직관적인 API
   - 명확한 에러 메시지

---

## 다음 단계 체크리스트

### Week 1
- [ ] CMake 프로젝트 설정
- [ ] 기본 폴더 구조 생성
- [ ] IRenderer, ICommandRecorder 인터페이스
- [ ] MockRenderer 구현
- [ ] 첫 번째 테스트 작성

### Week 2  
- [ ] Clear Screen 샘플
- [ ] 에러 처리 테스트
- [ ] Logger 시스템
- [ ] CI/CD 설정

이 로드맵을 따라가면 10주 안에 Production-Ready 렌더러를 완성할 수 있습니다.