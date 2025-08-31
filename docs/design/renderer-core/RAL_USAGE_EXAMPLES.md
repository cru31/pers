# Pers Graphics Engine V2 - RAL Usage Examples

## RAL (Rendering Abstraction Layer)

The RAL provides direct, low-level access to GPU resources with clear command recording semantics.

---

## Basic Triangle Rendering

```cpp
#include <pers/graphics/IInstance.h>
#include <pers/graphics/ILogicalDevice.h>
#include <pers/graphics/IResourceFactory.h>

void RenderTriangle(Application& app) {
    // Initialize graphics instance
    auto instance = CreateInstance(InstanceDesc{
        .backend = GraphicsBackend::WebGPU
    });
    
    // Request physical device and create logical device
    auto physicalDevice = instance->RequestPhysicalDevice(PhysicalDeviceOptions{
        .powerPreference = PowerPreference::HighPerformance
    });
    auto device = physicalDevice->CreateLogicalDevice(LogicalDeviceDesc{});
    
    // Get core components
    auto queue = device->GetQueue();
    auto factory = device->GetResourceFactory();
    
    // Create surface and swap chain
    auto surface = instance->CreateSurface(app.GetWindowHandle());
    auto swapChain = device->CreateSwapChain(surface, SwapChainDesc{
        .format = TextureFormat::BGRA8Unorm,
        .presentMode = PresentMode::Fifo
    });
    
    // Create vertex buffer
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // Bottom-left, red
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // Bottom-right, green
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f   // Top, blue
    };
    
    auto vertexBuffer = factory->CreateVertexBuffer(VertexBufferDesc{
        .size = sizeof(vertices),
        .data = vertices,
        .stride = sizeof(float) * 6,
        .layout = VertexLayout{
            {VertexAttribute::Position, VertexFormat::Float3},
            {VertexAttribute::Color, VertexFormat::Float3}
        }
    });
    
    // Create shaders
    auto vertexShader = factory->CreateShaderModule(ShaderModuleDesc{
        .stage = ShaderStage::Vertex,
        .source = vertexShaderSource,
        .entryPoint = "vs_main"
    });
    
    auto fragmentShader = factory->CreateShaderModule(ShaderModuleDesc{
        .stage = ShaderStage::Fragment,
        .source = fragmentShaderSource,
        .entryPoint = "fs_main"
    });
    
    // Create graphics pipeline
    auto pipeline = factory->CreateGraphicsPipeline(GraphicsPipelineDesc{
        .vertexShader = vertexShader,
        .fragmentShader = fragmentShader,
        .vertexLayout = vertexBuffer->GetLayout(),
        .primitiveTopology = PrimitiveTopology::TriangleList,
        .rasterization = RasterizationState{
            .cullMode = CullMode::None
        },
        .colorTargets = {{
            .format = swapChain->GetFormat()
        }}
    });
    
    // Main render loop
    while (app.IsRunning()) {
        // Begin command recording
        auto encoder = device->CreateCommandEncoder();
        
        // Get current backbuffer
        auto backbuffer = swapChain->getCurrentTextureView();
        
        // Begin render pass
        auto renderPass = encoder->beginRenderPass(RenderPassDesc{
            .colorAttachments = {{
                .view = backbuffer,
                .clearValue = {0.1f, 0.2f, 0.3f, 1.0f},
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store
            }}
        });
        
        // Record draw commands
        renderPass->cmdSetGraphicsPipeline(pipeline);
        renderPass->cmdSetVertexBuffer(0, vertexBuffer);
        renderPass->cmdDraw(3);
        
        // End render pass
        renderPass->end();
        
        // Finish command recording
        auto commands = encoder->finish();
        
        // Submit to GPU
        queue->submit(commands);
        
        // Present frame
        swapChain->present();
    }
}
```

## Textured Quad with Uniforms

