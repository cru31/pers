# Pers Graphics Engine - Validation Methodology & Implementation Plan

## 핵심 검증 원칙 (Core Validation Principles)

### 1. 근본적 문제 정의
**문제**: Pers Graphics Engine이 실제로 올바른 GPU 명령을 생성하는가?

**해결**: Vanilla WebGPU 구현과 Pers Engine 구현이 **동일한 GPU API 호출 시퀀스**를 생성하는지 검증

### 2. 검증 철학
```
"If it generates the same GPU commands, it IS correct."
```

- **동일한 입력** → **동일한 GPU 명령** → **동일한 출력**
- 추상화 레이어가 아무리 복잡해도, 최종적으로 GPU에 전달되는 명령이 같으면 올바른 구현

### 3. Trace-Based Validation 원칙

#### 3.1 투명성 원칙 (Transparency Principle)
- 모든 GPU API 호출은 추적 가능해야 함
- 숨겨진 내부 동작도 로그로 노출
- 디버깅 가능한 형태로 기록

#### 3.2 정규화 원칙 (Normalization Principle)
- 플랫폼/구현체 독립적인 로그 형식
- 타임스탬프는 상대시간으로 정규화
- 포인터/핸들 값은 심볼릭 ID로 변환

#### 3.3 계층적 검증 (Hierarchical Validation)
```
Level 1: API Call Sequence    - 함수 호출 순서만 비교
Level 2: Parameter Values      - 파라미터 값까지 비교  
Level 3: State Validation      - GPU 상태까지 비교
Level 4: Pixel-Perfect         - 최종 렌더링 결과 비교
```

## 검증 방법론 (Validation Methodology)

### Phase 1: Trace Infrastructure (추적 인프라 구축)

#### 목표
- 모든 WebGPU API 호출을 기록할 수 있는 추적 시스템 구축
- Vanilla와 Pers 양쪽에서 동일한 형식으로 로그 생성

#### 구현 전략
```cpp
// 모든 WebGPU 함수를 래핑
#define WRAP_WGPU_CALL(func, ...) \
    TraceLogger::LogCall(#func, ##__VA_ARGS__); \
    return Original_##func(__VA_ARGS__)

// 사용 예
wgpuRenderPassEncoderDraw = WRAP_WGPU_CALL(wgpuRenderPassEncoderDraw, pass, vertexCount, instanceCount);
```

#### 산출물
1. `TraceLogger` 라이브러리
2. WebGPU API 래퍼 생성기
3. 로그 정규화 도구

### Phase 2: Reference Implementation (참조 구현)

#### 목표
- Vanilla WebGPU로 구현한 표준 샘플 세트
- 각 샘플은 Pers Engine이 구현해야 할 기능을 대표

#### 샘플 계층 구조
```
Level 0: Initialization Only     - Device/Queue 생성만
Level 1: Clear Screen            - RenderPass + Clear
Level 2: Triangle                - Pipeline + Draw
Level 3: Textured Quad           - BindGroup + Texture
Level 4: Multiple Objects        - Multiple Draw Calls
Level 5: Multi-Pass              - Shadow + Main Pass
```

#### 각 샘플의 필수 요소
1. **Minimal Code**: 해당 기능만을 위한 최소 코드
2. **Full Tracing**: 모든 API 호출 추적
3. **Deterministic**: 항상 동일한 결과 생성
4. **Self-Contained**: 외부 의존성 없음

### Phase 3: Mock Implementation (모의 구현)

#### 목표
- Pers Engine API를 사용하지만 내부적으로 동일한 WebGPU 호출 생성
- 실제 렌더링 없이 API 호출만 검증

