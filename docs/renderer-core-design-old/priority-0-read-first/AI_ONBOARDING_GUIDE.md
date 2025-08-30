# AI 학습 가이드 - Pers Graphics Engine 설계 문서

## 🚨 AI는 이 순서대로 설계 문서를 학습하세요

이 가이드는 AI가 Pers Graphics Engine 구현을 시작하기 전에 **반드시 학습해야 할 설계 문서들의 정확한 순서**를 명시합니다.

---

## 📚 설계 문서 학습 순서

### STEP 1: 핵심 철학과 원칙 이해 (필수 - 먼저 읽기)

#### 1.1 프로젝트 기본 규칙
```
${project_root}/docs/renderer-core-design/CLAUDE.md
- 왜: 프로젝트 전체 규칙, 코딩 스타일, OCP/SOLID 원칙
- 핵심: 네이밍 컨벤션, 포인터 사용 규칙 (raw pointer 금지, shared_ptr 사용)
```

#### 1.2 렌더러 설계 철학
```
${project_root}/docs/renderer-core-design/FIRST_PRINCIPLES.md
- 왜: GPU 동작 원리와 우리 설계의 근본 이유
- 핵심: 명시적 제어, BeginRenderPass/EndRenderPass 필수, nullptr = 백버퍼
```

#### 1.3 렌더러 구현 가이드
```
${project_root}/docs/renderer-core-design/CLAUDE.md
- 왜: 렌더러 구체적 구현 방향, 기술 스택
- 핵심: Command-Based Rendering, Triple Buffering, 에러 처리
```

### STEP 2: API 설계 이해 (필수 - 구현 전 완독)

#### 2.1 Command-Based Renderer API
```
${project_root}/docs/renderer-core-design/command-based-renderer.md
- 왜: 핵심 API 설계와 각 결정의 근거
- 핵심: 왜 BeginRenderPass(nullptr)인지, 왜 명시적 EndRenderPass인지
```

#### 2.2 리소스 바인딩 모델
```
${project_root}/docs/renderer-core-design/resource-binding-model.md
- 왜: GPU 리소스 관리 전략
- 핵심: 4단계 바인딩 계층 (Frame/Pass/Material/Object)
```

### STEP 3: 구현 로드맵과 검증 (필수 - 작업 시작 전)

#### 3.1 전체 로드맵
```
${project_root}/docs/renderer-core-design/ROADMAP.md
- 왜: 10주 개발 계획과 마일스톤
- 핵심: Top-Down 접근, Mock → 실제 구현 순서
```

#### 3.2 검증 방법론
```
${project_root}/docs/renderer-core-design/roadmap/VALIDATION_METHODOLOGY.md
- 왜: Vanilla WebGPU와 동일한 GPU 명령 생성 검증
- 핵심: Trace 기반 검증, 95% 일치율 목표
```

#### 3.3 상세 구현 계획
```
${project_root}/docs/renderer-core-design/roadmap/IMPLEMENTATION_PLAN.md
- 왜: 주차별 구체적 작업 내용
- 핵심: 디렉토리 구조, Phase별 구현 순서
```

### STEP 4: 도구와 테스트 (참조용)

#### 4.1 TodoOrDie 가이드
```
${project_root}/docs/renderer-core-design/roadmap/NOTIMPLEMENTED_GUIDE.md
- 왜: 점진적 구현을 위한 스텁 시스템
- 언제 읽기: Mock 구현 시작할 때
```

#### 4.2 테스트 명세
```
${project_root}/docs/renderer-core-design/roadmap/TEST_SPECIFICATIONS.md
- 왜: 각 레벨별 테스트 케이스
- 언제 읽기: 각 구현 단계 완료 후
```

---

## 🎯 로드맵 단계별 설계 문서 매핑

### Phase 0: Foundation Setup (Week 0)

**반드시 읽어야 할 설계 문서:**
1. `./CLAUDE.md` - 프로젝트 규칙 숙지
2. `FIRST_PRINCIPLES.md` - 왜 이렇게 설계했는지 이해
3. `NOTIMPLEMENTED_GUIDE.md` - 스텁 구현 방법

**이 단계의 목표:**
- 프로젝트 구조 생성
- CMake 빌드 시스템 구성
- Logger, TodoOrDie 유틸리티 구현

