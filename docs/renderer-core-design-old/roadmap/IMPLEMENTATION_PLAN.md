# Pers Graphics Engine - Detailed Implementation Plan

## 전체 구현 전략

### 구현 원칙
1. **Top-Down Development**: API 설계 → Mock 구현 → 실제 구현
2. **Test-First**: 테스트 작성 → 구현 → 검증
3. **Incremental Complexity**: 단순 → 복잡 순서로 진행
4. **Continuous Validation**: 매 단계마다 Trace 검증

## Phase 0: Foundation Setup (Week 0)

### 목표
프로젝트 구조 확립 및 빌드 시스템 구성

### 작업 항목

#### 디렉토리 구조 생성
```
pers/
├── engine/
│   ├── include/pers/
│   │   ├── core/
│   │   │   └── Types.h
│   │   ├── renderer/
│   │   │   ├── IRenderer.h
│   │   │   ├── ICommandRecorder.h
│   │   │   └── Resources.h
│   │   └── utils/
│   │       ├── Logger.h
│   │       └── TodoOrDie.h
│   └── src/
│       ├── core/
│       │   └── Types.cpp
│       └── utils/
│           ├── Logger.cpp
│           └── TodoOrDie.cpp
├── validation/
│   ├── tracer/
│   │   ├── include/
│   │   │   └── TraceLogger.h
│   │   └── src/
│   │       └── TraceLogger.cpp
│   ├── vanilla/
│   │   ├── 00_init/
│   │   ├── 01_clear/
│   │   ├── 02_triangle/
│   │   ├── 03_textured_quad/
│   │   └── 04_multi_pass/
│   └── comparison/
│       ├── trace_parser.py
│       ├── trace_compare.py
│       └── report_generator.py
├── tests/
│   ├── unit/
│   ├── integration/
│   └── validation/
└── CMakeLists.txt
```

#### CMake 구성
```cmake
# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(PersGraphicsEngine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(PERS_BUILD_VALIDATION "Build validation tools" ON)
option(PERS_BUILD_TESTS "Build tests" ON)
option(PERS_BUILD_SAMPLES "Build samples" ON)

# Engine library
add_subdirectory(engine)

# Validation tools
if(PERS_BUILD_VALIDATION)
    add_subdirectory(validation)
endif()

# Tests
if(PERS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### 검증 항목
- [ ] CMake 빌드 성공
- [ ] 기본 Logger 동작 확인
- [ ] TodoOrDie 로그 출력 확인

## Phase 1: Core Interfaces & Mock Implementation (Week 1-2)

### 목표
핵심 인터페이스 정의 및 Mock 구현으로 API 검증

### Day 1-2: Core Interfaces

#### IRenderer.h
```cpp
#pragma once
#include <memory>

namespace pers {

class ICommandRecorder;
class RenderTarget;

class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // Frame lifecycle
    virtual std::shared_ptr<ICommandRecorder> BeginFrame() = 0;
    virtual void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) = 0;
    
    // Resource queries
    virtual std::shared_ptr<RenderTarget> GetCurrentBackbuffer() const = 0;
    
    // State queries
    virtual bool IsFrameInProgress() const = 0;
};

} // namespace pers
```

#### ICommandRecorder.h
```cpp
#pragma once
#include <memory>
#include "pers/core/Types.h"

namespace pers {

class RenderTarget;
class Pipeline;
class BindGroup;
class Buffer;

class ICommandRecorder {
public:
    virtual ~ICommandRecorder() = default;
    
    // RenderPass management
    virtual void BeginRenderPass(
        const std::shared_ptr<RenderTarget>& target,
        const ClearValue& clear = {}) = 0;
    virtual void EndRenderPass() = 0;
    virtual bool IsInRenderPass() const = 0;
    
    // Pipeline & State
    virtual void SetPipeline(const std::shared_ptr<Pipeline>& pipeline) = 0;
    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissor(const Rect& scissor) = 0;
    
