# TodoOrDie Utility - Purpose and Usage Guide

## 개요

`TodoOrDie`는 Pers Graphics Engine의 점진적 개발을 위한 핵심 유틸리티입니다. 이 시스템은 인터페이스와 구현을 분리하여 컴파일 가능한 상태를 유지하면서도 미구현 기능을 명확히 추적할 수 있게 합니다.

## 목적 (Purpose)

### 1. 컴파일 가능한 스텁(Stub) 제공
```cpp
// 인터페이스는 정의되었지만 실제 구현은 아직 없는 상태
class WebGPURenderer : public IRenderer {
    std::shared_ptr<Pipeline> CreatePipeline(const PipelineDesc& desc) override {
        TodoOrDie::Log("WebGPURenderer::CreatePipeline");
        return nullptr;  // 컴파일은 되지만 실행시 로그 출력
    }
};
```

### 2. 구현 우선순위 추적
개발 중 어떤 기능이 가장 많이 호출되는지 추적하여 구현 우선순위 결정:
```
[TODO_OR_DIE] WebGPURenderer::CreatePipeline called 45 times
[TODO_OR_DIE] WebGPURenderer::CreateBuffer called 23 times
[TODO_OR_DIE] WebGPURenderer::CreateTexture called 12 times
```

### 3. Top-Down 개발 지원
먼저 전체 아키텍처를 구성한 후 세부 구현을 채워나가는 방식:
```cpp
// Step 1: 인터페이스 정의
class IRenderer {
    virtual std::shared_ptr<Pipeline> CreatePipeline(...) = 0;
};

// Step 2: 스텁 구현 (NotImplemented 사용)
class MockRenderer : public IRenderer {
    std::shared_ptr<Pipeline> CreatePipeline(...) override {
        TodoOrDie::Log("MockRenderer::CreatePipeline");
        return std::make_shared<MockPipeline>();
    }
};

// Step 3: 실제 구현 (나중에)
class WebGPURenderer : public IRenderer {
    std::shared_ptr<Pipeline> CreatePipeline(...) override {
        // 실제 WebGPU pipeline 생성 코드
        return std::make_shared<WebGPUPipeline>(...);
    }
};
```

## 구현 내용 (Implementation)

### TodoOrDie.h
```cpp
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

namespace pers::utils {

class NotImplemented {
public:
    // 기본 로깅 - 함수명만
    static void Log(const std::string& functionName);
    
    // 상세 로깅 - 파일, 라인, 함수명
    static void LogDetailed(const char* file, int line, const std::string& functionName);
    
    // 통계 출력
    static void PrintStatistics();
    
    // 통계 초기화
    static void ResetStatistics();
    
    // 호출 횟수 조회
    static size_t GetCallCount(const std::string& functionName);
    
    // 치명적 미구현 (예외 발생)
    static void Fatal(const std::string& functionName);

private:
    static std::unordered_map<std::string, size_t> _callCounts;
    static std::mutex _mutex;
};

// 편의 매크로
#define TODO_OR_DIE() \
    pers::utils::TodoOrDie::LogDetailed(__FILE__, __LINE__, __FUNCTION__)

#define TODO_OR_DIE_FATAL() \
    pers::utils::TodoOrDie::Fatal(__FUNCTION__)

} // namespace pers::utils
```