#### 구현 전략
```cpp
class MockRenderer : public IRenderer {
    std::shared_ptr<ICommandRecorder> BeginFrame() override {
        TRACE_CALL("wgpuDeviceCreateCommandEncoder");
        // 실제 WebGPU 호출 없이 로그만 생성
        return std::make_shared<MockCommandRecorder>();
    }
};

class MockCommandRecorder : public ICommandRecorder {
    void BeginRenderPass(const std::shared_ptr<RenderTarget>& target) override {
        TRACE_CALL("wgpuCommandEncoderBeginRenderPass");
        _state = State::InRenderPass;
    }
    
    void Draw(uint32_t vertexCount, uint32_t instanceCount) override {
        ASSERT(_state == State::InRenderPass);
        TRACE_CALL_WITH_PARAMS("wgpuRenderPassEncoderDraw", 
                              vertexCount, instanceCount);
    }
};
```

### Phase 4: Comparison & Validation (비교 및 검증)

#### 목표
- 두 구현의 트레이스를 자동으로 비교
- 차이점을 정확히 식별하고 보고

#### 비교 알고리즘
```python
def compare_traces(vanilla_trace, pers_trace):
    # Step 1: Normalize traces (remove timestamps, normalize handles)
    vanilla_norm = normalize(vanilla_trace)
    pers_norm = normalize(pers_trace)
    
    # Step 2: Structural comparison (call sequence)
    if not compare_structure(vanilla_norm, pers_norm):
        return report_structural_diff()
    
    # Step 3: Parameter comparison
    if not compare_parameters(vanilla_norm, pers_norm):
        return report_parameter_diff()
    
    # Step 4: State validation
    if not compare_states(vanilla_norm, pers_norm):
        return report_state_diff()
    
    return ValidationResult.SUCCESS
```

## 구현 계획 (Implementation Plan)

### Week 1-2: Trace Infrastructure

#### Day 1-3: TraceLogger Core
- [ ] `TraceLogger.h/cpp` 구현
- [ ] Thread-safe 로깅 시스템
- [ ] 정규화된 출력 형식
- [ ] 파일/콘솔 출력 지원

#### Day 4-6: WebGPU Wrapper Generator
- [ ] WebGPU 헤더 파싱 스크립트
- [ ] 자동 래퍼 생성 도구
- [ ] 선택적 트레이싱 (카테고리별)

#### Day 7-10: Testing & Refinement
- [ ] 단위 테스트 작성
- [ ] 성능 측정 (오버헤드 < 5%)
- [ ] 로그 크기 최적화

### Week 3-4: Reference Samples

#### Day 1-2: Level 0 - Initialization
```cpp
// vanilla_00_init.cpp
int main() {
    TRACE_INIT("vanilla_00_init.log");
    
    // Instance
    TRACE_CALL("wgpuCreateInstance");
    WGPUInstance instance = wgpuCreateInstance(nullptr);
    
    // Adapter
    TRACE_CALL("wgpuInstanceRequestAdapter");
    WGPUAdapter adapter = RequestAdapter(instance);
    
    // Device
    TRACE_CALL("wgpuAdapterRequestDevice");
    WGPUDevice device = RequestDevice(adapter);
    
    // Queue
    TRACE_CALL("wgpuDeviceGetQueue");
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    
    // Cleanup
    Cleanup();
}
```

#### Day 3-4: Level 1 - Clear Screen
```cpp
// vanilla_01_clear.cpp
void RenderFrame() {
    TRACE_CALL("wgpuDeviceCreateCommandEncoder");
    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    
    TRACE_CALL("wgpuCommandEncoderBeginRenderPass");
    auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &clearPassDesc);
    
    TRACE_CALL("wgpuRenderPassEncoderEnd");
    wgpuRenderPassEncoderEnd(pass);
    
    TRACE_CALL("wgpuCommandEncoderFinish");
    auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
    
    TRACE_CALL("wgpuQueueSubmit");
    wgpuQueueSubmit(queue, 1, &commands);
}
```

#### Day 5-6: Level 2 - Triangle
- [ ] Shader 컴파일 추적
- [ ] Pipeline 생성 추적
- [ ] Draw 호출 추적

