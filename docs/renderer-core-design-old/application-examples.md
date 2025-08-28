# Command Renderer - Application Examples

## Overview
이 문서는 Command-Based Renderer API를 실제 애플리케이션에서 사용하는 다양한 예제를 제공합니다.

## 1. Basic Application - 단순 큐브 렌더링

### 1.1 최소한의 렌더링
```cpp
class MinimalApp : public Application {
private:
    std::unique_ptr<Scene> scene;
    Mesh* cubeMesh;
    Material* basicMaterial;
    
public:
    bool OnInitialize() override {
        // 씬 생성
        scene = std::make_unique<Scene>();
        
        // 기본 큐브 메쉬 생성
        cubeMesh = MeshBuilder::CreateCube(2.0f);
        
        // 기본 머티리얼 생성
        basicMaterial = Material::Create("shaders/basic.wgsl");
        basicMaterial->SetVector("color", {1.0f, 0.0f, 0.0f, 1.0f});
        
        // 카메라 설정
        auto camera = scene->CreateCamera();
        camera->SetPosition({0, 0, 5});
        camera->LookAt({0, 0, 0});
        
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        // 큐브 회전
        static float rotation = 0.0f;
        rotation += deltaTime * 45.0f;  // 45 degrees per second
        
        cubeMesh->SetRotation({0, rotation, 0});
    }
    
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        // 백버퍼에 렌더링 (nullptr = 백버퍼)
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        
        // 간단한 씬 렌더링
        cmd->RenderScene(scene.get());
        
        // 패스 종료 - 명시적 호출 필수
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 2. Multi-Pass Rendering - 그림자 맵핑

### 2.1 그림자가 있는 씬
```cpp
class ShadowMappingApp : public Application {
private:
    std::unique_ptr<Scene> scene;
    std::unique_ptr<RenderTarget> shadowMap;
    Light* directionalLight;
    Camera* shadowCamera;
    
public:
    bool OnInitialize() override {
        // 그림자 맵 생성 (2048x2048)
        shadowMap = RenderTarget::CreateDepthOnly(2048, 2048);
        
        // 씬 설정
        scene = std::make_unique<Scene>();
        scene->LoadFromFile("assets/scenes/courtyard.scene");
        
        // 방향광 설정
        directionalLight = scene->CreateDirectionalLight();
        directionalLight->SetDirection({-1, -1, -1});
        directionalLight->SetIntensity(2.0f);
        directionalLight->EnableShadows(true);
        
        // 그림자 카메라 설정 (orthographic)
        shadowCamera = Camera::CreateOrthographic(-10, 10, -10, 10, 0.1f, 100.0f);
        shadowCamera->SetPosition(directionalLight->GetDirection() * -20.0f);
        shadowCamera->LookAt({0, 0, 0});
        
        return true;
    }
    
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        // Pass 1: Shadow Map 렌더링
        cmd->BeginRenderPass(shadowMap.get(), ClearValue::Depth(1.0f));
        cmd->SetPipeline(shadowPipeline);
        cmd->RenderScene(scene.get(), shadowCamera);
        cmd->EndRenderPass();
        
        // Pass 2: 메인 렌더링 (백버퍼)
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        
        // 그림자 맵을 바인딩
        cmd->SetBindGroup(0, frameData);  // View/Proj
        cmd->SetBindGroup(1, CreateShadowBindGroup(shadowMap.get()));
        