```cpp
void RenderTexturedQuad() {
    // ... device initialization ...
    
    // Create vertex and index buffers
    auto vertexBuffer = factory->CreateVertexBuffer(quadVertices);
    auto indexBuffer = factory->CreateIndexBuffer(quadIndices);
    
    // Create texture
    auto texture = factory->CreateTexture2D(Texture2DDesc{
        .width = 256,
        .height = 256,
        .format = TextureFormat::RGBA8Unorm,
        .usage = TextureUsage::Sampled | TextureUsage::CopyDst,
        .data = textureData
    });
    
    // Create sampler
    auto sampler = factory->CreateSampler(SamplerDesc{
        .minFilter = FilterMode::Linear,
        .magFilter = FilterMode::Linear,
        .addressModeU = AddressMode::Repeat,
        .addressModeV = AddressMode::Repeat
    });
    
    // Create uniform buffer
    struct Uniforms {
        glm::mat4 mvpMatrix;
        float time;
    };
    
    auto uniformBuffer = factory->CreateUniformBuffer(UniformBufferDesc{
        .size = sizeof(Uniforms),
        .isDynamic = true
    });
    
    // Create bind group layout
    auto bindGroupLayout = factory->CreateBindGroupLayout(BindGroupLayoutDesc{
        .entries = {
            {0, BindingType::UniformBuffer, ShaderStage::Vertex},
            {1, BindingType::Texture2D, ShaderStage::Fragment},
            {2, BindingType::Sampler, ShaderStage::Fragment}
        }
    });
    
    // Create bind group
    auto bindGroup = factory->CreateBindGroup(BindGroupDesc{
        .layout = bindGroupLayout,
        .entries = {
            {0, uniformBuffer},
            {1, texture},
            {2, sampler}
        }
    });
    
    // Create pipeline with bind group layout
    auto pipeline = factory->CreateGraphicsPipeline(GraphicsPipelineDesc{
        .layout = factory->CreatePipelineLayout(PipelineLayoutDesc{
            .bindGroupLayouts = {bindGroupLayout}
        }),
        // ... other pipeline settings ...
    });
    
    // Render loop
    while (running) {
        // Update uniforms
        Uniforms uniforms{
            .mvpMatrix = CalculateMVP(),
            .time = GetTime()
        };
        uniformBuffer->Update(&uniforms);
        
        auto encoder = device->CreateCommandEncoder();
        auto renderPass = encoder->beginRenderPass(renderPassDesc);
        
        // Bind everything and draw
        renderPass->cmdSetGraphicsPipeline(pipeline);
        renderPass->cmdSetVertexBuffer(0, vertexBuffer);
        renderPass->cmdSetIndexBuffer(indexBuffer);
        renderPass->cmdSetBindGroup(0, bindGroup);
        renderPass->cmdDrawIndexed(6);  // 2 triangles
        
        renderPass->end();
        
        auto commands = encoder->finish();
        queue->submit(commands);
        swapChain->present();
    }
}
```

## Multi-Pass Rendering (Shadow Mapping)

```cpp
void RenderWithShadows() {
    // Create shadow map render texture
    auto shadowMap = factory->CreateRenderTexture(RenderTextureDesc{
        .width = 2048,
        .height = 2048,
        .depthFormat = TextureFormat::Depth32Float
    });
    
    // Shadow pass pipeline (depth only)
    auto shadowPipeline = factory->CreateGraphicsPipeline(shadowPipelineDesc);
    
    // Main pass pipeline
    auto mainPipeline = factory->CreateGraphicsPipeline(mainPipelineDesc);
    
    // Per-frame uniform buffer
    auto frameUniforms = factory->CreateUniformBuffer(FrameUniformsDesc{});
    
    // Main render loop
    while (running) {
        auto encoder = device->CreateCommandEncoder();
        
        // === Shadow Pass ===
        auto shadowPass = encoder->beginRenderPass(RenderPassDesc{
            .depthStencilAttachment = {
                .view = shadowMap->GetDepthStencilTexture()->CreateView(),
                .depthClearValue = 1.0f,
                .depthLoadOp = LoadOp::Clear,
                .depthStoreOp = StoreOp::Store
            }
        });
        
        shadowPass->cmdSetGraphicsPipeline(shadowPipeline);
        shadowPass->cmdSetBindGroup(0, lightBindGroup);
        
        // Draw all shadow casters
        for (const auto& object : shadowCasters) {
            shadowPass->cmdSetBindGroup(3, object.bindGroup);
            shadowPass->cmdSetVertexBuffer(0, object.vertexBuffer);
            shadowPass->cmdDrawIndexed(object.indexCount);
        }
        
        shadowPass->end();
        
        // === Main Pass ===
        auto mainPass = encoder->beginRenderPass(RenderPassDesc{
            .colorAttachments = {{
                .view = swapChain->getCurrentTextureView(),
                .clearValue = {0.1f, 0.1f, 0.1f, 1.0f},
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store
            }},
            .depthStencilAttachment = {
                .view = depthBuffer->CreateView(),
                .depthClearValue = 1.0f,
                .depthLoadOp = LoadOp::Clear,
                .depthStoreOp = StoreOp::Store
            }
        });
        
        mainPass->cmdSetGraphicsPipeline(mainPipeline);
        mainPass->cmdSetBindGroup(0, frameBindGroup);  // Camera matrices
        mainPass->cmdSetBindGroup(1, shadowBindGroup);  // Shadow map
        
        // Draw all objects
        for (const auto& object : objects) {
            mainPass->cmdSetBindGroup(2, object.materialBindGroup);
            mainPass->cmdSetBindGroup(3, object.transformBindGroup);
            mainPass->cmdSetVertexBuffer(0, object.vertexBuffer);
            mainPass->cmdSetIndexBuffer(object.indexBuffer);
            mainPass->cmdDrawIndexed(object.indexCount);
        }
        
        mainPass->end();
        
        // Submit all passes
        auto commands = encoder->finish();
        queue->submit(commands);
        swapChain->present();
    }
}
```

