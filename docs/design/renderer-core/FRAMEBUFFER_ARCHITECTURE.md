# Framebuffer Architecture

## Overview

This document describes the Framebuffer-based rendering architecture for the Pers Graphics Engine. The design unifies surface (SwapChain) and offscreen render targets under a common Framebuffer abstraction, following WebGPU/Vulkan patterns.

## Core Design Principles

### 1. Unified Framebuffer Abstraction
- All render targets (surface and offscreen) implement `IFramebuffer`
- SwapChain backbuffers are treated as a special case of Framebuffer
- Consistent interface for attachments across all framebuffer types

### 2. Explicit Resource Management
- No hidden allocations or automatic depth buffer creation
- Clear ownership and lifecycle for all resources
- RTTI-based type checking instead of virtual flags

### 3. Flexible Composition
- Depth buffers can be owned by framebuffers or managed separately
- Framebuffers can be resizable or fixed-size
- Mix-and-match attachments for advanced rendering techniques

## Architecture

### Interface Hierarchy

```cpp
IFramebuffer                    // Base interface for all framebuffers
├── IResizableFramebuffer       // Adds resize capability
│   ├── ISurfaceFramebuffer     // Surface-specific (acquire/present)
│   └── OffscreenFramebuffer    // Standard offscreen render target
└── FixedSizeFramebuffer        // Non-resizable framebuffer
```

### Core Interfaces

```cpp
// Base framebuffer interface
class IFramebuffer {
public:
    virtual ~IFramebuffer() = default;
    
    // Attachments
    virtual std::shared_ptr<ITextureView> getColorAttachment(uint32_t index = 0) const = 0;
    virtual std::shared_ptr<ITextureView> getDepthStencilAttachment() const = 0;
    
    // Properties
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getSampleCount() const = 0;
    virtual TextureFormat getColorFormat(uint32_t index = 0) const = 0;
    virtual TextureFormat getDepthFormat() const = 0;
};

// Resizable framebuffer
class IResizableFramebuffer : public IFramebuffer {
public:
    virtual bool resize(uint32_t width, uint32_t height) = 0;
};

// Surface framebuffer with present capabilities
class ISurfaceFramebuffer : public IResizableFramebuffer {
public:
    virtual bool acquireNextImage() = 0;
    virtual void present() = 0;
    virtual bool isReady() const = 0;
};
```

## Framebuffer Types

### 1. SurfaceFramebuffer
- Wraps SwapChain backbuffer
- Manages acquire/present lifecycle
- Optional depth attachment support
- Always sampleCount = 1

### 2. OffscreenFramebuffer
- General-purpose render target
- Supports MSAA (sampleCount > 1)
- Can include depth attachment
- Resizable by default

### 3. DepthOnlyFramebuffer
- Specialized for shadow mapping
- No color attachments
- Optimized for depth-only passes

### 4. FixedSizeFramebuffer
- Non-resizable render targets
- Used for fixed-resolution effects
- Memory-efficient for static content

## Depth Buffer Management

### Flexible Ownership Model

```cpp
// Option 1: Framebuffer owns depth
auto framebuffer = FramebufferFactory::createOffscreenFramebuffer(
    device, width, height, 
    TextureFormat::BGRA8Unorm,
    true  // createDepth
);

// Option 2: External depth management
auto colorFramebuffer = FramebufferFactory::createOffscreenFramebuffer(
    device, width, height,
    TextureFormat::BGRA8Unorm,
    false  // no depth
);
auto depthFramebuffer = std::make_shared<DepthOnlyFramebuffer>(device, width, height);

// Option 3: Surface with optional depth
auto surface = FramebufferFactory::createSurfaceFramebuffer(surface, device, width, height);
surface->setDepthFramebuffer(depthFramebuffer);  // Optional association
```

### Depth Sharing Patterns

```cpp
// Multiple passes sharing depth
auto gBufferDepth = std::make_shared<DepthOnlyFramebuffer>(device, width, height);

// G-buffer pass
RenderPassDesc gBufferPass;
gBufferPass.depthStencilAttachment = {
    .view = gBufferDepth->getDepthStencilAttachment(),
    .depthLoadOp = LoadOp::Clear,
    .depthStoreOp = StoreOp::Store
};

// Lighting pass (read-only depth)
RenderPassDesc lightingPass;
lightingPass.depthStencilAttachment = {
    .view = gBufferDepth->getDepthStencilAttachment(),
    .depthLoadOp = LoadOp::Load,
    .depthStoreOp = StoreOp::Store,
    .depthReadOnly = true
};
```

## MSAA Support

### MSAA Framebuffer Creation

```cpp
// Create 4x MSAA framebuffer
auto msaaFramebuffer = FramebufferFactory::createMSAAFramebuffer(
    device, width, height,
    4,  // sampleCount
    TextureFormat::BGRA8Unorm,
    true  // includeDepth
);

// Render with MSAA
RenderPassDesc desc;
desc.colorAttachments.push_back({
    .view = msaaFramebuffer->getColorAttachment(0),
    .resolveTarget = surface->getColorAttachment(0),  // Resolve to backbuffer
    .loadOp = LoadOp::Clear,
    .storeOp = StoreOp::Discard  // MSAA texture discarded after resolve
});
```

### MSAA Resolve Process

