# Pers Graphics Engine - Test Case Specifications

## 테스트 전략 개요

### 테스트 피라미드
```
         /\
        /  \  E2E Tests (5%)
       /    \  - Full application scenarios
      /------\
     /        \ Integration Tests (25%)
    /          \ - Multi-component interaction
   /------------\
  /              \ Unit Tests (70%)
 /                \ - Individual components
/------------------\
```

### 테스트 원칙
1. **Fast Feedback**: 단위 테스트는 밀리초 단위로 실행
2. **Deterministic**: 항상 동일한 결과 생성
3. **Isolated**: 테스트 간 의존성 없음
4. **Comprehensive**: 모든 코드 경로 커버

## Level 0: Initialization Tests

### TEST_00_01: Device Creation
**목적**: WebGPU 디바이스 생성 검증

**Vanilla WebGPU Trace**:
```
[TRACE][0.000][INSTANCE] wgpuCreateInstance
[TRACE][0.001][INSTANCE] wgpuInstanceRequestAdapter
[TRACE][0.010][ADAPTER] wgpuAdapterRequestDevice
[TRACE][0.020][DEVICE] wgpuDeviceGetQueue
```

**Pers Engine Trace**:
```
[TRACE][0.000][INSTANCE] wgpuCreateInstance
[TRACE][0.001][INSTANCE] wgpuInstanceRequestAdapter
[TRACE][0.010][ADAPTER] wgpuAdapterRequestDevice
[TRACE][0.020][DEVICE] wgpuDeviceGetQueue
```

**검증 항목**:
- [ ] Instance 생성 순서
- [ ] Adapter 요청 파라미터
- [ ] Device 기능 플래그
- [ ] Queue 획득

**예상 결과**: 100% 일치

### TEST_00_02: Device Limits Query
**목적**: 디바이스 제한사항 쿼리 검증

**테스트 코드**:
```cpp
TEST(InitTest, DeviceLimits) {
    auto renderer = CreateRenderer();
    auto limits = renderer->GetDeviceLimits();
    
    EXPECT_GE(limits.maxTextureSize, 4096);
    EXPECT_GE(limits.maxBindGroups, 4);
    EXPECT_GE(limits.maxVertexAttributes, 16);
}
```

## Level 1: Clear Screen Tests

### TEST_01_01: Basic Clear
**목적**: 단순 화면 클리어 검증

**Vanilla Implementation**:
```cpp
void VanillaClear() {
    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    
    WGPURenderPassColorAttachment colorAttachment = {
        .view = backbufferView,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = {0.2f, 0.3f, 0.4f, 1.0f}
    };
    
    WGPURenderPassDescriptor passDesc = {
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };
    
    auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);
    wgpuRenderPassEncoderEnd(pass);
    
    auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &commands);
}
```

**Pers Implementation**:
```cpp
void PersClear() {
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr, ClearValue{0.2f, 0.3f, 0.4f, 1.0f});
    cmd->EndRenderPass();
    renderer->EndFrame(cmd);
}
```

**Expected Trace Comparison**:
```
VANILLA                                    | PERS
-------------------------------------------|-------------------------------------------
wgpuDeviceCreateCommandEncoder            | wgpuDeviceCreateCommandEncoder
wgpuCommandEncoderBeginRenderPass         | wgpuCommandEncoderBeginRenderPass
  target=backbuffer                        |   target=backbuffer
  clearR=0.2                               |   clearR=0.2
  clearG=0.3                               |   clearG=0.3
  clearB=0.4                               |   clearB=0.4
  clearA=1.0                               |   clearA=1.0
wgpuRenderPassEncoderEnd                  | wgpuRenderPassEncoderEnd
wgpuCommandEncoderFinish                  | wgpuCommandEncoderFinish
wgpuQueueSubmit                           | wgpuQueueSubmit
```

**검증 항목**:
- [ ] Command encoder 생성
- [ ] RenderPass 시작 파라미터
- [ ] Clear 색상 값
- [ ] RenderPass 종료
- [ ] Command 제출