    // Resource binding
    virtual void SetBindGroup(
        uint32_t index, 
        const std::shared_ptr<BindGroup>& group,
        const uint32_t* dynamicOffsets = nullptr,
        uint32_t dynamicOffsetCount = 0) = 0;
    
    // Draw commands
    virtual void Draw(
        uint32_t vertexCount,
        uint32_t instanceCount = 1,
        uint32_t firstVertex = 0,
        uint32_t firstInstance = 0) = 0;
        
    virtual void DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount = 1,
        uint32_t firstIndex = 0,
        int32_t baseVertex = 0,
        uint32_t firstInstance = 0) = 0;
};

} // namespace pers
```

### Day 3-4: Mock Implementation

#### MockRenderer.cpp
```cpp
#include "MockRenderer.h"
#include "MockCommandRecorder.h"
#include "validation/TraceLogger.h"

namespace pers::mock {

class MockRenderer : public IRenderer {
private:
    bool _frameInProgress = false;
    std::shared_ptr<MockRenderTarget> _backbuffer;
    
public:
    MockRenderer() {
        TRACE_CALL("DEVICE", "wgpuCreateDevice");
        _backbuffer = std::make_shared<MockRenderTarget>("backbuffer");
    }
    
    std::shared_ptr<ICommandRecorder> BeginFrame() override {
        if (_frameInProgress) {
            throw std::runtime_error("Frame already in progress");
        }
        
        TRACE_CALL("DEVICE", "wgpuDeviceCreateCommandEncoder");
        _frameInProgress = true;
        return std::make_shared<MockCommandRecorder>();
    }
    
    void EndFrame(const std::shared_ptr<ICommandRecorder>& cmd) override {
        if (!_frameInProgress) {
            throw std::runtime_error("No frame in progress");
        }
        
        auto mockCmd = std::dynamic_pointer_cast<MockCommandRecorder>(cmd);
        if (mockCmd->IsInRenderPass()) {
            throw std::runtime_error("RenderPass not ended before EndFrame");
        }
        
        TRACE_CALL("ENCODER", "wgpuCommandEncoderFinish");
        TRACE_CALL("QUEUE", "wgpuQueueSubmit");
        
        _frameInProgress = false;
    }
};

} // namespace pers::mock
```

#### MockCommandRecorder.cpp
```cpp
#include "MockCommandRecorder.h"
#include "validation/TraceLogger.h"

namespace pers::mock {

class MockCommandRecorder : public ICommandRecorder {
private:
    enum class State { Idle, Recording, InRenderPass };
    State _state = State::Recording;
    std::shared_ptr<Pipeline> _currentPipeline;
    
public:
    void BeginRenderPass(
        const std::shared_ptr<RenderTarget>& target,
        const ClearValue& clear) override {
        
        if (_state == State::InRenderPass) {
            throw std::runtime_error("Already in RenderPass");
        }
        
        TRACE_CALL_WITH_PARAMS("ENCODER", "wgpuCommandEncoderBeginRenderPass",
            "target", target ? "custom" : "backbuffer",
            "clearR", clear.color[0],
            "clearG", clear.color[1],
            "clearB", clear.color[2],
            "clearA", clear.color[3]);
            
        _state = State::InRenderPass;
    }
    
    void EndRenderPass() override {
        if (_state != State::InRenderPass) {
            throw std::runtime_error("Not in RenderPass");
        }
        
        TRACE_CALL("PASS", "wgpuRenderPassEncoderEnd");
        _state = State::Recording;
    }
    
    void Draw(uint32_t vertexCount, uint32_t instanceCount,
              uint32_t firstVertex, uint32_t firstInstance) override {
        
        if (_state != State::InRenderPass) {
            throw std::runtime_error("Draw called outside RenderPass");
        }
        
        if (!_currentPipeline) {
            throw std::runtime_error("No pipeline set");
        }
        
        TRACE_CALL_WITH_PARAMS("PASS", "wgpuRenderPassEncoderDraw",
            "vertexCount", vertexCount,
            "instanceCount", instanceCount,
            "firstVertex", firstVertex,
            "firstInstance", firstInstance);
    }
};

} // namespace pers::mock
```

### Day 5-6: First Test Sample

#### test_01_clear.cpp
```cpp
#include "pers/renderer/IRenderer.h"
#include "mock/MockRenderer.h"
#include "validation/TraceLogger.h"

