# Trace-Based Validation System

## 핵심 개념

### 문제 정의
WebGPU의 순수 구현(Vanilla WebGPU)과 우리의 Pers Engine이 **동일한 GPU 명령을 생성하는지** 검증해야 합니다.

### 해결 방법
두 구현체에서 동일한 위치에 동일한 형식의 로그를 출력하고, 이를 비교합니다.

```
[Vanilla WebGPU Triangle] → Trace Log A
[Pers Engine Triangle]    → Trace Log B
                            ↓
                    Compare A == B
```

## Trace Format (정규화된 로그 형식)

### Level 1: Function Call Trace
```
[TRACE][timestamp][category] function_name
```

예시:
```
[TRACE][0.001][DEVICE] wgpuDeviceCreateCommandEncoder
[TRACE][0.002][ENCODER] wgpuCommandEncoderBeginRenderPass
[TRACE][0.003][PASS] wgpuRenderPassEncoderSetPipeline
[TRACE][0.004][PASS] wgpuRenderPassEncoderDraw
[TRACE][0.005][PASS] wgpuRenderPassEncoderEnd
[TRACE][0.006][ENCODER] wgpuCommandEncoderFinish
[TRACE][0.007][QUEUE] wgpuQueueSubmit
```

### Level 2: Function Call + Parameters
```
[TRACE][timestamp][category] function_name | param1=value | param2=value
```

예시:
```
[TRACE][0.003][PASS] wgpuRenderPassEncoderSetPipeline | pipeline=0x1234
[TRACE][0.004][PASS] wgpuRenderPassEncoderDraw | vertexCount=3 | instanceCount=1
[TRACE][0.005][BUFFER] wgpuBufferMap | offset=0 | size=256
```

### Level 3: State Validation
```
[STATE][timestamp] state_type | value
```

예시:
```
[STATE][0.010] FRAMEBUFFER | width=800 | height=600 | format=BGRA8Unorm
[STATE][0.011] VIEWPORT | x=0 | y=0 | width=800 | height=600
```

## Implementation Strategy

### Phase 1: Vanilla WebGPU Instrumentation

#### 1.1 WebGPU Triangle with Tracing
```cpp
// vanilla-webgpu-triangle.cpp
#include "trace_logger.h"

void CreatePipeline() {
    TRACE_CALL("wgpuDeviceCreateRenderPipeline");
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);
    TRACE_PARAM("pipeline", pipeline);
}

void RenderFrame() {
    TRACE_CALL("wgpuDeviceCreateCommandEncoder");
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    
    TRACE_CALL("wgpuCommandEncoderBeginRenderPass");
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    
    TRACE_CALL("wgpuRenderPassEncoderSetPipeline");
    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    
    TRACE_CALL_WITH_PARAMS("wgpuRenderPassEncoderDraw", 
                          "vertexCount", 3, 
                          "instanceCount", 1);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
    
    TRACE_CALL("wgpuRenderPassEncoderEnd");
    wgpuRenderPassEncoderEnd(pass);
    
    TRACE_CALL("wgpuCommandEncoderFinish");
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
    
    TRACE_CALL("wgpuQueueSubmit");
    wgpuQueueSubmit(queue, 1, &commands);
}
```

### Phase 2: Pers Engine Mock Implementation

#### 2.1 Mock Renderer with Tracing
```cpp
// MockRenderer.cpp
class MockRenderer : public IRenderer {
    std::shared_ptr<ICommandRecorder> BeginFrame() override {
        TRACE_CALL("wgpuDeviceCreateCommandEncoder");
        return std::make_shared<MockCommandRecorder>();
    }
    
    void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) override {
        TRACE_CALL("wgpuCommandEncoderFinish");
        TRACE_CALL("wgpuQueueSubmit");
    }
};

class MockCommandRecorder : public ICommandRecorder {
    void BeginRenderPass(const std::shared_ptr<RenderTarget>& target,
                         const ClearValue& clear) override {
        TRACE_CALL("wgpuCommandEncoderBeginRenderPass");
    }
    
    void EndRenderPass() override {
        TRACE_CALL("wgpuRenderPassEncoderEnd");
    }
    
    void Draw(uint32_t vertexCount, uint32_t instanceCount) override {
        TRACE_CALL_WITH_PARAMS("wgpuRenderPassEncoderDraw",
                              "vertexCount", vertexCount,
                              "instanceCount", instanceCount);
    }
};
```

#### 2.2 Pers Triangle Sample
```cpp
// pers-triangle.cpp
int main() {
    auto renderer = CreateMockRenderer();
    auto pipeline = renderer->CreatePipeline(triangleDesc);
    
    // Render one frame
    auto cmd = renderer->BeginFrame();
    cmd->BeginRenderPass(nullptr, ClearValue{0.2f, 0.3f, 0.4f, 1.0f});
    cmd->SetPipeline(pipeline);
    cmd->Draw(3, 1);
    cmd->EndRenderPass();
    renderer->EndFrame(cmd);
    
    return 0;
}
```

## Trace Logger Implementation