### TEST_01_02: Multiple Clears
**목적**: 프레임 내 여러 RenderPass 검증

**테스트 시나리오**:
```cpp
void MultipleClearPasses() {
    auto cmd = renderer->BeginFrame();
    
    // First pass - clear to red
    cmd->BeginRenderPass(target1, ClearValue{1.0f, 0.0f, 0.0f, 1.0f});
    cmd->EndRenderPass();
    
    // Second pass - clear to green
    cmd->BeginRenderPass(target2, ClearValue{0.0f, 1.0f, 0.0f, 1.0f});
    cmd->EndRenderPass();
    
    // Third pass - clear backbuffer to blue
    cmd->BeginRenderPass(nullptr, ClearValue{0.0f, 0.0f, 1.0f, 1.0f});
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

**검증 항목**:
- [ ] 각 RenderPass가 독립적으로 시작/종료
- [ ] 타겟별 clear 값 정확성
- [ ] 명령 순서 보존

## Level 2: Triangle Rendering Tests

### TEST_02_01: Basic Triangle
**목적**: 가장 단순한 삼각형 렌더링

**Pipeline 생성 Trace**:
```
[TRACE][DEVICE] wgpuDeviceCreateShaderModule | type=vertex
[TRACE][DEVICE] wgpuDeviceCreateShaderModule | type=fragment
[TRACE][DEVICE] wgpuDeviceCreateRenderPipeline
  vertexEntry=vs_main
  fragmentEntry=fs_main
  topology=TriangleList
  cullMode=None
```

**렌더링 Trace**:
```
[TRACE][PASS] wgpuRenderPassEncoderSetPipeline | pipeline=0x1001
[TRACE][PASS] wgpuRenderPassEncoderDraw | vertexCount=3 | instanceCount=1
```

**검증 항목**:
- [ ] Shader 모듈 생성
- [ ] Pipeline 설정
- [ ] Draw 파라미터 (3 vertices)

### TEST_02_02: Indexed Triangle
**목적**: 인덱스 버퍼를 사용한 렌더링

**버퍼 생성**:
```cpp
// Vertex data
float vertices[] = {
    -0.5f, -0.5f, 0.0f,  // vertex 0
     0.5f, -0.5f, 0.0f,  // vertex 1
     0.0f,  0.5f, 0.0f   // vertex 2
};

// Index data
uint16_t indices[] = {0, 1, 2};
```

**Expected Trace**:
```
[TRACE][DEVICE] wgpuDeviceCreateBuffer | size=36 | usage=Vertex
[TRACE][QUEUE] wgpuQueueWriteBuffer | buffer=0x2001 | offset=0 | size=36
[TRACE][DEVICE] wgpuDeviceCreateBuffer | size=6 | usage=Index
[TRACE][QUEUE] wgpuQueueWriteBuffer | buffer=0x2002 | offset=0 | size=6
[TRACE][PASS] wgpuRenderPassEncoderSetVertexBuffer | slot=0 | buffer=0x2001
[TRACE][PASS] wgpuRenderPassEncoderSetIndexBuffer | buffer=0x2002 | format=Uint16
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=3 | instanceCount=1
```

## Level 3: Resource Binding Tests

### TEST_03_01: Uniform Buffer Binding
**목적**: Uniform 버퍼 바인딩 검증

**리소스 설정**:
```cpp
struct UniformData {
    glm::mat4 mvpMatrix;
    glm::vec4 color;
};