#### Day 7-8: Level 3 - Textured Quad
- [ ] Texture 생성/업로드 추적
- [ ] BindGroup 생성 추적
- [ ] Sampler 설정 추적

#### Day 9-10: Level 4 & 5
- [ ] Multi-object 렌더링
- [ ] Multi-pass 렌더링

### Week 5-6: Mock Implementation

#### Day 1-3: Mock Renderer Core
```cpp
// MockRenderer.cpp
class MockRenderer : public IRenderer {
    std::unordered_map<void*, std::string> _handleMap;
    uint32_t _nextHandle = 0x1000;
    
    void* CreateMockHandle(const std::string& type) {
        void* handle = reinterpret_cast<void*>(_nextHandle++);
        _handleMap[handle] = type;
        TRACE_PARAM(type, handle);
        return handle;
    }
};
```

#### Day 4-6: Command Recording
```cpp
// MockCommandRecorder.cpp
class MockCommandRecorder : public CommandRecorderBase {
    enum class State { Idle, Recording, InRenderPass };
    State _state = State::Recording;
    
    void ValidateState(State expected, const char* operation) {
        if (_state != expected) {
            throw std::runtime_error(
                std::string(operation) + " called in wrong state");
        }
    }
};
```

#### Day 7-10: Pers Sample Implementation
```cpp
// pers_01_clear.cpp
int main() {
    TRACE_INIT("pers_01_clear.log");
    
    auto renderer = CreateMockRenderer();
    
    // One frame
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr, ClearValue{0.2f, 0.3f, 0.4f, 1.0f});
    cmd->EndRenderPass();
    renderer->EndFrame(cmd);
}
```

### Week 7-8: Comparison Tools

#### Day 1-3: Trace Parser
```python
# trace_parser.py
class TraceEntry:
    def __init__(self, line):
        # Parse: [TRACE][0.001][CATEGORY] function | param=value
        self.timestamp = extract_timestamp(line)
        self.category = extract_category(line)
        self.function = extract_function(line)
        self.params = extract_params(line)
    
    def normalize(self):
        # Remove timestamp, normalize handles
        pass
```

#### Day 4-6: Comparison Engine
```python
# trace_compare.py
class TraceComparator:
    def __init__(self, vanilla_trace, pers_trace):
        self.vanilla = parse_trace(vanilla_trace)
        self.pers = parse_trace(pers_trace)
    
    def compare_level1(self):
        # Compare function call sequence only
        vanilla_calls = [e.function for e in self.vanilla]
        pers_calls = [e.function for e in self.pers]
        return vanilla_calls == pers_calls
    
    def compare_level2(self):
        # Compare parameters too
        for v, p in zip(self.vanilla, self.pers):
            if not self.params_match(v.params, p.params):
                return False
        return True
```

#### Day 7-10: Report Generation
```python
# validation_report.py
class ValidationReport:
    def generate_html(self):
        # Side-by-side diff view
        # Highlight differences
        # Statistics summary
        pass
    
    def generate_json(self):
        # Machine-readable format
        # For CI/CD integration
        pass
```

## 검증 케이스 (Test Cases)

### 필수 검증 항목

#### 1. Initialization Sequence
- [ ] Instance → Adapter → Device → Queue 순서
- [ ] 각 단계의 옵션/플래그 일치
- [ ] 에러 처리 경로

#### 2. RenderPass Lifecycle
- [ ] BeginRenderPass 파라미터 (target, clear values)
- [ ] RenderPass 내 상태 변경 순서
- [ ] EndRenderPass 호출 시점

#### 3. Resource Binding
- [ ] BindGroup 인덱스 매핑
- [ ] Dynamic offset 계산
- [ ] 리소스 생명주기

#### 4. Draw Commands
- [ ] Draw vs DrawIndexed
- [ ] Instance count 처리
- [ ] Vertex/Index buffer 바인딩

### 엣지 케이스

