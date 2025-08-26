# Command-Based Renderer API 초기 컨셉 디자인

## 개요

모던 그래픽스 API(WebGPU, Vulkan, Metal, D3D12)의 커맨드 기반 렌더링 패러다임을 반영한 렌더러 설계입니다. 명시적 제어와 예측 가능성을 최우선으로 하면서도 사용성을 고려한 API를 제공합니다.

## 핵심 설계 원칙

### 1. 명시적 제어 (Explicit Control)
- 모든 렌더링 작업은 명시적으로 시작하고 종료
- 숨겨진 자동 처리 최소화
- 예측 가능한 동작

### 2. Command Recording 패러다임
- GPU 명령을 즉시 실행하지 않고 커맨드 버퍼에 기록
- 프레임 끝에서 일괄 제출
- CPU-GPU 병렬성 극대화

### 3. RenderPass 중심 설계
- 모든 렌더링은 RenderPass 내에서 수행
- 백버퍼도 일반 RenderTarget과 동일하게 취급
- GPU의 타일 기반 렌더링 최적화 활용

## Core API 설계

### CommandRecorder

렌더링 명령을 기록하는 핵심 인터페이스입니다.

```cpp
class CommandRecorder {
public:
    // RenderPass 관리
    void BeginRenderPass(RenderTarget* target = nullptr, const ClearValue& clear = {});
    void EndRenderPass();
    
    // 렌더링 명령
    void RenderScene(Scene* scene, Camera* camera = nullptr);
    void SetPipeline(Pipeline* pipeline);
    void SetBindGroup(uint32_t index, BindGroup* group);
    void SetVertexBuffer(Buffer* buffer, uint32_t slot = 0);
    void SetIndexBuffer(Buffer* buffer, IndexFormat format = IndexFormat::UInt32);
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1);
    
    // 리소스 명령
    void CopyTexture(Texture* src, Texture* dst);
    void UpdateBuffer(Buffer* buffer, const void* data, size_t size);
    
    // 상태 확인
    bool IsInRenderPass() const;
};
```

### Renderer

프레임 수명과 스왑체인을 관리하는 최상위 인터페이스입니다.

```cpp
class Renderer {
public:
    // 프레임 관리
    CommandRecorder* BeginFrame();
    void EndFrame(CommandRecorder* recorder);
    
    // 백버퍼 접근 (필요시)
    RenderTarget* GetCurrentBackbuffer() const;
    
    // 디바이스 정보
    const DeviceCapabilities& GetCapabilities() const;
};
```

## 설계 결정과 근거

### 왜 `BeginRenderPass(nullptr)`인가?

#### 고민했던 대안들

1. **백버퍼 특별 취급**
```cpp
// 고려했던 대안
cmd->BeginBackbufferPass();
cmd->BeginRenderTargetPass(target);
```
**거부 이유**: API 중복, 백버퍼도 결국 RenderTarget

2. **암묵적 백버퍼**
```cpp
// 고려했던 대안
cmd->BeginFrame(backbuffer);  // 백버퍼를 프레임과 연결
cmd->Render();  // 자동으로 백버퍼에
```
**거부 이유**: 멀티패스 시나리오에서 혼란, 명시성 부족

#### 선택한 이유

```cpp
cmd->BeginRenderPass(nullptr);  // nullptr = 백버퍼
cmd->BeginRenderPass(shadowMap);  // 명시적 타겟
```

1. **일관성**: 백버퍼도 단순히 RenderTarget 중 하나
2. **GPU API 일치**: 실제 GPU는 백버퍼를 특별 취급하지 않음
3. **간결성**: nullptr이라는 명확한 기본값 규칙

### 왜 명시적 EndRenderPass인가?

#### 고민했던 대안들

1. **자동 종료**
```cpp
// 고려했던 대안
cmd->BeginRenderPass(target1);
cmd->BeginRenderPass(target2);  // 자동으로 이전 패스 종료
// ...
renderer->EndFrame(cmd);  // 마지막 패스 자동 종료
```
**거부 이유**: 암묵적 동작, 예측 불가능, 디버깅 어려움

2. **RAII 패턴**
```cpp
// 고려했던 대안
{
    auto pass = cmd->BeginRenderPass();
    // ...
}  // 자동 종료
```
**거부 이유**: C++ 특정 패턴, 명시성 부족

#### 선택한 이유

```cpp
cmd->BeginRenderPass();
// ... 렌더링 ...
cmd->EndRenderPass();  // 반드시 명시적 호출
```

1. **명확성**: 패스의 시작과 끝이 명확
2. **검증 가능**: 컴파일/런타임에 검증 가능
3. **GPU 최적화**: 명시적 경계로 타일 기반 GPU 최적화

