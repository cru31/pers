# Forward Rendering Pipeline

## Overview
Forward rendering은 각 오브젝트를 그릴 때 모든 라이팅 계산을 수행하는 전통적인 렌더링 방식입니다.

## Pipeline Architecture

### 1. Render Passes

#### 1.1 Z-Prepass (Optional)
- **목적**: Early-Z rejection으로 overdraw 감소
- **출력**: Depth buffer only
- **셰이더**: Vertex shader only (no fragment shader)

#### 1.2 Shadow Pass (Per Light)
- **목적**: Shadow map 생성
- **출력**: Depth texture array
- **반복**: 각 shadow-casting light마다

#### 1.3 Main Forward Pass
- **목적**: 최종 색상 렌더링
- **출력**: Color buffer + Depth buffer
- **특징**: 모든 라이팅 계산 수행

#### 1.4 Transparency Pass
- **목적**: 반투명 오브젝트 렌더링
- **출력**: Color buffer (blending enabled)
- **정렬**: Back-to-front

#### 1.5 Post-processing Pass
- **목적**: Tone mapping, AA, bloom 등
- **출력**: Final SwapChain image

## Resource Requirements

### 1. Buffers

#### Per-Frame Resources (Set 0)
```cpp
struct FrameData {
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 viewProjMatrix;
    mat4 invViewProjMatrix;
    vec3 cameraPosition;
    float time;
    vec3 cameraDirection;
    float deltaTime;
    // Padding to 256 bytes
};

struct LightData {
    vec3 position;
    float range;
    vec3 color;
    float intensity;
    vec3 direction;  // For spot/directional
    float innerCone; // For spot
    float outerCone; // For spot
    int type;        // 0=point, 1=spot, 2=directional
    mat4 lightViewProj; // For shadows
};

struct FrameLights {
    LightData lights[MAX_LIGHTS];  // e.g., 128
    int numLights;
    vec3 ambientLight;
};
```

**Buffer Specifications:**
- `frameDataBuffer`: 256 bytes, UNIFORM, dynamic
- `lightsBuffer`: 128 * sizeof(LightData), STORAGE, dynamic
- Update frequency: Once per frame

#### Per-Pass Resources (Set 1)
```cpp
struct PassData {
    vec4 renderTargetSize; // width, height, 1/width, 1/height
    int passType;          // 0=depth, 1=color, 2=shadow
    int cascadeIndex;      // For cascaded shadow maps
    float nearPlane;
    float farPlane;
};
```

**Buffer Specifications:**
- `passDataBuffer`: 64 bytes, UNIFORM, dynamic
- Update frequency: Once per pass

#### Per-Material Resources (Set 2)
```cpp
struct MaterialData {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float normalScale;
    vec4 emissive;
    int alphaMode;     // 0=opaque, 1=blend, 2=mask
    float alphaCutoff;
    int doubleSided;
    int shadingModel;  // 0=PBR, 1=Phong, 2=Unlit
};
```

**Buffer Specifications:**
- `materialBuffer`: 96 bytes, UNIFORM, static
- Update frequency: Per material change

**Textures:**
- `baseColorTexture`: RGBA8, 2D, sRGB
- `normalTexture`: RGBA8, 2D, linear
- `metallicRoughnessTexture`: RG8, 2D, linear
- `aoTexture`: R8, 2D, linear
- `emissiveTexture`: RGBA8, 2D, sRGB

#### Per-Object Resources (Set 3)
```cpp
struct ObjectData {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 objectColor;  // Per-instance tint
    int objectID;
    int boneOffset;    // For skinning
    int morphWeights;  // For morph targets
    int customData;
};
```

**Buffer Specifications:**
- `objectBuffer`: 160 bytes per object, STORAGE, dynamic
- Update frequency: Every draw call

### 2. Render Targets

#### Main Framebuffer
- **Color**: RGBA16F or RGBA8 (HDR vs LDR)
- **Depth**: Depth24Plus-Stencil8
- **Size**: Screen resolution