## Compute Pass Example

```cpp
void ComputeParticleSimulation() {
    // Create storage buffers for particles
    auto particleBufferA = factory->CreateStorageBuffer(StorageBufferDesc{
        .size = sizeof(Particle) * PARTICLE_COUNT,
        .usage = StorageUsage::ReadWrite
    });
    
    auto particleBufferB = factory->CreateStorageBuffer(StorageBufferDesc{
        .size = sizeof(Particle) * PARTICLE_COUNT,
        .usage = StorageUsage::ReadWrite
    });
    
    // Create compute pipeline
    auto computePipeline = factory->CreateComputePipeline(ComputePipelineDesc{
        .computeShader = factory->CreateShaderModule(ShaderModuleDesc{
            .stage = ShaderStage::Compute,
            .source = particleUpdateSource,
            .entryPoint = "cs_main"
        }),
        .workgroupSize = {64, 1, 1}
    });
    
    // Create bind groups for ping-pong buffers
    auto bindGroupA = factory->CreateBindGroup(BindGroupDesc{
        .entries = {
            {0, particleBufferA},  // Read
            {1, particleBufferB}   // Write
        }
    });
    
    auto bindGroupB = factory->CreateBindGroup(BindGroupDesc{
        .entries = {
            {0, particleBufferB},  // Read
            {1, particleBufferA}   // Write
        }
    });
    
    bool pingPong = false;
    
    while (running) {
        auto encoder = device->CreateCommandEncoder();
        
        // Compute pass
        auto computePass = encoder->beginComputePass();
        
        computePass->cmdSetComputePipeline(computePipeline);
        computePass->cmdSetBindGroup(0, pingPong ? bindGroupB : bindGroupA);
        computePass->cmdDispatch(PARTICLE_COUNT / 64, 1, 1);
        
        computePass->end();
        
        // Render pass to visualize particles
        auto renderPass = encoder->beginRenderPass(renderPassDesc);
        
        renderPass->cmdSetGraphicsPipeline(particleRenderPipeline);
        renderPass->cmdSetStorageBuffer(0, pingPong ? particleBufferA : particleBufferB);
        renderPass->cmdDraw(PARTICLE_COUNT);
        
        renderPass->end();
        
        auto commands = encoder->finish();
        queue->submit(commands);
        swapChain->present();
        
        pingPong = !pingPong;
    }
}
```

## Resource Copy Operations

```cpp
void CopyOperations() {
    auto encoder = device->CreateCommandEncoder();
    
    // Buffer to buffer copy (outside any pass)
    encoder->cmdCopyBufferToBuffer(
        sourceBuffer, 0,
        destBuffer, 256,
        1024  // Copy 1KB from offset 0 to offset 256
    );
    
    // Buffer to texture copy
    encoder->cmdCopyBufferToTexture(
        stagingBuffer,
        texture,
        BufferTextureCopy{
            .bufferOffset = 0,
            .bufferBytesPerRow = 256 * 4,
            .textureOrigin = {0, 0, 0},
            .textureSize = {256, 256, 1}
        }
    );
    
    // Texture to texture copy (mipmap generation)
    for (uint32_t level = 1; level < mipLevels; ++level) {
        encoder->cmdCopyTextureToTexture(
            sourceTexture,
            destTexture,
            TextureTextureCopy{
                .sourceMipLevel = level - 1,
                .destMipLevel = level,
                .size = {width >> level, height >> level, 1}
            }
        );
    }
    
    auto commands = encoder->finish();
    queue->submit(commands);
}
```

## Indirect Drawing