**LearnWebGPU 참조:**
```
필요 없음 - 아직 WebGPU 구현 단계 아님
```

### Phase 1: Core Interfaces & Mock (Week 1-2)

**반드시 읽어야 할 설계 문서:**
1. `command-based-renderer.md` - IRenderer, ICommandRecorder API 정의
2. `CLAUDE.md` (렌더러) - 인터페이스 설계 규칙
3. `IMPLEMENTATION_PLAN.md` - Phase 1 섹션

**이 단계의 목표:**
- IRenderer, ICommandRecorder 인터페이스 정의
- MockRenderer, MockCommandRecorder 구현
- BeginFrame/EndFrame, BeginRenderPass/EndRenderPass 동작

**LearnWebGPU 참조:**
```
Chapter 3.1-3.2: Adapter and Device 개념만 이해
- https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device.html
- 실제 구현 아님, 개념 이해용
```

### Phase 2: Resource Management (Week 3-4)

**반드시 읽어야 할 설계 문서:**
1. `resource-binding-model.md` - 전체 읽기 (특히 4단계 계층 구조)
2. `IMPLEMENTATION_PLAN.md` - Phase 2 섹션
3. `command-based-renderer.md` - Resource 관련 API 부분

**이 단계의 목표:**
- Pipeline, Buffer, Texture, BindGroup 클래스 정의
- 리소스 생성 API 구현
- Triangle 샘플 작성

**LearnWebGPU 참조:**
```
Chapter 4.3: Uniforms and bind groups
- https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/some-interaction/uniforms.html
- BindGroup 개념 이해 필수

Chapter 4.4: Texturing
- https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/texturing/index.html
- Texture와 Sampler 개념
```

### Phase 3: Trace Validation System (Week 5-6)

**반드시 읽어야 할 설계 문서:**
1. `VALIDATION_METHODOLOGY.md` - 전체 읽기 (핵심)
2. `roadmap-validation/TRACE_VALIDATION_SYSTEM.md` - 구현 상세
3. `TEST_SPECIFICATIONS.md` - Level 0-2 테스트 케이스

**이 단계의 목표:**
- TraceLogger 구현
- Vanilla WebGPU 샘플 작성
- Pers Mock 샘플 작성
- Trace 비교 도구 구현

**LearnWebGPU 참조:**
```
필요 없음 - Trace 시스템은 우리 고유 설계
```

### Phase 4: WebGPU Backend (Week 7-8)

**반드시 읽어야 할 설계 문서:**
1. `command-based-renderer.md` - 다시 정독 (모든 API)
2. `FIRST_PRINCIPLES.md` - GPU 동작 원리 재확인
3. `IMPLEMENTATION_PLAN.md` - Phase 4 섹션

**이 단계의 목표:**
- 실제 WebGPU 백엔드 구현
- WebGPURenderer, WebGPUCommandRecorder
- 실제 GPU에서 Triangle 렌더링

**LearnWebGPU 필수 참조:**
```
Chapter 3.3-3.5: The Command Queue
- https://eliemichel.github.io/LearnWebGPU/getting-started/the-command-queue.html
- Command Encoder와 Command Buffer 이해

Chapter 3.6: The Swap Chain  
- https://eliemichel.github.io/LearnWebGPU/getting-started/the-swap-chain.html
- SwapChain과 백버퍼 관리

Chapter 4.1: Drawing a triangle
- https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
- RenderPipeline 생성과 Draw 호출

Chapter 4.2: Render Passes
- https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html#render-pass
- BeginRenderPass/EndRenderPass 구현
```

### Phase 5: Multi-Pass & Optimization (Week 9-10)

**반드시 읽어야 할 설계 문서:**
1. `TEST_SPECIFICATIONS.md` - Level 4-5 (Multi-pass 테스트)
2. `ROADMAP.md` - Phase 5-6 섹션
3. `resource-binding-model.md` - Dynamic offset 최적화

**이 단계의 목표:**
- Shadow mapping 구현
- Post-processing chain
- Command Pool 최적화
- 성능 벤치마크

**LearnWebGPU 참조:**
```
Chapter 5.1: Multiple Render Targets
- https://eliemichel.github.io/LearnWebGPU/advanced-techniques/hdr/index.html
- 여러 렌더 타겟 관리

Chapter 5.2: Shadow Mapping
- https://eliemichel.github.io/LearnWebGPU/advanced-techniques/shadow-mapping/index.html
- Depth texture와 shadow pass
```

