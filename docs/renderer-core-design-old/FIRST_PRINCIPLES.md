# Pers Graphics Engine - First Principles & Core Design

## 핵심 철학 (Core Philosophy)

### 우리가 해결하려는 문제
모던 GPU는 명령을 즉시 실행하지 않습니다. 대신 명령을 모아서 한 번에 처리합니다.
기존 렌더링 API들은 이를 숨기려 했지만, 우리는 이를 **명시적으로 드러냅니다**.

## First Principles

### 1. 명시적 제어 (Explicit Control)
```cpp
// ❌ 숨겨진 동작
renderer->Draw();  // 언제 GPU에 전달될까? 알 수 없음

// ✅ 명시적 동작
auto cmd = renderer->BeginFrame();
cmd->BeginRenderPass(target);
cmd->Draw();
cmd->EndRenderPass();  // 반드시 명시적으로 종료
renderer->EndFrame(cmd);  // 여기서 GPU에 제출
```

**원칙**: 숨겨진 마법은 없다. 모든 동작은 명시적이고 예측 가능하다.

### 2. 백버퍼는 특별하지 않다
```cpp
// 백버퍼 = 그냥 또 다른 RenderTarget
cmd->BeginRenderPass(nullptr);  // nullptr = 현재 백버퍼
cmd->BeginRenderPass(shadowMap);  // 다른 타겟도 동일한 방식
```

**원칙**: 특수한 경우를 만들지 않는다. 일관된 인터페이스.

### 3. 소유권은 명확하게
```cpp
// ❌ 애매한 소유권
Pipeline* pipeline = CreatePipeline();  // 누가 삭제?

// ✅ 명확한 소유권  
std::shared_ptr<Pipeline> pipeline = CreatePipeline();
cmd->SetPipeline(pipeline);  // shared_ptr로 공유
```

**원칙**: Raw pointer 금지. 모든 리소스는 shared_ptr로 관리.

## 코어 로직 (The Core Flow)

### 렌더링의 본질: 3단계 파이프라인

```
Frame N-2          Frame N-1          Frame N
[Presenting] ----> [GPU Executing] ----> [CPU Recording]
     ↑                   ↑                     ↑
  화면 표시중        GPU가 그리는 중       CPU가 명령 기록 중
```

### 핵심 코드 흐름
```cpp
// 1. 프레임 시작 - 백버퍼 획득 & 이전 프레임 대기
auto cmd = renderer->BeginFrame();

// 2. 명령 기록 - CPU에서 명령을 모음
cmd->BeginRenderPass(nullptr);  // 백버퍼에 그리기 시작
{
    cmd->SetPipeline(pipeline);
    cmd->SetBindGroup(0, perFrameData);   // 카메라, 라이트
    cmd->SetBindGroup(1, perMaterialData); // 텍스처, 머티리얼
    cmd->SetBindGroup(2, perObjectData);   // 트랜스폼
    cmd->DrawIndexed(mesh->GetIndexCount());
}
cmd->EndRenderPass();  // 반드시 종료!

// 3. 프레임 종료 - GPU에 제출 & 화면 표시
renderer->EndFrame(cmd);
```

## 리소스 바인딩 철학

### 변경 빈도에 따른 계층 구조
```
Set 0: Per-Frame    (1회/프레임)     - View, Projection, Lights
Set 1: Per-Pass     (3-5회/프레임)   - Pass 설정, RenderTarget 정보  
Set 2: Per-Material (10-100회/프레임) - Textures, Material 속성
Set 3: Per-Object   (100-1000회/프레임) - Model Matrix, Instance 데이터
```

**원칙**: 자주 바뀌는 것과 안 바뀌는 것을 분리. GPU 캐시 최적화.

## 에러 처리 철학

### Fail-Fast 원칙
```cpp
void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) {
    // 즉시 검증, 즉시 실패
    if (cmd->IsInRenderPass()) {
        throw std::runtime_error("RenderPass not ended!");
    }
}
```

**원칙**: 잘못된 사용은 즉시 에러. 디버깅 시간 절약.

## 왜 이렇게 설계했는가?

### 1. GPU와 동일한 멘탈 모델
- GPU는 명령을 큐에 쌓아서 처리합니다
- 우리 API도 동일하게 동작합니다
- 개발자가 GPU를 이해하면 API도 이해됩니다

### 2. 실수 방지
- `BeginRenderPass`했으면 반드시 `EndRenderPass`
- 컴파일러와 런타임이 실수를 잡아줍니다
- 숨겨진 상태 변경이 없어 디버깅이 쉽습니다

### 3. 최적화 가능
- 명령을 모아서 제출 → 배치 최적화 가능
- 리소스 바인딩 계층화 → 상태 변경 최소화
- 명시적 동기화 → 불필요한 대기 제거

## 핵심 인터페이스 (최소한의 API)

```cpp
// 딱 이것만 있으면 됩니다
class IRenderer {
    virtual std::shared_ptr<ICommandRecorder> BeginFrame() = 0;
    virtual void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) = 0;
};

class ICommandRecorder {
    virtual void BeginRenderPass(const std::shared_ptr<RenderTarget>& target) = 0;
    virtual void EndRenderPass() = 0;
    virtual void Draw(...) = 0;
};
```

## 이것만 기억하세요

1. **명시적으로** - 숨기지 말고 드러내라
2. **일관되게** - 특수 케이스를 만들지 마라  
3. **명확하게** - 소유권과 생명주기를 명확히 하라
4. **GPU처럼** - GPU의 동작 방식을 따라라

---

> "Simple is not easy, but simple is worth it."

모든 복잡한 렌더링 엔진도 결국 이 단순한 원칙들로 귀결됩니다.
**Pers**는 이 원칙들을 코드로 구현한 것입니다.