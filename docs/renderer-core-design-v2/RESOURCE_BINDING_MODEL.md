# Pers Graphics Engine V2 - Resource Binding Model

## Overview

The resource binding model in Pers Graphics Engine V2 follows WebGPU's explicit binding model, providing type-safe, performant resource management with clear ownership semantics.

## Core Concepts

### 1. Bind Group Layout
Defines the **structure** of resources that will be bound together:
- Resource types (buffer, texture, sampler)
- Binding points (indices)
- Shader visibility (vertex, fragment, compute)
- Dynamic offset capability

### 2. Bind Group
Contains **actual resources** matching a layout:
- Immutable once created
- Can be reused across multiple draw calls
- Supports dynamic offsets for buffer bindings

### 3. Pipeline Layout
Defines the **complete set** of bind group layouts a pipeline expects:
- Up to 4 bind group layouts (sets 0-3)
- Must match shader expectations exactly
- Shared between compatible pipelines

---

## Frequency-Based Binding Pattern

Resources are organized by update frequency to minimize state changes:

```cpp
enum class BindGroupIndex : uint32_t {
    Frame    = 0,  // Updated once per frame
    Pass     = 1,  // Updated per render pass
    Material = 2,  // Updated per material change
    Object   = 3   // Updated per object (uses dynamic offsets)
};
```

### Set 0: Frame Resources
```cpp
struct FrameUniforms {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjMatrix;
    glm::vec3 cameraPosition;
    float time;
    float deltaTime;
};
```

### Set 1: Pass Resources
```cpp
struct PassUniforms {
    LightData lights[MAX_LIGHTS];
    uint32_t numLights;
    glm::vec4 ambientLight;
    // Shadow maps, environment maps, etc.
};
```

### Set 2: Material Resources
```cpp
struct MaterialResources {
    ITexture2D* baseColorTexture;
    ITexture2D* normalTexture;
    ITexture2D* metallicRoughnessTexture;
    ITexture2D* occlusionTexture;
    ITexture2D* emissiveTexture;
    ISampler* sampler;
    MaterialUniforms constants;
};
```

### Set 3: Object Resources (Dynamic)
```cpp
struct ObjectUniforms {
    glm::mat4 modelMatrix;
    glm::mat3 normalMatrix;
    uint32_t objectID;
};
```

---

## Creating Bind Groups

### Step 1: Define Bind Group Layout
```cpp
auto layoutDesc = BindGroupLayoutDesc{
    .entries = {
        // Binding 0: Uniform buffer
        {
            .binding = 0,
            .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
            .type = BindingType::UniformBuffer,
            .bufferDynamicOffset = false,
            .bufferMinBindingSize = sizeof(MaterialUniforms)
        },
        // Binding 1: Base color texture
        {
            .binding = 1,
            .visibility = ShaderStage::Fragment,
            .type = BindingType::SampledTexture,
            .textureSampleType = TextureSampleType::Float,
            .textureViewDimension = TextureViewDimension::D2
        },
        // Binding 2: Sampler
        {
            .binding = 2,
            .visibility = ShaderStage::Fragment,
            .type = BindingType::Sampler,
            .samplerType = SamplerBindingType::Filtering
        }
    }
};

auto bindGroupLayout = factory->createBindGroupLayout(layoutDesc);
```

### Step 2: Create Pipeline Layout
```cpp
auto pipelineLayoutDesc = PipelineLayoutDesc{
    .bindGroupLayouts = {
        frameBindGroupLayout,    // Set 0
        passBindGroupLayout,     // Set 1
        materialBindGroupLayout, // Set 2
        objectBindGroupLayout    // Set 3
    }
};

auto pipelineLayout = factory->createPipelineLayout(pipelineLayoutDesc);
```

### Step 3: Create Bind Group
```cpp
auto bindGroupDesc = BindGroupDesc{
    .layout = materialBindGroupLayout,
    .entries = {
        // Binding 0: Material uniforms
        {
            .binding = 0,
            .buffer = materialUniformBuffer,
            .offset = 0,
            .size = sizeof(MaterialUniforms)
        },
        // Binding 1: Base color texture
        {
            .binding = 1,
            .textureView = baseColorTexture->createView()
        },
        // Binding 2: Sampler
        {
            .binding = 2,
            .sampler = textureSampler
        }
    }
};

auto materialBindGroup = factory->createBindGroup(bindGroupDesc);
```

---

## Dynamic Offsets

Dynamic offsets allow reusing a single bind group with different buffer regions:

