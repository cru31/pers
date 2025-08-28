# 🚨 Priority 0 - READ THIS FIRST!

## 이 폴더의 문서를 가장 먼저 읽으세요!

이 폴더는 Pers Graphics Engine 프로젝트에 참여하는 **모든 AI와 개발자가 반드시 첫 번째로 읽어야 할** 문서들을 포함합니다.

---

## 📋 필수 읽기 순서

### 1️⃣ AI_ONBOARDING_GUIDE.md (이 문서부터!)
- **읽는 시간**: 10분
- **내용**: 
  - 전체 문서 학습 순서
  - 로드맵 단계별 참조 가이드
  - LearnWebGPU 매핑
  - 구현 체크리스트

### 2️⃣ 프로젝트 핵심 문서 (순서대로)
1. `../CLAUDE.md` - 렌더러 구현 가이드 (10분)
2. `../FIRST_PRINCIPLES.md` - 설계 철학 (10분)
3. `../command-based-renderer.md` - API 설계 (5분)

---

## ⚡ Quick Start

```bash
# 1. 이 문서들을 읽은 후
# 2. 작업 디렉토리로 이동
cd D:\cru31.dev\pers_graphics\pers

# 3. 빌드 확인
cmake -B build
cmake --build build

# 4. Mock 구현부터 시작!
```

---

## 🎯 핵심 요약

### 프로젝트 목표
**Vanilla WebGPU와 동일한 GPU 명령을 생성하는 고수준 렌더링 엔진 구축**

### 핵심 원칙
1. **명시적 제어**: 숨겨진 동작 없음
2. **Top-Down 개발**: API 먼저, 구현은 나중에
3. **Trace 검증**: 95% 이상 일치율 목표

### 핵심 API
```cpp
// 이것만 기억하세요!
auto cmd = renderer->BeginFrame();
cmd->BeginRenderPass(nullptr);  // nullptr = 백버퍼
cmd->Draw(...);
cmd->EndRenderPass();  // 반드시 명시적 종료!
renderer->EndFrame(cmd);
```

---

## 📚 다음 단계

1. ✅ 이 README를 읽었다면
2. ➡️ `AI_ONBOARDING_GUIDE.md` 읽기
3. ➡️ 가이드에 명시된 순서대로 문서 학습
4. ➡️ Mock 구현 시작

---

## ⚠️ 경고

**이 문서들을 읽지 않고 시작하면:**
- ❌ 잘못된 설계 결정
- ❌ 프로젝트 규칙 위반
- ❌ 시간 낭비와 재작업

**반드시 순서대로 읽고 시작하세요!**

---

마지막 업데이트: 2024-01-25