int main() {
    // Initialize tracing
    TRACE_INIT("pers_01_clear.trace");
    
    // Create mock renderer
    auto renderer = pers::mock::CreateMockRenderer();
    
    // Render one frame with clear
    {
        auto cmd = renderer->BeginFrame();
        
        // Clear to blue
        pers::ClearValue clearValue;
        clearValue.color = {0.2f, 0.3f, 0.4f, 1.0f};
        
        cmd->BeginRenderPass(nullptr, clearValue);
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
    
    std::cout << "Trace saved to: pers_01_clear.trace\n";
    return 0;
}
```

### 검증 항목
- [ ] Mock 구현이 컴파일되고 실행됨
- [ ] 올바른 trace 생성
- [ ] 에러 조건 테스트 통과

## Phase 2: Resource Management (Week 3-4)

### 목표
리소스 생성 및 관리 시스템 구현

### Day 1-2: Resource Interfaces

#### Resources.h
```cpp
namespace pers {

// Pipeline State Object
class Pipeline {
public:
    virtual ~Pipeline() = default;
    virtual uint32_t GetId() const = 0;
};

// Bind Group (Descriptor Set)
class BindGroup {
public:
    virtual ~BindGroup() = default;
    virtual uint32_t GetId() const = 0;
};

// Buffer Resource
class Buffer {
public:
    enum class Usage {
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        Indirect = 1 << 4,
        CopySrc = 1 << 5,
        CopyDst = 1 << 6
    };
    
    virtual ~Buffer() = default;
    virtual size_t GetSize() const = 0;
    virtual Usage GetUsage() const = 0;
};

// Texture Resource
class Texture {
public:
    virtual ~Texture() = default;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual Format GetFormat() const = 0;
};

// Render Target
class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    virtual std::shared_ptr<Texture> GetColorAttachment(uint32_t index = 0) const = 0;
    virtual std::shared_ptr<Texture> GetDepthAttachment() const = 0;
};

} // namespace pers
```

### Day 3-4: Resource Creation API

#### IRenderer Extended
```cpp
class IRenderer {
public:
    // ... existing methods ...
    
    // Resource creation
    virtual std::shared_ptr<Pipeline> CreatePipeline(
        const PipelineDesc& desc) = 0;
        
    virtual std::shared_ptr<Buffer> CreateBuffer(
        const BufferDesc& desc) = 0;
        
    virtual std::shared_ptr<Texture> CreateTexture(
        const TextureDesc& desc) = 0;
        
    virtual std::shared_ptr<BindGroup> CreateBindGroup(
        const BindGroupDesc& desc) = 0;
        