#### 1. Error Conditions
```cpp
// RenderPass not ended
cmd->BeginRenderPass();
renderer->EndFrame(cmd); // Should error

// Nested RenderPass
cmd->BeginRenderPass();
cmd->BeginRenderPass(); // Should error
```

#### 2. Resource Conflicts
```cpp
// Same resource as input and output
cmd->BeginRenderPass(texture);
cmd->SetTexture(0, texture); // Should validate
```

#### 3. State Validation
```cpp
// Draw without pipeline
cmd->BeginRenderPass();
cmd->Draw(3); // Should error: No pipeline set
```

## 성공 기준 (Success Criteria)

### Milestone 1: Infrastructure (Week 2)
- [ ] TraceLogger 완성 및 테스트
- [ ] 100+ 트레이스 포인트 정의
- [ ] < 5% 성능 오버헤드

### Milestone 2: Reference Implementation (Week 4)
- [ ] 5개 레벨 샘플 완성
- [ ] 각 샘플 500+ 라인 트레이스
- [ ] 100% 재현 가능한 출력

### Milestone 3: Mock Implementation (Week 6)
- [ ] Pers API로 동일 기능 구현
- [ ] 트레이스 생성 확인
- [ ] 상태 검증 로직 완성

### Milestone 4: Validation Complete (Week 8)
- [ ] 95% 트레이스 일치율
- [ ] 모든 차이점 설명 가능
- [ ] 자동화된 검증 파이프라인

## 리스크 관리 (Risk Management)

### 기술적 리스크

#### 1. 비결정적 동작
**문제**: GPU 드라이버가 명령을 재배치할 수 있음
**해결**: 동기화 포인트 명시, 순서 의존성 문서화

#### 2. 플랫폼 차이
**문제**: Windows/Linux/Mac에서 다른 동작
**해결**: 플랫폼별 정규화 규칙 정의

#### 3. 핸들/포인터 값
**문제**: 실행마다 다른 메모리 주소
**해결**: 심볼릭 ID로 매핑 (0x1234 → "pipeline_1")

### 일정 리스크

#### 1. WebGPU 스펙 변경
**대응**: Dawn/wgpu-native 안정 버전 고정

#### 2. 복잡도 증가
**대응**: 단계적 검증, 점진적 복잡도 증가

## 도구 및 환경 (Tools & Environment)

### 개발 환경
- **컴파일러**: MSVC 2022 / GCC 11+ / Clang 14+
- **WebGPU**: Dawn (Google) 또는 wgpu-native
- **Python**: 3.9+ (비교 도구용)
- **CMake**: 3.20+

### CI/CD 통합
```yaml
# .github/workflows/validation.yml
validation:
  steps:
    - name: Build Vanilla Samples
      run: cmake --build . --target vanilla_samples
    
    - name: Build Pers Samples  
      run: cmake --build . --target pers_samples
    
    - name: Generate Traces
      run: |
        ./vanilla_triangle > vanilla.trace
        ./pers_triangle > pers.trace
    
    - name: Compare Traces
      run: python trace_compare.py vanilla.trace pers.trace
    
    - name: Upload Report
      uses: actions/upload-artifact@v2
      with:
        name: validation-report
        path: validation_report.html
```

## 결론

이 검증 시스템은 Pers Graphics Engine이 **올바른 GPU 명령을 생성한다는 것을 수학적으로 증명**합니다.

### 핵심 이점
1. **신뢰성**: 구현의 정확성을 객관적으로 입증
2. **디버깅**: 문제 발생 시 정확한 위치 파악
3. **회귀 방지**: 변경사항이 동작을 바꾸면 즉시 감지
4. **문서화**: 트레이스 자체가 실행 순서 문서

### 장기 비전
- 모든 그래픽스 API 백엔드에 적용 (Vulkan, Metal, D3D12)
- 성능 프로파일링 통합
- 자동 버그 리포트 생성
- GPU 벤더별 최적화 검증

**"Trust, but Verify"** - 이 시스템으로 우리는 검증합니다.