### Setup
```cpp
// Create large buffer for all objects
const size_t ALIGNMENT = 256;  // Min dynamic offset alignment
const size_t OBJECT_COUNT = 1000;
auto objectBuffer = factory->createUniformBuffer({
    .size = ALIGNMENT * OBJECT_COUNT,
    .isDynamic = true
});

// Create bind group with dynamic buffer
auto objectBindGroup = factory->createBindGroup({
    .layout = objectBindGroupLayout,
    .entries = {{
        .binding = 0,
        .buffer = objectBuffer,
        .offset = 0,
        .size = sizeof(ObjectUniforms)
    }}
});
```

### Usage
```cpp
// Update all object data
for (size_t i = 0; i < OBJECT_COUNT; ++i) {
    ObjectUniforms uniforms = calculateObjectUniforms(i);
    objectBuffer->update(&uniforms, i * ALIGNMENT, sizeof(ObjectUniforms));
}

// Render with dynamic offsets
for (size_t i = 0; i < OBJECT_COUNT; ++i) {
    uint32_t dynamicOffset = i * ALIGNMENT;
    renderPass->cmdSetBindGroup(3, objectBindGroup, {dynamicOffset});
    renderPass->cmdDrawIndexed(indexCount);
}
```

---

## Shader Interface

Shaders must declare bindings matching the layout:

### WGSL Example
```wgsl
// Set 0, Binding 0: Frame uniforms
struct FrameUniforms {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    viewProj: mat4x4<f32>,
    cameraPos: vec3<f32>,
    time: f32,
    deltaTime: f32,
}
@group(0) @binding(0) var<uniform> frame: FrameUniforms;

// Set 2, Binding 0: Material uniforms
struct MaterialUniforms {
    baseColor: vec4<f32>,
    metallic: f32,
    roughness: f32,
    emissive: vec3<f32>,
}
@group(2) @binding(0) var<uniform> material: MaterialUniforms;

// Set 2, Binding 1: Base color texture
@group(2) @binding(1) var baseColorTexture: texture_2d<f32>;

// Set 2, Binding 2: Sampler
@group(2) @binding(2) var textureSampler: sampler;

// Set 3, Binding 0: Object uniforms (dynamic)
struct ObjectUniforms {
    model: mat4x4<f32>,
    normalMatrix: mat3x3<f32>,
    objectID: u32,
}
@group(3) @binding(0) var<uniform> object: ObjectUniforms;
```

---

## Best Practices

### 1. Resource Lifetime Management
```cpp
class MaterialManager {
    std::unordered_map<MaterialID, std::shared_ptr<IBindGroup>> bindGroups;
    
    std::shared_ptr<IBindGroup> getOrCreate(MaterialID id) {
        if (auto it = bindGroups.find(id); it != bindGroups.end()) {
            return it->second;
        }
        return bindGroups[id] = createBindGroup(id);
    }
};
```

### 2. Bind Group Caching
```cpp
class BindGroupCache {
    struct Key {
        size_t layoutHash;
        std::vector<ResourceHandle> resources;
        
        bool operator==(const Key&) const = default;
    };
    
    std::unordered_map<Key, std::shared_ptr<IBindGroup>> cache;
    
    std::shared_ptr<IBindGroup> getOrCreate(
        const IBindGroupLayout* layout,
        const std::vector<BindGroupEntry>& entries
    ) {
        Key key = computeKey(layout, entries);
        if (auto it = cache.find(key); it != cache.end()) {
            return it->second;
        }
        return cache[key] = factory->createBindGroup({layout, entries});
    }
};
```

### 3. Efficient Updates
```cpp
class FrameResourceManager {
    static constexpr size_t FRAMES_IN_FLIGHT = 3;
    
    struct FrameData {
        std::shared_ptr<IUniformBuffer> frameUniformBuffer;
        std::shared_ptr<IBindGroup> frameBindGroup;
    };
    
    FrameData frames[FRAMES_IN_FLIGHT];
    size_t currentFrame = 0;
    
    void beginFrame() {
        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
        auto& frame = frames[currentFrame];
        
        // Update frame uniforms
        FrameUniforms uniforms = calculateFrameUniforms();
        frame.frameUniformBuffer->update(&uniforms);
    }
    
    std::shared_ptr<IBindGroup> getCurrentFrameBindGroup() {
        return frames[currentFrame].frameBindGroup;
    }
};
```

### 4. Validation and Debugging
```cpp
class BindGroupValidator {
    static bool validate(
        const IBindGroup* bindGroup,
        const IPipelineLayout* pipelineLayout,
        uint32_t setIndex
    ) {
        auto expectedLayout = pipelineLayout->getBindGroupLayout(setIndex);
        auto actualLayout = bindGroup->getLayout();
        
        if (!expectedLayout->isCompatible(actualLayout)) {
            Logger::Error("Bind group layout mismatch at set {}", setIndex);
            return false;
        }
        
        // Validate all resources are valid
        for (const auto& entry : bindGroup->getEntries()) {
            if (!validateResource(entry.resource)) {
                Logger::Error("Invalid resource at binding {}", entry.binding);
                return false;
            }
        }
        
        return true;
    }
};
```

---

## Performance Considerations

### 1. Minimize Bind Group Changes
- Sort draw calls by bind group to reduce state changes
- Use dynamic offsets instead of multiple bind groups when possible
- Batch similar materials together

### 2. Optimal Buffer Layout
```cpp
// Align structures to 256 bytes for dynamic offsets
struct alignas(256) ObjectUniformsAligned {
    ObjectUniforms data;
    uint8_t padding[256 - sizeof(ObjectUniforms)];
};
```

### 3. Resource Update Patterns
```cpp
// Good: Update once, use many times
uniformBuffer->update(data);
for (auto& object : objects) {
    renderPass->cmdSetBindGroup(0, bindGroup);
    renderPass->cmdDraw(...);
}

// Bad: Update per draw
for (auto& object : objects) {
    uniformBuffer->update(object.data);  // Stalls pipeline!
    renderPass->cmdSetBindGroup(0, bindGroup);
    renderPass->cmdDraw(...);
}
```

### 4. Memory Pooling
```cpp
class UniformBufferPool {
    struct BufferSlot {
        std::shared_ptr<IUniformBuffer> buffer;
        size_t offset;
        size_t size;
        bool inUse;
    };
    
    std::vector<BufferSlot> slots;
    
    BufferSlot* allocate(size_t size) {
        // Find free slot or create new buffer
        for (auto& slot : slots) {
            if (!slot.inUse && slot.size >= size) {
                slot.inUse = true;
                return &slot;
            }
        }
        return createNewSlot(size);
    }
    
    void release(BufferSlot* slot) {
        slot->inUse = false;
    }
};
```

---

## Common Patterns

### 1. Multi-Pass Rendering
```cpp
// Shadow pass
renderPass->cmdSetGraphicsPipeline(shadowPipeline);
renderPass->cmdSetBindGroup(0, shadowFrameBindGroup);
for (auto& object : shadowCasters) {
    renderPass->cmdSetBindGroup(3, object.bindGroup, {object.dynamicOffset});
    renderPass->cmdDrawIndexed(object.indexCount);
}

// Main pass
renderPass->cmdSetGraphicsPipeline(mainPipeline);
renderPass->cmdSetBindGroup(0, mainFrameBindGroup);
renderPass->cmdSetBindGroup(1, lightingBindGroup);  // Includes shadow maps
for (auto& object : visibleObjects) {
    renderPass->cmdSetBindGroup(2, object.material->bindGroup);
    renderPass->cmdSetBindGroup(3, object.bindGroup, {object.dynamicOffset});
    renderPass->cmdDrawIndexed(object.indexCount);
}
```

### 2. Instanced Rendering
```cpp
struct InstanceData {
    glm::mat4 modelMatrix;
    glm::vec4 color;
};

// Create instance buffer
auto instanceBuffer = factory->createStorageBuffer({
    .size = sizeof(InstanceData) * MAX_INSTANCES,
    .usage = StorageUsage::ReadOnly
});

// Update instances
std::vector<InstanceData> instances = gatherInstances();
instanceBuffer->update(instances.data(), 0, instances.size() * sizeof(InstanceData));

// Draw instanced
renderPass->cmdSetBindGroup(3, instanceBindGroup);
renderPass->cmdDrawIndexed(indexCount, instances.size());
```

### 3. Compute Dispatch
```cpp
// Compute pass for particle simulation
computePass->cmdSetComputePipeline(particlePipeline);
computePass->cmdSetBindGroup(0, timeBindGroup);
computePass->cmdSetBindGroup(1, particleBindGroup);
computePass->cmdDispatch(
    (particleCount + 63) / 64,  // Workgroups X
    1,                           // Workgroups Y
    1                            // Workgroups Z
);
```

---

## Troubleshooting

### Common Issues

1. **Bind Group Layout Mismatch**
   - Ensure shader declarations match bind group layout exactly
   - Check binding indices, types, and visibility flags

2. **Dynamic Offset Alignment**
   - All dynamic offsets must be aligned to 256 bytes minimum
   - Use proper padding in buffer structures

3. **Resource Lifetime**
   - Resources must outlive bind groups that reference them
   - Use shared_ptr for automatic lifetime management

4. **Performance Problems**
   - Profile bind group changes per frame
   - Check for unnecessary buffer updates
   - Verify proper use of dynamic offsets

---

This resource binding model provides a robust, type-safe foundation for GPU resource management while maintaining the performance benefits of modern graphics APIs.