# Deferred Rendering Pipeline

## Overview
Deferred rendering은 geometry 정보를 먼저 G-buffer에 렌더링한 후, screen-space에서 라이팅을 계산하는 방식입니다. 많은 수의 라이트를 효율적으로 처리할 수 있습니다.

## Pipeline Architecture

### 1. Render Passes

#### 1.1 G-Buffer Pass
- **목적**: Geometry 정보를 Multiple Render Targets에 저장
- **출력**: G-Buffer (MRT)
- **특징**: 라이팅 계산 없이 material properties만 저장

#### 1.2 Shadow Pass
- **목적**: Shadow maps 생성
- **출력**: Shadow map array
- **실행**: G-Buffer pass 전 또는 후

#### 1.3 Lighting Pass
- **목적**: Screen-space에서 라이팅 계산
- **입력**: G-Buffer textures
- **출력**: HDR color buffer

#### 1.4 Forward Pass (Transparency)
- **목적**: 반투명 오브젝트 렌더링
- **특징**: Deferred로 처리 불가능한 오브젝트

#### 1.5 Post-processing Pass
- **목적**: HDR to LDR, Anti-aliasing, Effects
- **출력**: Final SwapChain image

## G-Buffer Layout

### Standard G-Buffer (128-bit)
```cpp
struct GBufferData {
    // RT0: Albedo + Metallic (RGBA8)
    vec3 albedo;      // RGB channels
    float metallic;   // A channel
    
    // RT1: Normal + Roughness (RGBA8)
    vec2 normal;      // RG channels (encoded)
    float roughness;  // B channel
    float ao;         // A channel
    
    // RT2: Motion + ObjectID (RG16F + RG16I)
    vec2 motion;      // Screen space motion vectors
    uint objectID;    // Object identifier
    uint materialID;  // Material type
    
    // RT3: Emissive + Custom (RGBA16F) - Optional
    vec3 emissive;    // RGB channels
    float custom;     // A channel (e.g., subsurface)
    
    // Depth: Depth24Plus-Stencil8
    float depth;
    uint stencil;
};
```

### Compact G-Buffer (64-bit)
```cpp
struct CompactGBufferData {
    // RT0: Albedo + AO (RGBA8)
    vec3 albedo;
    float ao;
    
    // RT1: Normal + Material (RG16F + RG8)
    vec2 normal;      // Encoded world normal
    float roughness;
    float metallic;
    
    // Depth buffer
    float depth;
};
```

## Resource Requirements

### 1. G-Buffer Resources

#### Render Target Specifications
```cpp
// Full G-Buffer Setup (1920x1080)
GBufferTargets {
    RT0: {
        format: RGBA8Unorm_sRGB,
        usage: RenderAttachment | TextureBinding,
        size: 1920x1080,
        memory: 8.3 MB
    },
    RT1: {
        format: RGBA8Unorm,
        usage: RenderAttachment | TextureBinding,
        size: 1920x1080,
        memory: 8.3 MB
    },
    RT2: {
        format: RGBA16Float,
        usage: RenderAttachment | TextureBinding,
        size: 1920x1080,
        memory: 16.6 MB
    },
    RT3: {
        format: RGBA16Float,  // Optional
        usage: RenderAttachment | TextureBinding,
        size: 1920x1080,
        memory: 16.6 MB
    },
    Depth: {
        format: Depth24Plus_Stencil8,
        usage: RenderAttachment | TextureBinding,
        size: 1920x1080,
        memory: 8.3 MB
    }
    // Total: ~58 MB for full G-Buffer
}
```

### 2. Lighting Resources

#### Light Volume Geometry
```cpp
// For tiled deferred
struct TileData {
    uint lightCount;
    uint lightIndices[MAX_LIGHTS_PER_TILE];  // e.g., 32
};

TiledLightingResources {
    tileBuffer: {
        size: (screenWidth/TILE_SIZE) * (screenHeight/TILE_SIZE) * sizeof(TileData),
        usage: STORAGE,
        // For 1920x1080 with 16x16 tiles: 120*68*132 bytes = ~1 MB
    },
    lightIndexBuffer: {
        size: MAX_TOTAL_LIGHT_INDICES * sizeof(uint),
        usage: STORAGE
    }
}
```

#### Screen-Space Textures
```cpp
SSAOResources {
    noiseTexture: {
        format: RG16Float,
        size: 4x4,
        usage: TextureBinding
    },
    ssaoBuffer: {
        format: R8Unorm,
        size: screenSize / 2,  // Half resolution
        usage: RenderAttachment | TextureBinding
    }
}
```

### 3. Pipeline State Objects

#### G-Buffer Generation PSO
```cpp
GBufferPSO {
    vertex: {
        module: gBufferVS,
        entryPoint: "main",
        buffers: [vertexLayout]
    },
    fragment: {
        module: gBufferFS,
        entryPoint: "main",
        targets: [
            { format: RGBA8Unorm_sRGB },    // Albedo
            { format: RGBA8Unorm },          // Normal + Material
            { format: RGBA16Float },         // Motion + ID
            { format: RGBA16Float }          // Emissive (optional)
        ]
    },
    primitive: {
        topology: TriangleList,
        cullMode: Back
    },
    depthStencil: {
        format: Depth24Plus_Stencil8,
        depthWriteEnabled: true,
        depthCompare: Less
    }
}
```