### TodoOrDie.cpp
```cpp
#include "pers/utils/TodoOrDie.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace pers::utils {

std::unordered_map<std::string, size_t> TodoOrDie::_callCounts;
std::mutex TodoOrDie::_mutex;

void TodoOrDie::Log(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 호출 횟수 증가
    _callCounts[functionName]++;
    
    // 로그 출력
    Logger::Warn("[TODO_OR_DIE] {} - not yet implemented (call #{})",
                 functionName, _callCounts[functionName]);
    
    // 처음 호출시 스택 트레이스 출력 (디버그 모드)
    #ifdef DEBUG
    if (_callCounts[functionName] == 1) {
        Logger::Debug("  First call from:");
        // 스택 트레이스 출력 (플랫폼별 구현)
    }
    #endif
}

void TodoOrDie::LogDetailed(const char* file, int line, const std::string& functionName) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    _callCounts[functionName]++;
    
    Logger::Warn("[TODO_OR_DIE] {}:{} - {} not yet implemented (call #{})",
                 file, line, functionName, _callCounts[functionName]);
}

void TodoOrDie::Fatal(const std::string& functionName) {
    Logger::Error("[TODO_OR_DIE] FATAL: {} must be implemented before use!", functionName);
    throw std::runtime_error("Not implemented: " + functionName);
}

void TodoOrDie::PrintStatistics() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_callCounts.empty()) {
        Logger::Info("No unimplemented functions called");
        return;
    }
    
    Logger::Info("=== NotImplemented Statistics ===");
    
    // 호출 횟수로 정렬
    std::vector<std::pair<std::string, size_t>> sorted(
        _callCounts.begin(), _callCounts.end());
    
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;  // 내림차순
        });
    
    // 상위 10개 출력
    Logger::Info("Top unimplemented functions:");
    for (size_t i = 0; i < std::min(size_t(10), sorted.size()); ++i) {
        Logger::Info("  {:3d} calls - {}", 
                     sorted[i].second, sorted[i].first);
    }
    
    // 전체 통계
    size_t totalCalls = 0;
    for (const auto& [func, count] : _callCounts) {
        totalCalls += count;
    }
    
    Logger::Info("Total: {} unique functions, {} total calls",
                 _callCounts.size(), totalCalls);
}

void TodoOrDie::ResetStatistics() {
    std::lock_guard<std::mutex> lock(_mutex);
    _callCounts.clear();
}

size_t TodoOrDie::GetCallCount(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _callCounts.find(functionName);
    return (it != _callCounts.end()) ? it->second : 0;
}

} // namespace pers::utils
```

## 사용 예제 (Usage Examples)

### 예제 1: 기본 스텁 구현
```cpp
class MockRenderer : public IRenderer {
public:
    std::shared_ptr<Buffer> CreateBuffer(const BufferDesc& desc) override {
        TODO_OR_DIE();  // 매크로 사용 - 파일, 라인, 함수명 자동 기록
        
        // 테스트를 위한 mock 객체 반환
        return std::make_shared<MockBuffer>(desc.size);
    }
    
    void UpdateBuffer(Buffer* buffer, const void* data, size_t size) override {
        TodoOrDie::Log("MockRenderer::UpdateBuffer");  // 직접 호출
        // 아무것도 하지 않음 - 나중에 구현
    }
};
```

### 예제 2: 필수 구현 표시
```cpp
class CriticalSystem {
public:
    void Initialize() {
        // 이 함수는 반드시 구현되어야 함
        TODO_OR_DIE_FATAL();  // 예외 발생
    }
    
    void OptionalFeature() {
        TODO_OR_DIE();  // 경고만 출력
    }
};
```

### 예제 3: 구현 우선순위 분석
```cpp
int main() {
    // 애플리케이션 실행
    RunApplication();
    
    // 종료시 통계 출력
    TodoOrDie::PrintStatistics();
    
    /* 출력 예시:
    === NotImplemented Statistics ===
    Top unimplemented functions:
      234 calls - Renderer::CreatePipeline
       89 calls - Renderer::CreateBuffer
       45 calls - CommandRecorder::SetBindGroup
       12 calls - Renderer::CreateTexture
        3 calls - Renderer::CreateSampler
    Total: 5 unique functions, 383 total calls
    
    → CreatePipeline이 가장 많이 호출되므로 우선 구현
    */
}
```

### 예제 4: 테스트에서 활용
```cpp
TEST(RendererTest, UnimplementedTracking) {
    auto renderer = CreateMockRenderer();
    
    // 테스트 실행
    renderer->CreateBuffer(bufferDesc);
    renderer->CreateBuffer(bufferDesc);
    renderer->CreatePipeline(pipelineDesc);
    
    // 호출 횟수 검증
    EXPECT_EQ(TodoOrDie::GetCallCount("MockRenderer::CreateBuffer"), 2);
    EXPECT_EQ(TodoOrDie::GetCallCount("MockRenderer::CreatePipeline"), 1);
    
    // 통계 초기화
    TodoOrDie::ResetStatistics();
}
```