    virtual std::shared_ptr<RenderTarget> CreateRenderTarget(
        const RenderTargetDesc& desc) = 0;
};
```

### Day 5-6: Triangle Sample

#### test_02_triangle.cpp
```cpp
int main() {
    TRACE_INIT("pers_02_triangle.trace");
    
    auto renderer = pers::mock::CreateMockRenderer();
    
    // Create pipeline for triangle
    pers::PipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = LoadShader("triangle.vert");
    pipelineDesc.fragmentShader = LoadShader("triangle.frag");
    pipelineDesc.primitiveTopology = PrimitiveTopology::TriangleList;
    
    auto pipeline = renderer->CreatePipeline(pipelineDesc);
    
    // Render frame
    {
        auto cmd = renderer->BeginFrame();
        
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        cmd->SetPipeline(pipeline);
        cmd->Draw(3);  // 3 vertices for triangle
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
    
    return 0;
}
```

### 검증 항목
- [ ] Pipeline 생성 trace 일치
- [ ] Draw 명령 파라미터 일치
- [ ] 리소스 생명주기 관리

## Phase 3: Trace Validation System (Week 5-6)

### 목표
Vanilla WebGPU와 Pers Mock 구현의 trace 비교 시스템 구축

### Day 1-2: Vanilla WebGPU Samples

#### vanilla_01_clear.cpp
```cpp
#include <webgpu/webgpu.h>
#include "validation/TraceLogger.h"

int main() {
    TRACE_INIT("vanilla_01_clear.trace");
    
    // Initialize WebGPU
    WGPUInstance instance = wgpuCreateInstance(nullptr);
    TRACE_PARAM("instance", instance);
    
    // ... adapter, device, queue setup ...
    
    // Render one frame
    {
        // Create command encoder
        TRACE_CALL("DEVICE", "wgpuDeviceCreateCommandEncoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        
        // Begin render pass
        WGPURenderPassDescriptor passDesc = {};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;
        
        TRACE_CALL_WITH_PARAMS("ENCODER", "wgpuCommandEncoderBeginRenderPass",
            "target", "backbuffer",
            "clearR", 0.2f,
            "clearG", 0.3f,
            "clearB", 0.4f,
            "clearA", 1.0f);
            
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);
        
        // End render pass
        TRACE_CALL("PASS", "wgpuRenderPassEncoderEnd");
        wgpuRenderPassEncoderEnd(pass);
        
        // Finish and submit
        TRACE_CALL("ENCODER", "wgpuCommandEncoderFinish");
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
        
        TRACE_CALL("QUEUE", "wgpuQueueSubmit");
        wgpuQueueSubmit(queue, 1, &commands);
    }
    
    return 0;
}
```

### Day 3-4: Trace Comparison Tool

#### trace_compare.py
```python
#!/usr/bin/env python3
import sys
import re
import json
from dataclasses import dataclass
from typing import List, Dict, Optional
from enum import Enum

class ComparisonLevel(Enum):
    STRUCTURE = 1  # Function calls only
    PARAMETERS = 2  # Include parameters
    STATE = 3       # Include state changes

@dataclass
class TraceEntry:
    timestamp: float
    category: str
    function: str
    parameters: Dict[str, str]
    
    @classmethod
    def parse(cls, line: str) -> Optional['TraceEntry']:
        # [TRACE][0.001][CATEGORY] function | param1=value1 | param2=value2
        pattern = r'\[TRACE\]\[(\d+\.\d+)\]\[(\w+)\] (\w+)(.*)'
        match = re.match(pattern, line)
        
        if not match:
            return None
            
        timestamp = float(match.group(1))
        category = match.group(2)
        function = match.group(3)
        param_str = match.group(4)
        
        # Parse parameters
        parameters = {}
        if param_str:
            for param in param_str.split(' | '):
                if '=' in param:
                    key, value = param.strip().split('=', 1)
                    parameters[key] = value
        
        return cls(timestamp, category, function, parameters)

class TraceComparator:
    def __init__(self, vanilla_file: str, pers_file: str):
        self.vanilla_traces = self._load_traces(vanilla_file)
        self.pers_traces = self._load_traces(pers_file)
        
    def _load_traces(self, filename: str) -> List[TraceEntry]:
        traces = []
        with open(filename, 'r') as f:
            for line in f:
                entry = TraceEntry.parse(line.strip())
                if entry:
                    traces.append(entry)
        return traces
    
    def _normalize_traces(self, traces: List[TraceEntry]) -> List[TraceEntry]:
        """Remove timestamps and normalize handle values"""
        normalized = []
        handle_map = {}
        next_id = 1
        
        for trace in traces:
            # Normalize handles (0x addresses to symbolic names)
            norm_params = {}
            for key, value in trace.parameters.items():
                if value.startswith('0x'):
                    if value not in handle_map:
                        handle_map[value] = f"handle_{next_id}"
                        next_id += 1
                    norm_params[key] = handle_map[value]
                else:
                    norm_params[key] = value
            
            normalized.append(TraceEntry(
                0.0,  # Remove timestamp
                trace.category,
                trace.function,
                norm_params
            ))
        
        return normalized
    
    def compare(self, level: ComparisonLevel = ComparisonLevel.PARAMETERS) -> Dict:
        vanilla_norm = self._normalize_traces(self.vanilla_traces)
        pers_norm = self._normalize_traces(self.pers_traces)
        
        result = {
            'level': level.name,
            'vanilla_count': len(vanilla_norm),
            'pers_count': len(pers_norm),
            'matches': [],
            'differences': []
        }
        
        # Compare structure
        if level >= ComparisonLevel.STRUCTURE:
            for i, (v, p) in enumerate(zip(vanilla_norm, pers_norm)):
                if v.function != p.function:
                    result['differences'].append({
                        'index': i,
                        'vanilla': v.function,
                        'pers': p.function,
                        'type': 'function_mismatch'
                    })
                else:
                    result['matches'].append(i)
        
        # Compare parameters
        if level >= ComparisonLevel.PARAMETERS:
            for i, (v, p) in enumerate(zip(vanilla_norm, pers_norm)):
                if v.function == p.function and v.parameters != p.parameters:
                    result['differences'].append({
                        'index': i,
                        'function': v.function,
                        'vanilla_params': v.parameters,
                        'pers_params': p.parameters,
                        'type': 'parameter_mismatch'
                    })
        
        # Calculate match rate
        total = max(len(vanilla_norm), len(pers_norm))
        matched = len(result['matches'])
        result['match_rate'] = (matched / total * 100) if total > 0 else 0
        
        return result

def main():
    if len(sys.argv) != 3:
        print("Usage: trace_compare.py <vanilla_trace> <pers_trace>")
        sys.exit(1)
    
    comparator = TraceComparator(sys.argv[1], sys.argv[2])
    result = comparator.compare(ComparisonLevel.PARAMETERS)
    
    print(f"Comparison Results")
    print(f"==================")
    print(f"Vanilla traces: {result['vanilla_count']}")
    print(f"Pers traces: {result['pers_count']}")
    print(f"Match rate: {result['match_rate']:.2f}%")
    
    if result['differences']:
        print(f"\nDifferences found: {len(result['differences'])}")
        for diff in result['differences'][:5]:  # Show first 5
            print(f"  - {diff}")
    else:
        print("\n✅ All traces match!")
    
    # Save detailed report
    with open('comparison_report.json', 'w') as f:
        json.dump(result, f, indent=2)
    
    return 0 if result['match_rate'] >= 95 else 1

if __name__ == "__main__":
    sys.exit(main())
```

### Day 5-6: CI/CD Integration

#### CMakeLists.txt for Validation
```cmake
# validation/CMakeLists.txt
add_library(trace_logger STATIC
    tracer/src/TraceLogger.cpp
)

# Vanilla samples
add_executable(vanilla_01_clear
    vanilla/01_clear/main.cpp
)
target_link_libraries(vanilla_01_clear
    trace_logger
    webgpu
)

# Pers samples  
add_executable(pers_01_clear
    pers/01_clear/main.cpp
)
target_link_libraries(pers_01_clear
    pers_engine
    trace_logger
)

# Add validation test
add_test(NAME validation_01_clear
    COMMAND ${CMAKE_COMMAND}
    -DVANILLA_EXEC=$<TARGET_FILE:vanilla_01_clear>
    -DPERS_EXEC=$<TARGET_FILE:pers_01_clear>
    -DCOMPARE_SCRIPT=${CMAKE_SOURCE_DIR}/validation/comparison/trace_compare.py
    -P ${CMAKE_SOURCE_DIR}/cmake/RunValidation.cmake
)
```

### 검증 항목
- [ ] Vanilla와 Pers trace 생성
- [ ] 95% 이상 일치율
- [ ] CI/CD 자동 검증

## Phase 4: WebGPU Backend Implementation (Week 7-8)

### 목표
실제 WebGPU 백엔드 구현

### Day 1-3: WebGPU Device Setup

#### WebGPURenderer.cpp
```cpp
#include "WebGPURenderer.h"
#include <webgpu/webgpu.h>

namespace pers::webgpu {

class WebGPURenderer : public IRenderer {
private:
    WGPUInstance _instance;
    WGPUAdapter _adapter;
    WGPUDevice _device;
    WGPUQueue _queue;
    WGPUSwapChain _swapChain;
    
    bool _frameInProgress = false;
    
public:
    bool Initialize(const RendererDesc& desc) {
        // Create instance
        WGPUInstanceDescriptor instanceDesc = {};
        _instance = wgpuCreateInstance(&instanceDesc);
        
        // Request adapter
        WGPURequestAdapterOptions adapterOpts = {};
        adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
        
        // ... adapter and device setup ...
        
        return true;
    }
    
    std::shared_ptr<ICommandRecorder> BeginFrame() override {
        if (_frameInProgress) {
            throw std::runtime_error("Frame already in progress");
        }
        
        // Get next swap chain texture
        WGPUTextureView backbuffer = wgpuSwapChainGetCurrentTextureView(_swapChain);
        
        // Create command encoder
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, nullptr);
        
        _frameInProgress = true;
        return std::make_shared<WebGPUCommandRecorder>(encoder, backbuffer);
    }
};

} // namespace pers::webgpu
```

### Day 4-6: Command Recording

#### WebGPUCommandRecorder.cpp
```cpp
namespace pers::webgpu {

class WebGPUCommandRecorder : public ICommandRecorder {
private:
    WGPUCommandEncoder _encoder;
    WGPURenderPassEncoder _currentPass = nullptr;
    WGPUTextureView _backbuffer;
    
public:
    WebGPUCommandRecorder(WGPUCommandEncoder encoder, WGPUTextureView backbuffer)
        : _encoder(encoder), _backbuffer(backbuffer) {}
        