        cmd->RenderScene(scene.get(), scene->GetMainCamera());
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 3. Deferred Rendering - G-Buffer 기반

### 3.1 Deferred Rendering Pipeline
```cpp
class DeferredRenderingApp : public Application {
private:
    struct GBuffer {
        std::unique_ptr<RenderTarget> albedo;
        std::unique_ptr<RenderTarget> normal;
        std::unique_ptr<RenderTarget> material;
        std::unique_ptr<RenderTarget> depth;
    } gbuffer;
    
    std::unique_ptr<Scene> scene;
    Pipeline* geometryPipeline;
    Pipeline* lightingPipeline;
    
public:
    bool OnInitialize() override {
        uint32_t width = 1920, height = 1080;
        
        // G-Buffer 생성
        gbuffer.albedo = RenderTarget::Create(width, height, Format::RGBA8);
        gbuffer.normal = RenderTarget::Create(width, height, Format::RG16F);
        gbuffer.material = RenderTarget::Create(width, height, Format::RGBA8);
        gbuffer.depth = RenderTarget::CreateDepth(width, height);
        
        // 파이프라인 생성
        geometryPipeline = CreateGeometryPipeline();
        lightingPipeline = CreateLightingPipeline();
        
        // 씬 로드
        scene = std::make_unique<Scene>();
        scene->LoadFromFile("assets/scenes/sponza.scene");
        
        return true;
    }
    
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        // Pass 1: Geometry Pass (G-Buffer 채우기)
        RenderTarget* gBufferTargets[] = {
            gbuffer.albedo.get(),
            gbuffer.normal.get(),
            gbuffer.material.get()
        };
        
        cmd->BeginRenderPassMRT(gBufferTargets, 3, gbuffer.depth.get());
        cmd->SetPipeline(geometryPipeline);
        
        // 모든 지오메트리 렌더링
        for (const auto& renderable : scene->GetRenderables()) {
            cmd->SetBindGroup(2, renderable.material->GetBindGroup());
            cmd->SetBindGroup(3, renderable.transform->GetBindGroup());
            cmd->DrawMesh(renderable.mesh);
        }
        
        cmd->EndRenderPass();
        
        // Pass 2: Lighting Pass (백버퍼에 최종 렌더)
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        cmd->SetPipeline(lightingPipeline);
        
        // G-Buffer 텍스처들을 바인딩
        auto gBufferBindGroup = BindGroup::Create({
            {0, gbuffer.albedo->GetTexture()},
            {1, gbuffer.normal->GetTexture()},
            {2, gbuffer.material->GetTexture()},
            {3, gbuffer.depth->GetTexture()}
        });
        cmd->SetBindGroup(0, gBufferBindGroup);
        
        // 라이트 데이터 바인딩
        cmd->SetBindGroup(1, scene->GetLightBindGroup());
        
        // 전체 화면 쿼드 렌더링
        cmd->Draw(3);  // Fullscreen triangle trick
        
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 4. Post-Processing Pipeline

### 4.1 포스트 프로세싱 체인
```cpp
class PostProcessingApp : public Application {
private:
    std::unique_ptr<RenderTarget> hdrBuffer;
    std::unique_ptr<RenderTarget> bloomBuffer;
    std::unique_ptr<RenderTarget> blurBuffer;
    
    Pipeline* scenePipeline;
    Pipeline* bloomExtractPipeline;
    Pipeline* blurPipeline;
    Pipeline* compositePipeline;
    
public:
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        // Pass 1: HDR 씬 렌더링
        cmd->BeginRenderPass(hdrBuffer.get(), ClearValue::Black());
        cmd->SetPipeline(scenePipeline);
        cmd->RenderScene(scene.get());
        cmd->EndRenderPass();
        
        // Pass 2: Bloom 추출
        cmd->BeginRenderPass(bloomBuffer.get());
        cmd->SetPipeline(bloomExtractPipeline);
        cmd->SetTexture(0, hdrBuffer->GetTexture());
        cmd->SetFloat("threshold", 1.0f);
        cmd->Draw(3);  // Fullscreen triangle
        cmd->EndRenderPass();
        
        // Pass 3: Gaussian Blur (Horizontal)
        cmd->BeginRenderPass(blurBuffer.get());
        cmd->SetPipeline(blurPipeline);
        cmd->SetTexture(0, bloomBuffer->GetTexture());
        cmd->SetVector2("direction", {1, 0});
        cmd->Draw(3);
        cmd->EndRenderPass();
        
        // Pass 4: Gaussian Blur (Vertical) 
        cmd->BeginRenderPass(bloomBuffer.get());
        cmd->SetPipeline(blurPipeline);
        cmd->SetTexture(0, blurBuffer->GetTexture());
        cmd->SetVector2("direction", {0, 1});
        cmd->Draw(3);
        cmd->EndRenderPass();
        
        // Pass 5: 최종 합성 (백버퍼)
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        cmd->SetPipeline(compositePipeline);
        cmd->SetTexture(0, hdrBuffer->GetTexture());
        cmd->SetTexture(1, bloomBuffer->GetTexture());
        cmd->SetFloat("exposure", 1.2f);
        cmd->SetFloat("bloomIntensity", 0.5f);
        cmd->Draw(3);
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 5. Instanced Rendering

### 5.1 대량 인스턴스 렌더링
```cpp
class InstancedRenderingApp : public Application {
private:
    struct InstanceData {
        glm::mat4 transform;
        glm::vec4 color;
    };
    
    std::vector<InstanceData> instances;
    std::unique_ptr<Buffer> instanceBuffer;
    Mesh* grassMesh;
    
public:
    bool OnInitialize() override {
        // 10,000개 풀 인스턴스 생성
        instances.reserve(10000);
        for (int i = 0; i < 100; ++i) {
            for (int j = 0; j < 100; ++j) {
                InstanceData instance;
                instance.transform = glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(i * 2.0f, 0, j * 2.0f)
                );
                instance.color = glm::vec4(
                    0.0f, 
                    0.5f + RandomFloat() * 0.5f,  // Green variation
                    0.0f, 
                    1.0f
                );
                instances.push_back(instance);
            }
        }
        
        // 인스턴스 버퍼 생성
        instanceBuffer = Buffer::Create(
            BufferType::Vertex,
            instances.data(),
            instances.size() * sizeof(InstanceData)
        );
        
        grassMesh = MeshBuilder::CreateGrass();
        
        return true;
    }
    
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        cmd->BeginRenderPass(nullptr, ClearValue::SkyBlue());
        
        // 인스턴스드 렌더링 설정
        cmd->SetPipeline(instancedPipeline);
        cmd->SetVertexBuffer(0, grassMesh->GetVertexBuffer());
        cmd->SetVertexBuffer(1, instanceBuffer.get());  // Instance data
        cmd->SetIndexBuffer(grassMesh->GetIndexBuffer());
        
        // 프레임 데이터 바인딩
        cmd->SetBindGroup(0, frameDataBindGroup);
        
        // 10,000개 인스턴스를 한 번의 드로우 콜로
        cmd->DrawIndexedInstanced(
            grassMesh->GetIndexCount(),
            instances.size()
        );
        
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 6. Multi-Viewport Rendering

### 6.1 에디터 스타일 멀티 뷰포트
```cpp
class EditorApp : public Application {
private:
    struct Viewport {
        std::unique_ptr<RenderTarget> target;
        std::unique_ptr<Camera> camera;
        glm::vec4 rect;  // x, y, width, height in normalized coords
    };
    
    std::vector<Viewport> viewports;
    
public:
    bool OnInitialize() override {
        // 4개 뷰포트 설정 (Top, Front, Side, Perspective)
        viewports.resize(4);
        
        // Top view
        viewports[0].target = RenderTarget::Create(960, 540);
        viewports[0].camera = Camera::CreateOrthographic(-10, 10, -10, 10);
        viewports[0].camera->SetPosition({0, 10, 0});
        viewports[0].camera->LookAt({0, 0, 0}, {0, 0, -1});
        viewports[0].rect = {0, 0, 0.5f, 0.5f};
        
        // Front view
        viewports[1].target = RenderTarget::Create(960, 540);
        viewports[1].camera = Camera::CreateOrthographic(-10, 10, -10, 10);
        viewports[1].camera->SetPosition({0, 0, 10});
        viewports[1].camera->LookAt({0, 0, 0});
        viewports[1].rect = {0.5f, 0, 0.5f, 0.5f};
        
        // Side view
        viewports[2].target = RenderTarget::Create(960, 540);
        viewports[2].camera = Camera::CreateOrthographic(-10, 10, -10, 10);
        viewports[2].camera->SetPosition({10, 0, 0});
        viewports[2].camera->LookAt({0, 0, 0});
        viewports[2].rect = {0, 0.5f, 0.5f, 0.5f};
        
        // Perspective view
        viewports[3].target = RenderTarget::Create(960, 540);
        viewports[3].camera = Camera::CreatePerspective(60.0f, 16.0f/9.0f);
        viewports[3].camera->SetPosition({5, 5, 5});
        viewports[3].camera->LookAt({0, 0, 0});
        viewports[3].rect = {0.5f, 0.5f, 0.5f, 0.5f};
        
        return true;
    }
    
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        // 각 뷰포트 렌더링
        for (auto& viewport : viewports) {
            cmd->BeginRenderPass(viewport.target.get(), ClearValue::DarkGray());
            
            // 뷰포트별 카메라 설정
            cmd->SetViewport(viewport.rect);
            cmd->RenderScene(scene.get(), viewport.camera.get());
            
            // 그리드 렌더링 (에디터 스타일)
            cmd->SetPipeline(gridPipeline);
            cmd->Draw(6);  // Grid quad
            
            cmd->EndRenderPass();
        }
        
        // 최종 합성 (백버퍼에 4개 뷰포트 배치)
        cmd->BeginRenderPass(nullptr, ClearValue::Black());
        cmd->SetPipeline(blitPipeline);
        
        for (size_t i = 0; i < viewports.size(); ++i) {
            cmd->SetTexture(0, viewports[i].target->GetTexture());
            cmd->SetVector4("targetRect", viewports[i].rect);
            cmd->Draw(6);  // Quad
        }
        
        cmd->EndRenderPass();
        
        renderer->EndFrame(cmd);
    }
};
```

## 7. Error Handling Examples

### 7.1 올바른 사용 패턴
```cpp
void CorrectUsage() {
    auto* renderer = GetRenderer();
    auto* cmd = renderer->BeginFrame();
    
    // ✅ 올바른 사용: 명시적 EndRenderPass
    cmd->BeginRenderPass(nullptr);
    cmd->RenderScene(scene);
    cmd->EndRenderPass();  // 반드시 호출
    
    // ✅ 멀티패스도 각각 종료
    cmd->BeginRenderPass(target1);
    cmd->Draw(6);
    cmd->EndRenderPass();
    
    cmd->BeginRenderPass(target2);
    cmd->Draw(6);
    cmd->EndRenderPass();
    
    renderer->EndFrame(cmd);
}
```

### 7.2 잘못된 사용 패턴과 에러
```cpp
void IncorrectUsage() {
    auto* renderer = GetRenderer();
    auto* cmd = renderer->BeginFrame();
    
    // ❌ 에러: EndRenderPass 없이 EndFrame
    cmd->BeginRenderPass(nullptr);
    cmd->RenderScene(scene);
    // cmd->EndRenderPass();  // 빠뜨림!
    
    try {
        renderer->EndFrame(cmd);  // RuntimeError 발생
    } catch (const std::runtime_error& e) {
        // "RenderPass still active"
    }
    
    // ❌ 에러: 중첩된 RenderPass
    cmd->BeginRenderPass(target1);
    cmd->BeginRenderPass(target2);  // RuntimeError: "Previous pass not ended"
    
    // ❌ 에러: RenderPass 밖에서 드로우
    cmd->Draw(6);  // RuntimeError: "Not in RenderPass"
}
```

## 8. Performance Tips

### 8.1 최적화된 렌더링 패턴
```cpp
class OptimizedApp : public Application {
    void OnRender() override {
        auto* renderer = GetRenderer();
        auto* cmd = renderer->BeginFrame();
        
        cmd->BeginRenderPass(nullptr);
        
        // 1. 파이프라인별로 정렬
        std::sort(renderables.begin(), renderables.end(), 
            [](const auto& a, const auto& b) {
                return a.pipeline < b.pipeline;
            });
        
        Pipeline* currentPipeline = nullptr;
        Material* currentMaterial = nullptr;
        
        for (const auto& renderable : renderables) {
            // 2. 상태 변경 최소화
            if (renderable.pipeline != currentPipeline) {
                cmd->SetPipeline(renderable.pipeline);
                currentPipeline = renderable.pipeline;
            }
            
            if (renderable.material != currentMaterial) {
                cmd->SetBindGroup(2, renderable.material->GetBindGroup());
                currentMaterial = renderable.material;
            }
            
            // 3. 동적 오프셋 사용
            cmd->SetBindGroup(3, objectDataBindGroup, renderable.dynamicOffset);
            cmd->DrawIndexed(renderable.indexCount);
        }
        
        cmd->EndRenderPass();
        renderer->EndFrame(cmd);
    }
};
```

## Summary

Command-Based Renderer API는 다양한 렌더링 시나리오를 지원합니다:

1. **단순함**: 기본 렌더링은 몇 줄로 가능
2. **유연성**: 멀티패스, 디퍼드, 포스트프로세싱 등 복잡한 파이프라인 지원
3. **명시성**: 모든 RenderPass는 명시적으로 시작하고 종료
4. **안전성**: 런타임 검증으로 실수 방지
5. **성능**: 배칭, 인스턴싱, 상태 변경 최소화 지원

핵심 규칙:
- `BeginRenderPass()` 후 반드시 `EndRenderPass()`
- `nullptr`은 현재 백버퍼를 의미
- 모든 드로우 콜은 RenderPass 내에서만 가능