UniformData uniformData = {
    glm::mat4(1.0f),
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
};
```

**Expected Trace**:
```
[TRACE][DEVICE] wgpuDeviceCreateBuffer | size=80 | usage=Uniform
[TRACE][QUEUE] wgpuQueueWriteBuffer | buffer=0x3001 | size=80
[TRACE][DEVICE] wgpuDeviceCreateBindGroupLayout
[TRACE][DEVICE] wgpuDeviceCreateBindGroup
  binding=0 | buffer=0x3001 | offset=0 | size=80
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=0 | bindGroup=0x3002
```

### TEST_03_02: Texture Binding
**목적**: 텍스처 샘플링 검증

**텍스처 생성**:
```cpp
// 2x2 red texture
uint32_t textureData[] = {
    0xFF0000FF, 0xFF0000FF,
    0xFF0000FF, 0xFF0000FF
};
```

**Expected Trace**:
```
[TRACE][DEVICE] wgpuDeviceCreateTexture | width=2 | height=2 | format=RGBA8Unorm
[TRACE][QUEUE] wgpuQueueWriteTexture | texture=0x4001 | size=16
[TRACE][DEVICE] wgpuDeviceCreateSampler | filter=Linear | wrap=Repeat
[TRACE][DEVICE] wgpuDeviceCreateBindGroup
  binding=0 | texture=0x4001
  binding=1 | sampler=0x4002
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=1 | bindGroup=0x4003
```

### TEST_03_03: Dynamic Offsets
**목적**: Dynamic offset을 사용한 per-object 데이터

**시나리오**:
```cpp
// Single buffer with multiple object data
struct ObjectData {
    glm::mat4 modelMatrix;
};

// Buffer contains data for 3 objects
auto buffer = CreateBuffer(sizeof(ObjectData) * 3);

// Render 3 objects with different offsets
for (int i = 0; i < 3; i++) {
    uint32_t offset = i * sizeof(ObjectData);
    cmd->SetBindGroup(2, objectBindGroup, &offset, 1);
    cmd->DrawIndexed(indexCount);
}
```

**Expected Trace**:
```
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=2 | dynamicOffset=0
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=36
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=2 | dynamicOffset=64
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=36
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=2 | dynamicOffset=128
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=36
```

## Level 4: Multi-Pass Rendering Tests

### TEST_04_01: Shadow Mapping
**목적**: Shadow pass + Main pass 검증

**Pass 구조**:
```
Pass 1: Shadow Map Generation
  - Target: 1024x1024 depth texture
  - Pipeline: Shadow pipeline (depth only)
  - View: Light perspective

Pass 2: Main Rendering
  - Target: Backbuffer
  - Pipeline: Main pipeline with shadows
  - Bindings: Shadow map as texture
  - View: Camera perspective
```

**Expected Trace Sequence**:
```
// Shadow Pass
[TRACE][ENCODER] wgpuCommandEncoderBeginRenderPass | target=shadowMap
[TRACE][PASS] wgpuRenderPassEncoderSetPipeline | pipeline=shadowPipeline
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=0 | group=lightMatrices
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=36
[TRACE][PASS] wgpuRenderPassEncoderEnd

// Main Pass
[TRACE][ENCODER] wgpuCommandEncoderBeginRenderPass | target=backbuffer
[TRACE][PASS] wgpuRenderPassEncoderSetPipeline | pipeline=mainPipeline
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=0 | group=cameraMatrices
[TRACE][PASS] wgpuRenderPassEncoderSetBindGroup | index=1 | group=shadowMap
[TRACE][PASS] wgpuRenderPassEncoderDrawIndexed | indexCount=36
[TRACE][PASS] wgpuRenderPassEncoderEnd
```

### TEST_04_02: Post-Processing Chain
**목적**: 연속적인 포스트프로세싱 패스 검증

**Pass 체인**:
1. Scene → HDR Buffer
2. HDR → Bloom Extract
3. Bloom → Blur Horizontal
4. Blur H → Blur Vertical
5. Composite → Backbuffer

**검증 항목**:
- [ ] 각 패스의 입출력 연결
- [ ] 중간 버퍼 재사용
- [ ] 최종 백버퍼 출력

## Level 5: Error Condition Tests

### TEST_05_01: RenderPass Not Ended
**목적**: RenderPass를 닫지 않고 프레임 종료 시도

**테스트 코드**:
```cpp
TEST(ErrorTest, RenderPassNotEnded) {
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr);
    // Intentionally not calling EndRenderPass()
    
    EXPECT_THROW(
        renderer->EndFrame(cmd),
        std::runtime_error
    );
}
```

**Expected Error**:
```
std::runtime_error: "RenderPass not ended before EndFrame"
```

### TEST_05_02: Nested RenderPass
**목적**: 중첩된 RenderPass 시도

**테스트 코드**:
```cpp
TEST(ErrorTest, NestedRenderPass) {
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr);
    
    EXPECT_THROW(
        cmd->BeginRenderPass(nullptr),
        std::runtime_error
    );
}
```

### TEST_05_03: Draw Without Pipeline
**목적**: Pipeline 설정 없이 Draw 호출

**테스트 코드**:
```cpp
TEST(ErrorTest, DrawWithoutPipeline) {
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr);
    
    EXPECT_THROW(
        cmd->Draw(3),
        std::runtime_error
    );
}
```

### TEST_05_04: Invalid Resource State
**목적**: 잘못된 리소스 상태에서 사용

**시나리오**:
```cpp
// Buffer being used as both input and output
auto buffer = CreateBuffer(BufferDesc{...});