#### Lighting Pass PSO
```cpp
LightingPSO {
    vertex: {
        module: fullscreenVS,  // Simple fullscreen triangle
        entryPoint: "main"
    },
    fragment: {
        module: deferredLightingFS,
        entryPoint: "main",
        targets: [{
            format: RGBA16Float,  // HDR output
            blend: {
                // Additive blending for multiple light passes
                color: { src: One, dst: One, op: Add }
            }
        }]
    },
    primitive: {
        topology: TriangleList,
        cullMode: None
    },
    depthStencil: null  // No depth test
}
```

#### Light Volume PSO (for individual lights)
```cpp
LightVolumePSO {
    vertex: {
        module: lightVolumeVS,
        entryPoint: "main",
        buffers: [sphereVertexLayout]  // For point lights
    },
    fragment: {
        module: lightVolumeFS,
        entryPoint: "main",
        targets: [{
            format: RGBA16Float,
            blend: { /* Additive */ }
        }]
    },
    primitive: {
        topology: TriangleList,
        cullMode: Front,  // Render back faces
        frontFace: CCW
    },
    depthStencil: {
        format: Depth24Plus_Stencil8,
        depthWriteEnabled: false,
        depthCompare: GreaterEqual,  // Reverse depth test
        stencilFront: {
            compare: NotEqual,
            failOp: Keep,
            depthFailOp: IncrementWrap,
            passOp: Keep
        }
    }
}
```

## Shader Interfaces

### G-Buffer Generation Shaders
```wgsl
// Vertex Output for G-Buffer
struct GBufferVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) tangent: vec3<f32>,
    @location(3) texcoord: vec2<f32>,
    @location(4) prevClipPos: vec4<f32>,  // For motion vectors
    @location(5) currClipPos: vec4<f32>,
};

// Fragment Output (MRT)
struct GBufferOutput {
    @location(0) albedo: vec4<f32>,     // Albedo + Metallic
    @location(1) normal: vec4<f32>,     // Normal + Roughness
    @location(2) motion: vec4<f32>,     // Motion + IDs
    @location(3) emissive: vec4<f32>,   // Emissive + Custom
};
```

### Deferred Lighting Shader
```wgsl
@group(0) @binding(0) var gBufferAlbedo: texture_2d<f32>;
@group(0) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(0) @binding(2) var gBufferMotion: texture_2d<f32>;
@group(0) @binding(3) var gBufferDepth: texture_depth_2d;
@group(0) @binding(4) var linearSampler: sampler;

@group(1) @binding(0) var<storage, read> lights: array<Light>;
@group(1) @binding(1) var<uniform> lightingParams: LightingParams;

fn reconstructWorldPos(uv: vec2<f32>, depth: f32) -> vec3<f32> {
    let clipPos = vec4<f32>(uv * 2.0 - 1.0, depth, 1.0);
    let worldPos = invViewProj * clipPos;
    return worldPos.xyz / worldPos.w;
}

@fragment
fn deferredLighting(@builtin(position) fragCoord: vec4<f32>) -> @location(0) vec4<f32> {
    let uv = fragCoord.xy / screenSize;
    
    // Sample G-Buffer
    let albedo = textureSample(gBufferAlbedo, linearSampler, uv).rgb;
    let normalRoughness = textureSample(gBufferNormal, linearSampler, uv);
    let depth = textureSample(gBufferDepth, linearSampler, uv);
    
    // Reconstruct world position
    let worldPos = reconstructWorldPos(uv, depth);
    let normal = decodeNormal(normalRoughness.xy);
    let roughness = normalRoughness.z;
    let metallic = normalRoughness.w;
    
    // Accumulate lighting
    var color = vec3<f32>(0.0);
    for (var i = 0u; i < arrayLength(&lights); i++) {
        color += calculateLight(lights[i], worldPos, normal, albedo, roughness, metallic);
    }
    
    return vec4<f32>(color, 1.0);
}
```

## Optimization Techniques

### 1. Tiled Deferred Rendering
- Divide screen into tiles (e.g., 16x16 pixels)
- Cull lights per tile in compute shader
- Process only relevant lights per pixel

### 2. Light Volumes
- Render light volumes (spheres/cones) for large lights
- Use stencil to mask affected pixels
- Reduces overdraw significantly

### 3. G-Buffer Compression
- Pack normals using octahedral encoding (2 components)
- Store material IDs instead of properties
- Use lower precision where possible

### 4. Temporal Techniques
- Temporal Anti-Aliasing (TAA)
- Screen-Space Reflections (SSR)
- Temporal upsampling

## Memory Budget

### 1920x1080 Resolution
- G-Buffer (4 RT): ~58 MB
- HDR Light Accumulation: ~16 MB
- Shadow Maps (4x 2048²): ~64 MB
- SSAO Buffer: ~2 MB
- Tiled Light Data: ~1 MB
- **Total**: ~141 MB

### 4K Resolution (3840x2160)
- G-Buffer (4 RT): ~232 MB
- HDR Light Accumulation: ~64 MB
- Shadow Maps (4x 4096²): ~256 MB
- SSAO Buffer: ~8 MB
- Tiled Light Data: ~4 MB
- **Total**: ~564 MB

## Advantages
- Excellent for many lights (100+)
- Consistent shading cost
- Good memory bandwidth utilization
- Natural SSAO/SSR integration
- Reduced overdraw

## Disadvantages
- No transparency support (requires forward pass)
- No MSAA (must use post-process AA)
- High memory bandwidth for G-Buffer
- Limited material variety
- Difficulty with volumetric effects