    void BeginRenderPass(
        const std::shared_ptr<RenderTarget>& target,
        const ClearValue& clear) override {
        
        if (_currentPass) {
            throw std::runtime_error("Already in RenderPass");
        }
        
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = target ? GetTargetView(target) : _backbuffer;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {
            clear.color[0], clear.color[1],
            clear.color[2], clear.color[3]
        };
        
        WGPURenderPassDescriptor passDesc = {};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;
        
        _currentPass = wgpuCommandEncoderBeginRenderPass(_encoder, &passDesc);
    }
    
    void Draw(uint32_t vertexCount, uint32_t instanceCount,
              uint32_t firstVertex, uint32_t firstInstance) override {
        
        if (!_currentPass) {
            throw std::runtime_error("Not in RenderPass");
        }
        
        wgpuRenderPassEncoderDraw(_currentPass,
            vertexCount, instanceCount,
            firstVertex, firstInstance);
    }
};

} // namespace pers::webgpu
```

### 검증 항목
- [ ] 실제 GPU에서 렌더링
- [ ] Mock과 동일한 API 동작
- [ ] Trace 일치 확인

## Phase 5: Integration & Optimization (Week 9-10)

### 목표
전체 시스템 통합 및 최적화

### Day 1-3: Performance Optimization

#### Command Pool Implementation
```cpp
class CommandPool {
private:
    std::vector<std::shared_ptr<CommandRecorder>> _available;
    std::vector<std::shared_ptr<CommandRecorder>> _inUse;
    
public:
    std::shared_ptr<CommandRecorder> Acquire() {
        if (_available.empty()) {
            return CreateNew();
        }
        
        auto cmd = _available.back();
        _available.pop_back();
        _inUse.push_back(cmd);
        
        cmd->Reset();
        return cmd;
    }
    