### 왜 CommandRecorder를 프레임마다 생성/반환하는가?

#### 고민했던 대안들

1. **영구 CommandBuffer**
```cpp
// 고려했던 대안
renderer->BeginRecording();
renderer->Draw();
renderer->Submit();
```
**거부 이유**: 멀티스레드 기록 불가, 프레임 경계 불명확

2. **즉시 모드**
```cpp
// 고려했던 대안
renderer->Draw();  // 즉시 GPU로 전송
```
**거부 이유**: 성능 문제, 배칭 불가, CPU-GPU 동기화 문제

#### 선택한 이유

```cpp
auto cmd = renderer->BeginFrame();
// ... 기록 ...
renderer->EndFrame(cmd);
```

1. **프레임 경계 명확**: 프레임 시작/끝이 명확
2. **리소스 관리**: 프레임별 리소스 자동 관리
3. **병렬 기록**: 향후 멀티스레드 확장 가능

## 사용 예시

### 기본 렌더링
```cpp
void OnRender() {
    auto cmd = renderer->BeginFrame();
    
    cmd->BeginRenderPass();  // nullptr = 백버퍼
    cmd->RenderScene(scene);
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

### 멀티패스 렌더링
```cpp
void OnRender() {
    auto cmd = renderer->BeginFrame();
    
    // Shadow pass
    cmd->BeginRenderPass(shadowMap, ClearValue::Depth(1.0f));
    cmd->RenderScene(scene, lightCamera);
    cmd->EndRenderPass();
    
    // Main pass
    cmd->BeginRenderPass(nullptr, ClearValue::Black());
    cmd->RenderScene(scene, mainCamera);
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

### 포스트프로세싱
```cpp
void OnRender() {
    auto cmd = renderer->BeginFrame();
    
    // Scene to HDR buffer
    cmd->BeginRenderPass(hdrBuffer);
    cmd->RenderScene(scene);
    cmd->EndRenderPass();
    
    // Bloom
    cmd->BeginRenderPass(bloomBuffer);
    cmd->SetPipeline(bloomExtractPipeline);
    cmd->SetBindGroup(0, hdrTexture);
    cmd->Draw(3);  // Fullscreen triangle
    cmd->EndRenderPass();
    
    // Composite to backbuffer
    cmd->BeginRenderPass();
    cmd->SetPipeline(compositePipeline);
    cmd->SetBindGroup(0, hdrTexture);
    cmd->SetBindGroup(1, bloomTexture);
    cmd->Draw(3);
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

## 에러 처리

### 명시적 검증
```cpp
void Renderer::EndFrame(CommandRecorder* cmd) {
    // RenderPass가 열려있으면 에러
    if (cmd->IsInRenderPass()) {
        throw std::runtime_error("RenderPass not ended before EndFrame");
    }
    
    auto buffer = cmd->Finish();
    queue->Submit(buffer);
    swapChain->Present();
}
```

### Debug 모드 검증
```cpp
class CommandRecorder {
#ifdef DEBUG
    enum class State { Recording, InRenderPass };
    State state = State::Recording;
    
    void BeginRenderPass(...) {
        assert(state == State::Recording);
        state = State::InRenderPass;
        // ...
    }
#endif
};
```

## 향후 확장 가능성

### 1. 멀티스레드 명령 기록
```cpp
// 향후 가능한 확장
auto cmd1 = renderer->CreateSecondaryCommand();
auto cmd2 = renderer->CreateSecondaryCommand();

parallel_for(chunks, [&](auto& chunk) {
    cmd1->RenderChunk(chunk);
});

mainCmd->ExecuteCommands({cmd1, cmd2});
```
[CLASS_HIERARCHY.puml](../renderer-core-design-v2/CLASS_HIERARCHY.puml)
### 2. Compute Pass
```cpp
// 향후 가능한 확장
cmd->BeginComputePass();
cmd->SetComputePipeline(pipeline);
cmd->Dispatch(x, y, z);
cmd->EndComputePass();
```

### 3. 간접 렌더링
```cpp
// 향후 가능한 확장
cmd->DrawIndirect(indirectBuffer, offset);
cmd->DrawIndexedIndirect(indirectBuffer, offset);
```

## 요약

이 설계는 모던 GPU의 커맨드 기반 아키텍처를 충실히 반영하면서도, 사용성을 해치지 않는 균형점을 찾았습니다. 

**핵심 특징:**
- 명시적이고 예측 가능한 API
- GPU 하드웨어 모델과 일치
- 백버퍼를 특별 취급하지 않는 일관성
- 명확한 프레임과 패스 경계
- 향후 확장 가능한 구조

이 API는 초보자도 쉽게 시작할 수 있으면서, 전문가가 필요로 하는 세밀한 제어도 가능합니다.