```
[MSAA Framebuffer]          [Surface Framebuffer]
  sampleCount=4      →         sampleCount=1
  (Render target)         (Resolve target/Backbuffer)
```

## Usage Patterns

### Basic Rendering

```cpp
class SimpleRenderer {
    std::shared_ptr<ISurfaceFramebuffer> _surface;
    
    void renderFrame() {
        // Acquire backbuffer
        if (!_surface->acquireNextImage()) {
            LOG_ERROR("Failed to acquire image");
            return;
        }
        
        // Configure render pass
        RenderPassDesc desc;
        desc.colorAttachments.push_back({
            .view = _surface->getColorAttachment(0),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store
        });
        
        // Optional depth
        if (auto depth = _surface->getDepthStencilAttachment()) {
            desc.depthStencilAttachment = {
                .view = depth,
                .depthLoadOp = LoadOp::Clear,
                .depthStoreOp = StoreOp::Store
            };
        }
        
        // Render
        auto renderPass = encoder->beginRenderPass(desc);
        // ... draw calls ...
        renderPass->end();
        
        // Present
        _surface->present();
    }
};
```

### Deferred Rendering

```cpp
class DeferredRenderer {
    std::shared_ptr<IFramebuffer> _gBuffer;
    std::shared_ptr<ISurfaceFramebuffer> _surface;
    
    void renderFrame() {
        // G-buffer pass
        RenderPassDesc gBufferDesc;
        gBufferDesc.colorAttachments.push_back({_gBuffer->getColorAttachment(0)});  // Albedo
        gBufferDesc.colorAttachments.push_back({_gBuffer->getColorAttachment(1)});  // Normal
        gBufferDesc.colorAttachments.push_back({_gBuffer->getColorAttachment(2)});  // Position
        gBufferDesc.depthStencilAttachment = {
            .view = _gBuffer->getDepthStencilAttachment()
        };
        
        // Lighting pass
        _surface->acquireNextImage();
        RenderPassDesc lightingDesc;
        lightingDesc.colorAttachments.push_back({
            .view = _surface->getColorAttachment(0)
        });
        
        // ... render ...
        _surface->present();
    }
};
```

### Shadow Mapping

```cpp
class ShadowMapper {
    std::shared_ptr<DepthOnlyFramebuffer> _shadowMap;
    
    void renderShadowMap() {
        RenderPassDesc shadowDesc;
        // No color attachments for depth-only pass
        shadowDesc.depthStencilAttachment = {
            .view = _shadowMap->getDepthStencilAttachment(),
            .depthLoadOp = LoadOp::Clear,
            .depthStoreOp = StoreOp::Store,
            .depthClearValue = 1.0f
        };
        
        auto renderPass = encoder->beginRenderPass(shadowDesc);
        // ... render shadow casters ...
    }
};
```

## Type Checking with RTTI

```cpp
void handleFramebuffer(std::shared_ptr<IFramebuffer> fb) {
    // Check if surface framebuffer
    if (auto surface = dynamic_cast<ISurfaceFramebuffer*>(fb.get())) {
        surface->acquireNextImage();
        // ... render ...
        surface->present();
    }
    
    // Check if resizable
    if (auto resizable = dynamic_cast<IResizableFramebuffer*>(fb.get())) {
        resizable->resize(newWidth, newHeight);
    }
    
    // Check sample count for MSAA
    if (fb->getSampleCount() > 1) {
        // Handle MSAA framebuffer
    }
}
```

## Error Handling

### No Exceptions Policy

```cpp
// All operations return success/failure
if (!surface->acquireNextImage()) {
    LOG_ERROR("Failed to acquire surface texture");
    return;
}

if (!resizable->resize(width, height)) {
    LOG_WARNING("Resize failed, using previous size");
}

// Null checks for optional attachments
if (auto depth = framebuffer->getDepthStencilAttachment()) {
    // Use depth
} else {
    LOG_INFO("No depth attachment available");
}
```

## Memory Management

### Lifecycle Management
- Framebuffers own their textures
- Automatic cleanup in destructors
- Shared_ptr for reference counting

### Resize Strategy
- Destroy old textures before creating new ones
- Validate new size before allocation
- Fallback to previous size on failure

## Performance Considerations

### Optimization Strategies
1. **Framebuffer Reuse**: Cache framebuffers across frames
2. **Lazy Creation**: Create attachments only when needed
3. **Format Optimization**: Use platform-optimal formats
4. **MSAA Levels**: Balance quality vs performance (typically 4x)

### Mobile Considerations
- Prefer fixed-size framebuffers when possible
- Use `StoreOp::Discard` for temporary attachments
- Consider bandwidth costs of MSAA resolve

## Implementation Status

### Completed
- [x] Interface design
- [x] Architecture documentation

### In Progress
- [ ] IFramebuffer interface implementation
- [ ] SurfaceFramebuffer implementation
- [ ] OffscreenFramebuffer implementation
- [ ] FramebufferFactory implementation

### Future Work
- [ ] MRT support for deferred rendering
- [ ] Framebuffer cache/pool
- [ ] Automatic format selection
- [ ] Performance profiling

## References

- [WebGPU Specification - Render Pass](https://www.w3.org/TR/webgpu/#render-pass-encoder)
- [Vulkan Framebuffer](https://www.khronos.org/registry/vulkan/specs/1.3/html/vkspec.html#VkFramebuffer)
- [Metal Render Pass Descriptor](https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor)