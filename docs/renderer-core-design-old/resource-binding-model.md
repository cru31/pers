# Resource Binding Model 설계

## 개요

모던 GPU는 리소스(버퍼, 텍스처, 샘플러)를 Descriptor Set/Bind Group 단위로 바인딩합니다. 효율적인 렌더링을 위해 리소스를 업데이트 빈도별로 계층화하여 관리합니다.

## 바인딩 빈도 계층 (Binding Frequency Tiers)

### Set 0: Per-Frame Resources
**업데이트 빈도**: 프레임당 1회  
**용도**: 전체 프레임에서 공통으로 사용되는 데이터

```cpp
struct FrameData {
    // View & Projection
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 viewProjMatrix;
    mat4 invViewProjMatrix;
    
    // Camera
    vec3 cameraPosition;
    float nearPlane;
    vec3 cameraDirection;
    float farPlane;
    
    // Time
    float time;
    float deltaTime;
    uint32_t frameCount;
    float _padding;
    
    // Screen
    vec2 screenSize;
    vec2 invScreenSize;
};

struct GlobalLightData {
    // Directional lights
    DirectionalLight sun;
    
    // IBL
    vec3 ambientLight;
    float ambientIntensity;
    
    // Fog
    vec3 fogColor;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec2 _padding;
};
```

**리소스 예시**:
- View/Projection 매트릭스
- 시간 정보
- 전역 조명 데이터
- IBL 큐브맵
- 쉐도우 맵 배열
- SSAO 노이즈 텍스처

### Set 1: Per-Pass Resources  
**업데이트 빈도**: 렌더 패스당 1회  
**용도**: 특정 렌더 패스에서만 사용되는 데이터

```cpp
struct PassData {
    // Pass info
    uint32_t passType;  // 0=depth, 1=forward, 2=deferred, 3=shadow
    uint32_t passIndex; // Multi-pass index
    vec2 _padding;
    
    // Render target info
    vec4 viewport;  // x, y, width, height
    vec2 targetSize;
    vec2 invTargetSize;
    
    // Pass-specific
    mat4 lightViewProj;  // For shadow pass
    uint32_t cascadeIndex;  // For cascaded shadows
    float splitDistance;
    vec2 _padding2;
};
```

**리소스 예시**:
- 패스별 뷰포트 정보
- 렌더 타겟 크기
- 쉐도우 패스용 라이트 매트릭스
- 포스트프로세싱 파라미터
- 이전 패스 결과 텍스처

### Set 2: Per-Material Resources
**업데이트 빈도**: 머티리얼 변경시  
**용도**: 머티리얼별 속성과 텍스처

```cpp
struct MaterialData {
    // PBR parameters
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    
    vec3 emissiveFactor;
    float emissiveStrength;
    
    // Texture flags
    uint32_t textureFlags;  // Bit flags for which textures are bound
    uint32_t alphaMode;     // 0=opaque, 1=blend, 2=mask
    float alphaCutoff;
    float _padding;
    
    // Advanced
    vec2 uvScale;
    vec2 uvOffset;
};
```

**리소스 예시**:
- Base color 텍스처
- Normal map
- Metallic-Roughness 텍스처
- Occlusion map
- Emissive 텍스처
- 머티리얼 상수 버퍼

### Set 3: Per-Object Resources
**업데이트 빈도**: 오브젝트(드로우 콜)당 1회  
**용도**: 개별 오브젝트 트랜스폼과 인스턴스 데이터

```cpp
struct ObjectData {
    // Transform
    mat4 modelMatrix;
    mat4 normalMatrix;  // transpose(inverse(modelMatrix))
    mat4 prevModelMatrix;  // For motion vectors
    
    // Instance data
    vec4 instanceColor;
    uint32_t objectID;
    uint32_t batchID;
    vec2 _padding;
    
    // Skinning
    uint32_t boneOffset;
    uint32_t boneCount;
    vec2 _padding2;
};

struct BoneData {
    mat4 boneMatrices[MAX_BONES];  // For skinned meshes
};
```

**리소스 예시**:
- Model 매트릭스
- Normal 매트릭스  
- 오브젝트 ID (picking용)
- 인스턴스 데이터
- 스키닝 매트릭스

## 추가 고려사항

### Set 4: Per-View Resources (Optional)
**업데이트 빈도**: 뷰포트/카메라 변경시  
**용도**: 멀티 뷰포트 렌더링, VR 스테레오 렌더링

```cpp
struct ViewData {
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 eyePosition;
    float _padding;
    
    // Frustum planes for culling
    vec4 frustumPlanes[6];
    
    // Viewport
    vec4 viewport;
};
```

**사용 시나리오**:
- 에디터의 멀티 뷰포트
- VR 좌/우 눈 렌더링
- 큐브맵 페이스 렌더링
- 화면 분할 멀티플레이어

### Set 5: Per-Light Resources (Optional)
**업데이트 빈도**: 라이트별  
**용도**: Forward+ 또는 Clustered Deferred에서 라이트별 데이터

```cpp
struct LightData {
    vec3 position;
    float range;
    vec3 color;
    float intensity;
    vec3 direction;
    float innerCone;
    float outerCone;
    uint32_t type;
    vec2 _padding;
    
    mat4 lightViewProj;  // For shadow mapping
    uint32_t shadowMapIndex;
    uint32_t _padding2[3];
};
```

## 바인딩 레이아웃 설계

### 기본 4-Set 레이아웃
```cpp
enum class BindGroupIndex : uint32_t {
    Frame = 0,     // 가장 적게 변경
    Pass = 1,      
    Material = 2,  
    Object = 3     // 가장 자주 변경
};

class BindGroupLayoutCache {
    wgpu::BindGroupLayout frameLayout;
    wgpu::BindGroupLayout passLayout;
    wgpu::BindGroupLayout materialLayout;
    wgpu::BindGroupLayout objectLayout;
    
    void Initialize() {
        // Frame layout (Set 0)
        frameLayout = CreateLayout({
            {0, BufferBinding, ShaderStage::Vertex | Fragment},  // FrameData
            {1, BufferBinding, ShaderStage::Fragment},           // Lights
            {2, TextureBinding, ShaderStage::Fragment},          // Shadow maps
            {3, SamplerBinding, ShaderStage::Fragment},          // Shadow sampler
            {4, TextureBinding, ShaderStage::Fragment},          // IBL cube
            {5, TextureBinding, ShaderStage::Fragment}           // BRDF LUT
        });
        
        // Material layout (Set 2)  
        materialLayout = CreateLayout({
            {0, BufferBinding, ShaderStage::Fragment},   // MaterialData
            {1, TextureBinding, ShaderStage::Fragment},  // Base color
            {2, TextureBinding, ShaderStage::Fragment},  // Normal
            {3, TextureBinding, ShaderStage::Fragment},  // Metal-rough
            {4, TextureBinding, ShaderStage::Fragment},  // Occlusion
            {5, TextureBinding, ShaderStage::Fragment},  // Emissive
            {6, SamplerBinding, ShaderStage::Fragment}   // Sampler
        });
    }
};
```

## 최적화 전략

### 1. 동적 오프셋 (Dynamic Offsets)
```cpp
// Per-Object 데이터를 하나의 큰 버퍼로 관리
class ObjectDataBuffer {
    Buffer* buffer;
    size_t stride = 256;  // 256 byte alignment for dynamic offset
    
    void BindForObject(CommandRecorder* cmd, uint32_t objectIndex) {
        uint32_t offset = objectIndex * stride;
        cmd->SetBindGroup(3, objectBindGroup, {offset});
    }
};
```

### 2. 바인드리스 텍스처 (Bindless Textures)
```cpp
// 모든 텍스처를 배열로 관리 (GPU가 지원하는 경우)
struct BindlessMaterialData {
    uint32_t baseColorIndex;
    uint32_t normalIndex;
    uint32_t metallicRoughnessIndex;
    uint32_t occlusionIndex;
    uint32_t emissiveIndex;
    // ... material parameters ...
};

// Shader
texture2d_array<float> allTextures;
float4 baseColor = allTextures.Sample(sampler, float3(uv, material.baseColorIndex));
```

### 3. 인스턴싱 최적화
```cpp
// 같은 메쉬와 머티리얼을 공유하는 오브젝트들을 인스턴싱
struct InstanceData {
    mat4 modelMatrix;
    vec4 instanceColor;
    uint32_t objectID;
};

// Vertex buffer로 인스턴스 데이터 전달
cmd->SetVertexBuffer(0, meshVertices);
cmd->SetVertexBuffer(1, instanceBuffer);  // Instance data
cmd->DrawIndexedInstanced(indexCount, instanceCount);
```

## 사용 예시

### 렌더링 루프에서의 바인딩
```cpp
void RenderFrame(CommandRecorder* cmd) {
    // Set 0: Frame - 프레임당 1회
    cmd->SetBindGroup(0, frameBindGroup);
    
    for (auto& pass : renderPasses) {
        cmd->BeginRenderPass(pass.target);
        
        // Set 1: Pass - 패스당 1회
        cmd->SetBindGroup(1, pass.bindGroup);
        
        for (auto& batch : pass.batches) {
            // Set 2: Material - 머티리얼당 1회
            cmd->SetBindGroup(2, batch.material->GetBindGroup());
            
            for (auto& object : batch.objects) {
                // Set 3: Object - 오브젝트당 1회
                cmd->SetBindGroup(3, object.bindGroup);
                cmd->DrawIndexed(object.indexCount);
            }
        }
        
        cmd->EndRenderPass();
    }
}
```

### 셰이더에서의 접근
```wgsl
// Set 0: Frame
@group(0) @binding(0) var<uniform> frame: FrameData;
@group(0) @binding(1) var<storage> lights: array<Light>;
@group(0) @binding(2) var shadowMaps: texture_depth_2d_array;

// Set 1: Pass  
@group(1) @binding(0) var<uniform> pass: PassData;

// Set 2: Material
@group(2) @binding(0) var<uniform> material: MaterialData;
@group(2) @binding(1) var baseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var normalTexture: texture_2d<f32>;

// Set 3: Object
@group(3) @binding(0) var<uniform> object: ObjectData;

@vertex
fn vs_main(@location(0) position: vec3<f32>) -> VertexOutput {
    // Transform: Object -> World -> View -> Clip
    let worldPos = object.modelMatrix * vec4(position, 1.0);
    let clipPos = frame.viewProjMatrix * worldPos;
    return VertexOutput(clipPos, worldPos.xyz, ...);
}
```

## 메모리 레이아웃 고려사항

### 정렬 요구사항
```cpp
// WebGPU/Vulkan alignment requirements
struct alignas(16) Vec3 {
    float x, y, z;
    float _padding;  // vec3는 16 byte 정렬 필요
};

struct alignas(256) DynamicUniformData {
    // Dynamic offset은 256 byte 정렬 필요 (minUniformBufferOffsetAlignment)
    mat4 transform;
    // ... padding to 256 bytes ...
};
```

### 버퍼 패킹
```cpp
// 효율적인 메모리 사용을 위한 패킹
struct PackedMaterialData {
    vec4 baseColorFactor;          // 16 bytes
    vec2 metallicRoughness;         // 8 bytes  
    vec2 normalOcclusion;           // 8 bytes
    vec4 emissiveFactorStrength;   // 16 bytes
    vec4 uvScaleOffset;             // 16 bytes
    uint32_t flags[4];              // 16 bytes
    // Total: 80 bytes (5 vec4)
};
```

## 성능 고려사항

### 바인딩 변경 최소화
1. **드로우 콜 정렬**: Pipeline → Material → Object 순으로 정렬
2. **배칭**: 같은 머티리얼을 사용하는 오브젝트 그룹화
3. **인스턴싱**: 동일 메쉬의 복수 인스턴스 한번에 그리기

### 리소스 재사용
1. **바인드 그룹 캐싱**: 자주 사용되는 조합 캐싱
2. **동적 버퍼**: 작은 uniform 데이터는 동적 오프셋 사용
3. **텍스처 아틀라스**: 작은 텍스처들을 하나로 합치기

## 요약

이 리소스 바인딩 모델은 GPU의 descriptor set 모델을 효율적으로 활용하면서, 렌더링 성능을 최적화합니다. 

**핵심 원칙**:
- 업데이트 빈도별 계층화 (Frame → Pass → Material → Object)
- 낮은 인덱스 = 적은 변경, 높은 인덱스 = 자주 변경
- 동적 오프셋과 인스턴싱으로 바인딩 변경 최소화
- 플랫폼별 정렬 요구사항 준수