### 예제 5: 점진적 구현
```cpp
// Phase 1: 모든 것이 NotImplemented
class RendererV1 : public IRenderer {
    void BeginFrame() override { TODO_OR_DIE(); }
    void EndFrame() override { TODO_OR_DIE(); }
    void Draw() override { TODO_OR_DIE(); }
};

// Phase 2: BeginFrame/EndFrame 구현
class RendererV2 : public IRenderer {
    void BeginFrame() override { 
        // 실제 구현
        _frameInProgress = true;
    }
    void EndFrame() override {
        // 실제 구현  
        _frameInProgress = false;
    }
    void Draw() override { TODO_OR_DIE(); }  // 아직 미구현
};

// Phase 3: 모든 것 구현 완료
class RendererV3 : public IRenderer {
    void BeginFrame() override { /* 구현 */ }
    void EndFrame() override { /* 구현 */ }
    void Draw() override { /* 구현 */ }
    // NotImplemented 호출 없음
};
```

## 빌드 설정

### CMake 통합
```cmake
# engine/CMakeLists.txt
add_library(pers_engine
    src/utils/TodoOrDie.cpp
    src/utils/Logger.cpp
    # ... 다른 소스 파일들
)

# NotImplemented 통계를 자동으로 출력하는 옵션
option(PERS_PRINT_NOT_IMPL_STATS "Print NotImplemented statistics on exit" ON)

if(PERS_PRINT_NOT_IMPL_STATS)
    target_compile_definitions(pers_engine PRIVATE PERS_PRINT_NOT_IMPL_STATS)
endif()
```

### 자동 통계 출력
```cpp
// main.cpp
#ifdef PERS_PRINT_NOT_IMPL_STATS
#include <cstdlib>

namespace {
    struct NotImplReporter {
        ~NotImplReporter() {
            pers::utils::TodoOrDie::PrintStatistics();
        }
    } _reporter;
}
#endif
```

## 모범 사례 (Best Practices)

### DO ✅
1. **초기 개발 단계에서 적극 활용**
   - 전체 구조를 먼저 잡고 세부 구현은 나중에

2. **Mock/Test 구현에서 사용**
   - 테스트용 더미 구현을 빠르게 만들 때

3. **통계 분석으로 우선순위 결정**
   - 가장 많이 호출되는 기능부터 구현

4. **명확한 함수명 사용**
   - `TodoOrDie::Log("ClassName::MethodName")`

### DON'T ❌
1. **Production 코드에 남기지 않기**
   - 릴리즈 전 모든 NotImplemented 제거 확인

2. **중요 기능에 사용하지 않기**
   - 초기화, 메모리 해제 등은 NOT_IMPL_FATAL 사용

3. **성능 크리티컬 경로에 남기지 않기**
   - 렌더링 루프 내부 등

## CI/CD 통합

### NotImplemented 검사 스크립트
```python
#!/usr/bin/env python3
# check_not_implemented.py

import subprocess
import sys

def check_not_implemented():
    # 소스 코드에서 NotImplemented 호출 검색
    result = subprocess.run(
        ['grep', '-r', 'TodoOrDie::', 'src/', '--include=*.cpp'],
        capture_output=True, text=True
    )
    
    if result.stdout:
        print("Warning: NotImplemented calls found:")
        print(result.stdout)
        
        # Production 빌드에서는 실패
        if os.environ.get('BUILD_TYPE') == 'Release':
            print("ERROR: NotImplemented not allowed in Release builds!")
            return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(check_not_implemented())
```

## 결론

`TodoOrDie`는 Pers Graphics Engine의 점진적 개발을 가능하게 하는 핵심 도구입니다:

1. **컴파일 가능한 상태 유지** - 전체 구조를 먼저 구성
2. **구현 추적** - 어떤 기능이 필요한지 명확히 파악
3. **우선순위 결정** - 통계 기반 개발 순서 결정
4. **테스트 지원** - Mock 구현을 빠르게 생성

이를 통해 **"항상 컴파일되고 실행 가능한"** 코드베이스를 유지하면서도 체계적으로 기능을 구현해 나갈 수 있습니다.