cmd->BeginComputePass();
cmd->SetBindGroup(0, bufferAsInput);
cmd->SetBindGroup(1, bufferAsOutput);  // Same buffer!
EXPECT_THROW(cmd->Dispatch(1, 1, 1), std::runtime_error);
```

## Performance Tests

### PERF_01: Draw Call Throughput
**목적**: 초당 처리 가능한 draw call 수 측정

**테스트 시나리오**:
```cpp
TEST(PerfTest, DrawCallThroughput) {
    const int NUM_OBJECTS = 1000;
    const int NUM_FRAMES = 100;
    
    auto startTime = GetTime();
    
    for (int frame = 0; frame < NUM_FRAMES; frame++) {
        auto cmd = renderer->BeginFrame();
        cmd->BeginRenderPass(nullptr);
        
        for (int i = 0; i < NUM_OBJECTS; i++) {
            cmd->SetBindGroup(2, objects[i]);
            cmd->DrawIndexed(36);
        }
        
        cmd->EndRenderPass();
        renderer->EndFrame(cmd);
    }
    
    auto elapsed = GetTime() - startTime;
    auto fps = NUM_FRAMES / elapsed;
    auto drawCallsPerSecond = NUM_OBJECTS * fps;
    
    EXPECT_GT(fps, 60.0);
    EXPECT_GT(drawCallsPerSecond, 60000);  // 60k draw calls/sec
}
```

### PERF_02: State Change Overhead
**목적**: 상태 변경 오버헤드 측정

**측정 항목**:
- Pipeline 변경: < 0.1ms
- BindGroup 변경: < 0.01ms
- Viewport 변경: < 0.001ms

### PERF_03: Memory Usage
**목적**: 메모리 사용량 추적

**측정 시나리오**:
```cpp
TEST(MemoryTest, FrameMemoryUsage) {
    size_t initialMemory = GetMemoryUsage();
    
    // Render 1000 frames
    for (int i = 0; i < 1000; i++) {
        RenderFrame();
    }
    
    size_t finalMemory = GetMemoryUsage();
    size_t leaked = finalMemory - initialMemory;
    
    EXPECT_LT(leaked, 1024 * 1024);  // Less than 1MB leak
}
```

## Stress Tests

### STRESS_01: Maximum Resources
**목적**: 최대 리소스 생성 한계 테스트

**테스트 항목**:
- 최대 텍스처 수: 10,000+
- 최대 버퍼 수: 10,000+
- 최대 Pipeline 수: 1,000+

### STRESS_02: Rapid Context Loss
**목적**: GPU 컨텍스트 손실 복구

**시나리오**:
```cpp
TEST(StressTest, ContextLossRecovery) {
    for (int i = 0; i < 10; i++) {
        SimulateContextLoss();
        
        // Renderer should recover
        EXPECT_NO_THROW(RenderFrame());
        
        // Verify output is correct
        auto pixels = CaptureFramebuffer();
        EXPECT_TRUE(VerifyPixels(pixels));
    }
}
```

## Validation Matrix

### Coverage Matrix
| Test Level | Unit Tests | Integration Tests | E2E Tests | Trace Validation |
|------------|------------|-------------------|-----------|------------------|
| Level 0    | 10         | 2                 | 1         | ✓                |
| Level 1    | 15         | 3                 | 1         | ✓                |
| Level 2    | 20         | 5                 | 2         | ✓                |
| Level 3    | 25         | 7                 | 2         | ✓                |
| Level 4    | 15         | 10                | 3         | ✓                |
| Level 5    | 20         | 5                 | 1         | ✓                |
| **Total**  | **105**    | **32**            | **10**    | **100%**         |

### Success Criteria
- **Unit Tests**: 100% pass, < 1s total runtime
- **Integration Tests**: 100% pass, < 10s total runtime
- **E2E Tests**: 100% pass, < 30s total runtime
- **Trace Validation**: 95%+ match rate
- **Performance Tests**: Meet all benchmarks
- **Stress Tests**: No crashes, proper recovery

## Test Automation

### CI/CD Pipeline
```yaml
name: Validation Pipeline

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - name: Run Unit Tests
        run: ctest -L unit --output-on-failure
        
  integration-tests:
    runs-on: ubuntu-latest
    steps:
      - name: Run Integration Tests
        run: ctest -L integration --output-on-failure
        
  trace-validation:
    runs-on: ubuntu-latest
    steps:
      - name: Generate Vanilla Traces
        run: ./run_vanilla_samples.sh
        
      - name: Generate Pers Traces
        run: ./run_pers_samples.sh
        
      - name: Compare Traces
        run: python compare_all_traces.py
        
      - name: Upload Report
        uses: actions/upload-artifact@v2
        with:
          name: validation-report
          path: reports/
          
  performance-tests:
    runs-on: [self-hosted, gpu]
    steps:
      - name: Run Performance Tests
        run: ./run_perf_tests.sh
        
      - name: Check Benchmarks
        run: python check_benchmarks.py
