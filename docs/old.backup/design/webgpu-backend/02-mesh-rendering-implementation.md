# Mesh Rendering Implementation with WebGPU Backend

## Overview
WebGPU에서 메쉬를 렌더링하는 전체 과정을 구현 관점에서 설명합니다. 초기화부터 프레임 렌더링까지 모든 시퀀스를 다룹니다.

## 1. Application Layer Usage Pattern

```cpp
// Application 코드 예시
class MeshRenderApp : public Application {
    void Initialize() {
        // 1. Graphics Device 초기화
        _device = CreateWebGPUDevice();
        
        // 2. 메쉬 데이터 로드
        _mesh = LoadMesh("model.gltf");
        
        // 3. 셰이더 로드
        _shader = _device->LoadShader("mesh.wgsl");
        
        // 4. 파이프라인 생성
        _pipeline = _device->CreateRenderPipeline(pipelineDesc);
        
        // 5. 리소스 바인딩 설정
        _bindGroup = _device->CreateBindGroup(bindGroupDesc);
    }
    
    void Render() {
        // 1. 프레임 시작
        auto commandEncoder = _device->BeginFrame();
        
        // 2. 렌더 패스 설정
        auto renderPass = commandEncoder->BeginRenderPass(passDesc);
        
        // 3. 파이프라인 바인딩
        renderPass->SetPipeline(_pipeline);
        
        // 4. 리소스 바인딩
        renderPass->SetBindGroup(0, _frameData);
        renderPass->SetBindGroup(1, _materialData);
        renderPass->SetBindGroup(2, _objectData);
        
        // 5. 메쉬 그리기
        renderPass->SetVertexBuffer(_mesh->vertexBuffer);
        renderPass->SetIndexBuffer(_mesh->indexBuffer);
        renderPass->DrawIndexed(_mesh->indexCount);
        
        // 6. 패스 종료 및 제출
        renderPass->End();
        _device->Submit(commandEncoder->Finish());
        _device->Present();
    }
};
```

## 2. Initialization Sequence

### 2.1 Device Creation
```
1. Create Instance
   └─> Enumerate Adapters
       └─> Select Best Adapter (Discrete GPU > Integrated)
           └─> Request Device
               ├─> Set Required Features
               ├─> Set Required Limits
               └─> Create Queue
```

### 2.2 SwapChain Setup
```
1. Create Surface (Platform Specific)
   └─> Configure SwapChain
       ├─> Format: BGRA8Unorm/RGBA8Unorm
       ├─> Present Mode: Mailbox/Fifo
       ├─> Buffer Count: 2-3
       └─> Usage: RenderAttachment
```

### 2.3 Resource Creation Flow
```
1. Mesh Loading
   ├─> Parse Geometry Data
   ├─> Create Vertex Buffer
   │   ├─> Calculate Size
   │   ├─> Set Usage: VERTEX | COPY_DST
   │   └─> Upload Data (Staging Buffer)
   └─> Create Index Buffer
       ├─> Calculate Size
       ├─> Set Usage: INDEX | COPY_DST
       └─> Upload Data (Staging Buffer)

2. Uniform Buffer Creation
   ├─> Frame Uniforms (View, Projection)
   │   └─> Dynamic Buffer (Ring Buffer)
   ├─> Material Uniforms
   │   └─> Static Buffer
   └─> Object Uniforms (Model Matrix)
       └─> Dynamic Buffer (Per-Instance)

3. Texture Loading
   ├─> Decode Image
   ├─> Generate Mipmaps
   ├─> Create Texture
   └─> Create Sampler
```

## 3. Pipeline State Object Creation

### 3.1 Shader Compilation
```
1. Load WGSL Source
2. Add Preprocessor Defines
3. Create Shader Module
4. Validate Entry Points
```

### 3.2 Pipeline Assembly
```
RenderPipelineDescriptor {
    // Vertex Stage
    vertex: {
        module: vertexShader,
        entryPoint: "vs_main",
        buffers: [
            {
                arrayStride: sizeof(Vertex),
                attributes: [
                    { location: 0, offset: 0,  format: Float32x3 },  // position
                    { location: 1, offset: 12, format: Float32x3 },  // normal
                    { location: 2, offset: 24, format: Float32x2 }   // texcoord
                ]
            }
        ]
    },
    
    // Fragment Stage
    fragment: {
        module: fragmentShader,
        entryPoint: "fs_main",
        targets: [{
            format: SwapChainFormat,
            blend: {
                color: { src: SrcAlpha, dst: OneMinusSrcAlpha },
                alpha: { src: One, dst: One }
            }
        }]
    },
    
    // Pipeline Layout
    layout: {
        bindGroupLayouts: [
            frameBindGroupLayout,    // Set 0
            materialBindGroupLayout,  // Set 1
            objectBindGroupLayout     // Set 2
        ]
    },
    
    // Fixed Function States
    primitive: { topology: TriangleList, cullMode: Back },
    depthStencil: {
        format: Depth24Plus,
        depthWriteEnabled: true,
        depthCompare: Less
    },
    multisample: { count: 1 }
}
```