---

## 📊 설계 문서 학습 완료 체크리스트

### 필수 설계 문서 (반드시 순서대로)
- [ ] `${project_root}/CLAUDE.md` - 프로젝트 규칙
- [ ] `${project_root}/docs/renderer-core-design/FIRST_PRINCIPLES.md` - 철학과 원칙  
- [ ] `${project_root}/docs/renderer-core-design/CLAUDE.md` - 렌더러 가이드
- [ ] `${project_root}/docs/renderer-core-design/command-based-renderer.md` - API 설계
- [ ] `${project_root}/docs/renderer-core-design/resource-binding-model.md` - 리소스 바인딩
- [ ] `${project_root}/docs/renderer-core-design/ROADMAP.md` - 전체 계획
- [ ] `${project_root}/docs/renderer-core-design/roadmap/VALIDATION_METHODOLOGY.md` - 검증 방법
- [ ] `${project_root}/docs/renderer-core-design/roadmap/IMPLEMENTATION_PLAN.md` - 상세 계획

### LearnWebGPU 필수 챕터
- [ ] Chapter 3: Getting Started (Device, Queue 개념)
- [ ] Chapter 4.1: Hello Triangle (Pipeline, Draw)
- [ ] Chapter 4.3: Uniforms (BindGroup)
- [ ] Chapter 4.4: Texturing (Texture, Sampler)

---

## ⚠️ 중요 주의사항

### 설계 문서를 읽지 않고 구현하면:
1. **잘못된 API 설계** - BeginRenderPass 자동 종료 등
2. **규칙 위반** - raw pointer 사용, 잘못된 네이밍
3. **검증 실패** - Trace 불일치로 재작업

### 반드시 기억할 핵심:
```cpp
// ❌ 잘못된 예
cmd->BeginRenderPass();
renderer->EndFrame(cmd);  // RenderPass 자동 종료? NO!

// ✅ 올바른 예  
cmd->BeginRenderPass(nullptr);  // nullptr = 백버퍼
cmd->EndRenderPass();  // 반드시 명시적 종료
renderer->EndFrame(cmd);
```

---

## 📋 단계별 설계 문서 빠른 참조

| 단계 | 필수 설계 문서 | LearnWebGPU 참조 |
|------|-----------------|------------------|
| Phase 0 | CLAUDE.md, FIRST_PRINCIPLES.md | 없음 |
| Phase 1 | command-based-renderer.md | Ch 3.1-3.2 |
| Phase 2 | resource-binding-model.md | Ch 4.3-4.4 |
| Phase 3 | VALIDATION_METHODOLOGY.md | 없음 |
| Phase 4 | command-based-renderer.md (재독) | Ch 3.3-3.6, 4.1-4.2 |
| Phase 5 | TEST_SPECIFICATIONS.md | Ch 5.1-5.2 |



---

## 🚀 AI가 해야 할 일

### 1. 설계 문서 학습 (STEP 1-4 순서대로)
1. 먼저 이 가이드를 완독
2. STEP 1부터 순서대로 모든 설계 문서 학습
3. 각 Phase별 필수 문서 파악

### 2. 구현 시작
1. Phase 0: 프로젝트 구조 생성
2. Phase 1: Mock 구현
3. Phase 2: 리소스 시스템
4. Phase 3: Trace 검증
5. Phase 4: WebGPU 구현
6. Phase 5: 최적화

### 3. 각 Phase에서 참조할 문서
- 위의 "로드맵 단계별 설계 문서 매핑" 섹션 참조
- LearnWebGPU는 명시된 챕터만 참고

---

## 📌 최종 정리

**AI는 이 순서를 반드시 따라야 합니다:**

1. 이 가이드 완독
2. STEP 1-4 순서대로 설계 문서 학습
3. Phase별 구현 시작
4. 각 Phase에서 해당 설계 문서 참조
5. Trace 검증으로 정확성 확인

**핵심 원칙:** 
- BeginRenderPass(nullptr) = 백버퍼
- EndRenderPass() 반드시 명시적 호출
- Raw pointer 금지, shared_ptr 사용
- TodoOrDie로 점진적 구현