### TraceLogger.h
```cpp
#pragma once
#include <fstream>
#include <chrono>
#include <sstream>

class TraceLogger {
public:
    static TraceLogger& Instance() {
        static TraceLogger instance;
        return instance;
    }
    
    void TraceCall(const std::string& function) {
        auto timestamp = GetTimestamp();
        _output << "[TRACE][" << timestamp << "][CALL] " << function << std::endl;
    }
    
    template<typename... Args>
    void TraceCallWithParams(const std::string& function, Args... args) {
        auto timestamp = GetTimestamp();
        _output << "[TRACE][" << timestamp << "][CALL] " << function;
        AppendParams(args...);
        _output << std::endl;
    }
    
    void SaveToFile(const std::string& filename) {
        std::ofstream file(filename);
        file << _output.str();
    }
    
    std::string GetTrace() const {
        return _output.str();
    }
    
private:
    std::stringstream _output;
    std::chrono::steady_clock::time_point _startTime;
    
    TraceLogger() : _startTime(std::chrono::steady_clock::now()) {}
    
    double GetTimestamp() {
        auto now = std::chrono::steady_clock::now();
        auto duration = now - _startTime;
        return std::chrono::duration<double>(duration).count();
    }
    
    template<typename T, typename U, typename... Rest>
    void AppendParams(const T& name, const U& value, Rest... rest) {
        _output << " | " << name << "=" << value;
        if constexpr (sizeof...(rest) > 0) {
            AppendParams(rest...);
        }
    }
};

#define TRACE_CALL(func) TraceLogger::Instance().TraceCall(func)
#define TRACE_CALL_WITH_PARAMS(func, ...) TraceLogger::Instance().TraceCallWithParams(func, __VA_ARGS__)
```

## Validation Tool

### trace_compare.py
```python
#!/usr/bin/env python3
import sys
import difflib

def normalize_trace(trace_file):
    """Remove timestamps and normalize format"""
    lines = []
    with open(trace_file, 'r') as f:
        for line in f:
            # Remove timestamp: [TRACE][0.001][CALL] -> [TRACE][CALL]
            parts = line.split('][')
            if len(parts) >= 3:
                normalized = ']['.join([parts[0]] + parts[2:])
                lines.append(normalized.strip())
    return lines

def compare_traces(vanilla_file, pers_file):
    """Compare two trace files"""
    vanilla_lines = normalize_trace(vanilla_file)
    pers_lines = normalize_trace(pers_file)
    
    # Level 1: Check if same functions are called
    vanilla_calls = [l for l in vanilla_lines if '[CALL]' in l]
    pers_calls = [l for l in pers_lines if '[CALL]' in l]
    
    if vanilla_calls == pers_calls:
        print("✅ Level 1 PASS: Function calls match")
    else:
        print("❌ Level 1 FAIL: Function calls don't match")
        diff = difflib.unified_diff(vanilla_calls, pers_calls)
        for line in diff:
            print(line)
        return False
    
    # Level 2: Check parameters
    if vanilla_lines == pers_lines:
        print("✅ Level 2 PASS: Parameters match")
        return True
    else:
        print("⚠️  Level 2 PARTIAL: Some parameters differ")
        diff = difflib.unified_diff(vanilla_lines, pers_lines)
        for line in diff:
            print(line)
        return False

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: trace_compare.py <vanilla_trace> <pers_trace>")
        sys.exit(1)
    
    result = compare_traces(sys.argv[1], sys.argv[2])
    sys.exit(0 if result else 1)
```

## Validation Workflow

### Step 1: Generate Vanilla Trace
```bash
# Run vanilla WebGPU triangle with tracing
./vanilla-webgpu-triangle
# Output: vanilla_trace.log
```

### Step 2: Generate Pers Trace
```bash
# Run Pers engine triangle with tracing
./pers-triangle
# Output: pers_trace.log
```

### Step 3: Compare Traces
```bash
# Compare the two traces
python trace_compare.py vanilla_trace.log pers_trace.log
```

Expected output:
```
✅ Level 1 PASS: Function calls match
✅ Level 2 PASS: Parameters match
```

## Test Cases

### Test 1: Basic Triangle
- Vanilla: Minimal triangle with solid color
- Pers: Same triangle through our API
- Expected: Identical WebGPU calls

### Test 2: Textured Quad
- Vanilla: Quad with texture
- Pers: Same quad through our API
- Expected: Identical resource bindings

### Test 3: Multiple Draw Calls
- Vanilla: 3 objects in one pass
- Pers: Same 3 objects
- Expected: Same draw call sequence

### Test 4: Multi-Pass Rendering
- Vanilla: Shadow pass + main pass
- Pers: Same two-pass setup
- Expected: Identical pass transitions

## Success Criteria

### Phase 1 (Week 1-2)
- [ ] Vanilla WebGPU triangle with full tracing
- [ ] Trace output in normalized format
- [ ] 100+ trace points captured

### Phase 2 (Week 3-4)
- [ ] Mock Pers implementation with tracing
- [ ] Identical function call sequence
- [ ] trace_compare.py tool working

### Phase 3 (Week 5-6)
- [ ] Parameter validation working
- [ ] 95% trace match rate
- [ ] All test cases passing

## Key Benefits

1. **Correctness Guarantee**: 우리 API가 올바른 WebGPU 호출을 생성함을 증명
2. **Regression Detection**: 변경사항이 출력을 바꾸면 즉시 감지
3. **Debug Tool**: 어디서 차이가 발생하는지 정확히 파악
4. **Documentation**: Trace가 곧 실행 순서 문서

## Important Notes

⚠️ **Deterministic Execution**: 양쪽 구현이 동일한 순서로 실행되도록 보장
⚠️ **Handle Values**: 포인터/핸들 값은 달라도 됨, 순서와 타입만 확인
⚠️ **Timing Independent**: 타임스탬프는 무시, 순서만 중요