## 4. Frame Rendering Sequence

### 4.1 Frame Preparation
```
1. Wait for Previous Frame
   └─> Fence/Semaphore Wait

2. Acquire SwapChain Image
   └─> Get Next Available Image Index

3. Update Dynamic Resources
   ├─> Update Frame Uniforms (Camera)
   ├─> Update Object Transforms
   └─> Update Animation Data
```

### 4.2 Command Recording
```
1. Create Command Encoder
   
2. Copy Operations (if needed)
   └─> Update Buffers/Textures
   
3. Begin Render Pass
   ├─> Set Color Attachments
   ├─> Set Depth Attachment
   └─> Set Clear Values

4. For Each Draw Call:
   ├─> Set Pipeline State
   ├─> Set Bind Groups
   │   ├─> Set 0: Frame Data (View/Proj)
   │   ├─> Set 1: Material Data (Textures)
   │   └─> Set 2: Object Data (Model Matrix)
   ├─> Set Vertex/Index Buffers
   └─> Draw Indexed

5. End Render Pass

6. Finish Command Buffer
```

### 4.3 Submission and Presentation
```
1. Submit to Queue
   ├─> Command Buffers[]
   ├─> Signal Semaphores[]
   └─> Wait Semaphores[]

2. Present
   └─> SwapChain Present
```

## 5. Resource Binding Detail

### 5.1 Bind Group Creation
```cpp
// Frame Data (Set 0) - Updated once per frame
BindGroupDescriptor frameBindGroup {
    layout: frameLayout,
    entries: [
        { binding: 0, resource: viewProjBuffer },
        { binding: 1, resource: lightsBuffer },
        { binding: 2, resource: shadowMapTexture },
        { binding: 3, resource: shadowMapSampler }
    ]
}

// Material Data (Set 1) - Changed per material
BindGroupDescriptor materialBindGroup {
    layout: materialLayout,
    entries: [
        { binding: 0, resource: materialPropsBuffer },
        { binding: 1, resource: diffuseTexture },
        { binding: 2, resource: normalTexture },
        { binding: 3, resource: textureSampler }
    ]
}

// Object Data (Set 2) - Changed per object
BindGroupDescriptor objectBindGroup {
    layout: objectLayout,
    entries: [
        { binding: 0, resource: modelMatrixBuffer },
        { binding: 1, resource: objectIDBuffer }
    ]
}
```

### 5.2 Dynamic Buffer Update Pattern
```cpp
// Ring Buffer for Dynamic Updates
class DynamicUniformBuffer {
    wgpu::Buffer buffer;
    size_t alignment;
    size_t currentOffset;
    
    void* BeginUpdate() {
        // Map buffer region
        void* data = buffer.MapWrite(currentOffset, size);
        return data;
    }
    
    void EndUpdate() {
        buffer.Unmap();
        currentOffset = Align(currentOffset + size, alignment);
        if (currentOffset >= totalSize) {
            currentOffset = 0; // Wrap around
        }
    }
}
```

## 6. Optimization Strategies

### 6.1 Batching
- Minimize state changes
- Sort by Pipeline > Material > Mesh
- Instance rendering for repeated objects

### 6.2 Resource Management
- Buffer sub-allocation
- Texture atlasing
- Bindless textures (if supported)

### 6.3 Multi-threading
- Parallel command buffer recording
- Async resource loading
- GPU-based culling

## 7. Error Handling

### 7.1 Validation
```cpp
// Development mode validation
if (DEBUG) {
    device.SetUncapturedErrorCallback([](ErrorType type, const char* msg) {
        LOG_ERROR("WebGPU Error: {}", msg);
    });
}
```

### 7.2 Device Lost Recovery
```cpp
device.SetDeviceLostCallback([this]() {
    // 1. Release all resources
    // 2. Recreate device
    // 3. Reload resources
    RecreateDevice();
});
```

## 8. Performance Considerations

### 8.1 GPU Timing
```cpp
// Query timestamp for profiling
commandEncoder.WriteTimestamp(querySet, 0);
// ... rendering commands ...
commandEncoder.WriteTimestamp(querySet, 1);
```

### 8.2 Memory Management
- Use appropriate buffer usage flags
- Minimize CPU-GPU transfers
- Pool frequently created resources