```cpp
void IndirectDraw() {
    // Create indirect buffer with draw commands
    struct DrawIndirectCommand {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };
    
    DrawIndirectCommand commands[] = {
        {36, 10, 0, 0},   // 10 cubes
        {6, 100, 36, 10}, // 100 quads
    };
    
    auto indirectBuffer = factory->CreateIndirectBuffer(IndirectBufferDesc{
        .commandType = IndirectCommandType::Draw,
        .data = commands,
        .count = 2
    });
    
    auto encoder = device->CreateCommandEncoder();
    auto renderPass = encoder->beginRenderPass(renderPassDesc);
    
    renderPass->cmdSetGraphicsPipeline(pipeline);
    renderPass->cmdSetVertexBuffer(0, vertexBuffer);
    renderPass->cmdSetBindGroup(0, bindGroup);
    
    // Draw multiple objects with one command
    renderPass->cmdDrawIndirect(indirectBuffer, 0);  // First command
    renderPass->cmdDrawIndirect(indirectBuffer, sizeof(DrawIndirectCommand)); // Second
    
    renderPass->end();
    
    auto commands = encoder->finish();
    queue->submit(commands);
}
```

## Dynamic Buffer Updates

```cpp
void DynamicUniformBuffers() {
    // Create dynamic uniform buffer with space for all objects
    const size_t ALIGNMENT = 256;  // Min dynamic offset alignment
    const size_t OBJECT_COUNT = 100;
    
    auto dynamicUBO = factory->CreateUniformBuffer(UniformBufferDesc{
        .size = ALIGNMENT * OBJECT_COUNT,
        .isDynamic = true
    });
    
    // Update all object transforms
    for (size_t i = 0; i < OBJECT_COUNT; ++i) {
        ObjectUniforms uniforms{
            .modelMatrix = CalculateModelMatrix(i),
            .normalMatrix = CalculateNormalMatrix(i)
        };
        dynamicUBO->Update(&uniforms, i * ALIGNMENT, sizeof(uniforms));
    }
    
    auto encoder = device->CreateCommandEncoder();
    auto renderPass = encoder->beginRenderPass(renderPassDesc);
    
    renderPass->cmdSetGraphicsPipeline(pipeline);
    
    // Draw all objects with dynamic offsets
    for (size_t i = 0; i < OBJECT_COUNT; ++i) {
        uint32_t dynamicOffset = i * ALIGNMENT;
        renderPass->cmdSetBindGroup(3, objectBindGroup, {dynamicOffset});
        renderPass->cmdSetVertexBuffer(0, objects[i].vertexBuffer);
        renderPass->cmdDrawIndexed(objects[i].indexCount);
    }
    
    renderPass->end();
    
    auto commands = encoder->finish();
    queue->submit(commands);
}
```

## Error Handling

```cpp
void RobustRendering() {
    try {
        auto encoder = device->CreateCommandEncoder();
        
        // Check encoder state
        if (encoder->isRecording()) {
            throw std::runtime_error("Encoder already recording");
        }
        
        auto renderPass = encoder->beginRenderPass(renderPassDesc);
        
        // Check pass state
        if (!renderPass->isActive()) {
            throw std::runtime_error("RenderPass failed to begin");
        }
        
        // Record commands...
        renderPass->cmdSetGraphicsPipeline(pipeline);
        renderPass->cmdDraw(vertexCount);
        
        // Must end pass before finishing encoder
        renderPass->end();
        
        if (renderPass->isActive()) {
            throw std::runtime_error("RenderPass failed to end");
        }
        
        auto commands = encoder->finish();
        queue->submit(commands);
        
    } catch (const std::exception& e) {
        Logger::Error("Rendering failed: {}", e.what());
        // Handle error...
    }
}
```

## Command Naming Convention Summary

### Recorded Commands (cmd prefix)
```cpp
// These commands are recorded into command buffer
cmdSetGraphicsPipeline()    // Set pipeline state
cmdSetVertexBuffer()         // Bind vertex buffer
cmdSetBindGroup()            // Bind resource group
cmdDraw()                    // Record draw command
cmdDispatch()                // Record compute dispatch
cmdCopyBufferToBuffer()      // Record copy operation
```

### Immediate Operations (no prefix)
```cpp
// These execute immediately on CPU
beginRenderPass()    // Creates and returns encoder
end()                // Closes encoder
finish()             // Creates command buffer
submit()             // Submits to GPU queue
writeBuffer()        // Direct data upload
present()            // Present swapchain
```

---

This RAL provides type-safe, explicit control over GPU resources while maintaining clear separation between command recording and immediate execution.