#### Shadow Maps
- **Format**: Depth32F or Depth16
- **Size**: 2048x2048 or 4096x4096
- **Type**: Texture2DArray for multiple lights

#### Intermediate Buffers (Optional)
- **Motion Vectors**: RG16F for temporal effects
- **Object ID**: R32I for selection

### 3. Pipeline State Objects

#### Z-Prepass PSO
```cpp
RenderPipelineDescriptor {
    vertex: {
        module: zPrepassVS,
        entryPoint: "main",
        buffers: [vertexLayout]
    },
    fragment: null,  // No fragment shader
    primitive: {
        topology: TriangleList,
        cullMode: Back
    },
    depthStencil: {
        format: Depth24Plus,
        depthWriteEnabled: true,
        depthCompare: Less
    },
    colorTargets: []  // No color output
}
```

#### Main Forward PSO
```cpp
RenderPipelineDescriptor {
    vertex: {
        module: forwardVS,
        entryPoint: "main",
        buffers: [vertexLayout]
    },
    fragment: {
        module: forwardFS,
        entryPoint: "main",
        targets: [{
            format: RGBA16F,
            blend: null  // No blending for opaque
        }]
    },
    primitive: {
        topology: TriangleList,
        cullMode: Back
    },
    depthStencil: {
        format: Depth24Plus,
        depthWriteEnabled: true,
        depthCompare: LessEqual  // Equal for Z-prepass
    },
    multisample: { count: 4 }  // Optional MSAA
}
```

#### Transparency PSO
```cpp
RenderPipelineDescriptor {
    // ... similar to main ...
    fragment: {
        targets: [{
            format: RGBA16F,
            blend: {
                color: {
                    srcFactor: SrcAlpha,
                    dstFactor: OneMinusSrcAlpha,
                    operation: Add
                },
                alpha: {
                    srcFactor: One,
                    dstFactor: OneMinusSrcAlpha,
                    operation: Add
                }
            }
        }]
    },
    depthStencil: {
        depthWriteEnabled: false,  // No depth write
        depthCompare: Less
    }
}
```

## Shader Requirements

### Vertex Shader Interface
```wgsl
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) tangent: vec4<f32>,
    @location(3) texcoord0: vec2<f32>,
    @location(4) texcoord1: vec2<f32>,
    @location(5) color: vec4<f32>,
    @location(6) joints: vec4<u32>,
    @location(7) weights: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) clipPosition: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) tangent: vec3<f32>,
    @location(3) bitangent: vec3<f32>,
    @location(4) texcoord0: vec2<f32>,
    @location(5) texcoord1: vec2<f32>,
    @location(6) color: vec4<f32>,
    @location(7) shadowCoords: array<vec4<f32>, 4>, // For cascaded shadows
};
```

### Fragment Shader Features
- Multi-light support (loop over lights array)
- PBR shading model
- Normal mapping
- Shadow mapping with PCF
- Optional: Parallax mapping, SSR

## Performance Optimizations

### 1. Batching Strategy
- Sort by: Pipeline → Material → Mesh
- Instance rendering for repeated objects
- Indirect drawing for GPU-driven rendering

### 2. Light Culling
- Tile-based light culling for many lights
- Distance-based LOD for shadows
- Cascaded shadow maps for directional lights

### 3. Early-out Optimizations
- Z-prepass for complex scenes
- Frustum culling on CPU
- GPU occlusion queries

## Memory Budget

### Typical Scene (1080p)
- Frame buffers: ~32 MB
- Shadow maps (4x 2048²): ~64 MB
- Uniform buffers: ~1 MB
- Vertex/Index buffers: ~100-500 MB
- Textures: ~500-2000 MB
- **Total**: ~1-3 GB

## Advantages
- Simple and straightforward
- Works well with transparency
- Good for scenes with few lights
- Hardware MSAA support

## Disadvantages
- Poor scaling with many lights
- Overdraw issues
- Repeated shading calculations
- Limited post-processing options