```

### Test Execution Order
```
1. Fast Unit Tests (< 1s)
2. Integration Tests (< 10s)
3. Trace Validation (< 30s)
4. E2E Tests (< 1m)
5. Performance Tests (< 5m)
6. Stress Tests (< 10m)
```

## Test Data Management

### Reference Traces
```
validation/reference_traces/
├── vanilla/
│   ├── 01_clear.trace
│   ├── 02_triangle.trace
│   ├── 03_textured.trace
│   ├── 04_shadow.trace
│   └── 05_postprocess.trace
└── golden/
    └── verified_traces.json
```

### Test Assets
```
test_assets/
├── textures/
│   ├── checker_2x2.png
│   ├── white_1x1.png
│   └── gradient_256x256.png
├── models/
│   ├── triangle.obj
│   ├── cube.obj
│   └── sphere.obj
└── shaders/
    ├── basic.vert
    ├── basic.frag
    └── shadow.frag
```

## Debugging Support

### Trace Analysis Tools
```bash
# Analyze trace differences
python analyze_trace.py vanilla.trace pers.trace --verbose

# Filter specific API calls
python filter_trace.py input.trace --include "Draw*" --exclude "Set*"

# Generate timing report
python timing_report.py input.trace --group-by category
```

### Visual Debugging
```cpp
// Enable visual debugging
#ifdef DEBUG_RENDERING
    cmd->DebugMarkerBegin("Shadow Pass");
    RenderShadowPass();
    cmd->DebugMarkerEnd();
    
    cmd->DebugMarkerBegin("Main Pass");
    RenderMainPass();
    cmd->DebugMarkerEnd();
#endif
```

## Conclusion

이 테스트 명세는 Pers Graphics Engine의 **완벽한 검증**을 보장합니다.

**핵심 특징**:
1. **계층적 검증**: 단순에서 복잡으로
2. **자동화된 비교**: Trace 기반 검증
3. **성능 보장**: 명확한 벤치마크
4. **에러 처리**: 모든 예외 상황 커버

모든 테스트를 통과하면 Production-ready 품질을 보장합니다.