    void Release(std::shared_ptr<CommandRecorder> cmd) {
        auto it = std::find(_inUse.begin(), _inUse.end(), cmd);
        if (it != _inUse.end()) {
            _inUse.erase(it);
            _available.push_back(cmd);
        }
    }
};
```

### Day 4-6: Multi-Pass Rendering

#### Shadow Mapping Sample
```cpp
void RenderShadowMap(IRenderer* renderer, Scene* scene) {
    auto cmd = renderer->BeginFrame();
    
    // Shadow pass
    cmd->BeginRenderPass(shadowMap, ClearValue::Depth(1.0f));
    cmd->SetPipeline(shadowPipeline);
    cmd->SetBindGroup(0, lightMatrices);
    
    for (auto& object : scene->GetObjects()) {
        cmd->SetBindGroup(1, object->GetTransform());
        cmd->DrawIndexed(object->GetIndexCount());
    }
    cmd->EndRenderPass();
    
    // Main pass
    cmd->BeginRenderPass(nullptr, ClearValue::Black());
    cmd->SetPipeline(mainPipeline);
    cmd->SetBindGroup(0, cameraMatrices);
    cmd->SetBindGroup(1, shadowMap);
    
    for (auto& object : scene->GetObjects()) {
        cmd->SetBindGroup(2, object->GetMaterial());
        cmd->SetBindGroup(3, object->GetTransform());
        cmd->DrawIndexed(object->GetIndexCount());
    }
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

### Day 7-10: Final Validation

#### Full Test Suite
```bash
# Run all validation tests
ctest --test-dir build/validation -j8

# Generate report
python validation/comparison/generate_report.py \
    --input build/validation/results \
    --output validation_report.html
```

### 검증 항목
- [ ] 모든 샘플 95% 이상 trace 일치
- [ ] 60 FPS 성능 달성
- [ ] 메모리 누수 없음

## 최종 산출물

### 코드 산출물
1. **Engine Core**
   - IRenderer, ICommandRecorder 인터페이스
   - Mock 구현체
   - WebGPU 구현체

2. **Validation System**
   - TraceLogger 라이브러리
   - Vanilla WebGPU 샘플 (5개)
   - Pers Engine 샘플 (5개)
   - Python 비교 도구

3. **Tests**
   - Unit tests (50+)
   - Integration tests (20+)
   - Validation tests (5+)

### 문서 산출물
1. **API Documentation**
   - Doxygen 생성 HTML
   - 사용 예제

2. **Validation Reports**
   - Trace 비교 결과
   - 성능 벤치마크
   - 메모리 프로파일

3. **Architecture Documentation**
   - UML 다이어그램
   - 시퀀스 다이어그램
   - 설계 결정 문서

## 성공 지표

### 기능적 지표
- [ ] 5개 검증 샘플 모두 통과
- [ ] 95% 이상 trace 일치율
- [ ] 모든 에러 케이스 처리

### 성능 지표
- [ ] 1000 draw calls @ 60 FPS
- [ ] < 100MB 메모리 사용
- [ ] < 5% CPU 오버헤드

### 품질 지표
- [ ] 0 컴파일 경고
- [ ] 80% 테스트 커버리지
- [ ] 0 메모리 누수

## 리스크 및 대응 방안

### 기술적 리스크
1. **WebGPU 스펙 변경**
   - 대응: Dawn/wgpu-native 버전 고정
   
2. **플랫폼별 차이**
   - 대응: 플랫폼별 정규화 규칙

3. **성능 목표 미달**
   - 대응: 프로파일링 후 최적화

### 일정 리스크
1. **복잡도 증가**
   - 대응: 단계별 검증, 스코프 조정

2. **외부 의존성**
   - 대응: 대체 라이브러리 준비

## 결론

이 구현 계획은 Pers Graphics Engine을 **체계적이고 검증 가능한 방식**으로 구축합니다.

핵심 성공 요소:
1. **Trace-based validation**으로 정확성 보장
2. **Top-down development**로 API 우선 설계
3. **Incremental complexity**로 리스크 관리
4. **Continuous validation**으로 품질 유지

10주 후 Production-ready 엔진 완성을 목표로 합니다.