# Depth Buffer Management Strategy

## Overview

This document describes the depth buffer management strategy for the Pers Graphics Engine, balancing WebGPU's explicit control philosophy with developer convenience.

## Core Principles

### 1. Explicit Control with Sensible Defaults
- WebGPU mandates explicit resource management, including depth buffers
- The engine provides automatic depth buffer management as a convenience layer
- Advanced users can override automatic behavior for custom scenarios

### 2. SwapChain Ownership Model
- Each SwapChain owns and manages its own depth buffer
- Depth buffer lifecycle is tied to SwapChain lifecycle
- Automatic resize handling when SwapChain dimensions change

## Implementation Strategy

### SwapChain Depth Buffer Management

```cpp
class ISwapChain {
public:
    // Depth buffer is enabled by default for 3D rendering
    virtual void setDepthBufferEnabled(bool enabled) = 0;
    
    // Returns nullptr if depth buffer is disabled
    virtual std::shared_ptr<ITextureView> getDepthTextureView() = 0;
};
```

**Key Features:**
- **Enabled by default**: Assumes 3D rendering as the common case
- **Lazy creation**: Depth texture created on first access
- **Automatic resize**: Recreated when SwapChain dimensions change
- **Optional disable**: Can be turned off for 2D/UI rendering

### RenderPass Automatic Linking

The engine automatically links SwapChain's depth buffer when appropriate:

```cpp
// Automatic depth attachment for SwapChain rendering
RenderPassEncoder* beginRenderPass(RenderPassDesc& desc) {
    // Check if rendering to SwapChain
    bool isSwapChainTarget = false;
    for (const auto& colorAttachment : desc.colorAttachments) {
        if (isFromSwapChain(colorAttachment.view)) {
            isSwapChainTarget = true;
            break;
        }
    }
    
    // Auto-attach depth if:
    // 1. No depth attachment specified
    // 2. Rendering to SwapChain
    // 3. SwapChain has depth buffer enabled
    if (!desc.depthAttachment && isSwapChainTarget) {
        if (auto depthView = swapChain->getDepthTextureView()) {
            desc.depthAttachment = {
                .view = depthView,
                .depthLoadOp = LoadOp::Clear,
                .depthStoreOp = StoreOp::Store,
                .depthClearValue = 1.0f
            };
        }
    }
    
    return createRenderPassInternal(desc);
}
```

### Pipeline Default Configuration

```cpp
struct DepthStencilState {
    // Standard 3D rendering defaults
    bool depthWriteEnabled = false;
    CompareFunction depthCompare = CompareFunction::Less;  // Forward Z
    
    // Helpers for common scenarios
    static DepthStencilState disabled() {
        return {
            .depthWriteEnabled = false,
            .depthCompare = CompareFunction::Undefined
        };
    }
    
    static DepthStencilState reverseZ() {
        return {
            .depthWriteEnabled = true,
            .depthCompare = CompareFunction::Greater
        };
    }
};
```

## Usage Patterns

### Basic 3D Rendering (Default Case)
```cpp
// Depth buffer automatically created and attached
auto renderPass = commandEncoder->beginRenderPass({
    .colorAttachments = { swapChain->getCurrentTextureView() }
    // depth automatically attached from SwapChain
});
```

### 2D/UI Rendering
```cpp
// Explicitly disable depth buffer for 2D
swapChain->setDepthBufferEnabled(false);

// Or use pipeline without depth testing
pipeline.depthStencil = DepthStencilState::disabled();
```

### Shadow Mapping
```cpp
// Custom depth-only pass with explicit attachment
auto shadowPass = commandEncoder->beginRenderPass({
    .depthAttachment = {
        .view = shadowMapDepthTexture,
        .depthLoadOp = LoadOp::Clear,
        .depthStoreOp = StoreOp::Store
    }
    // no color attachments
});
```

### Deferred Rendering
```cpp
// G-buffer pass creates depth
auto gBufferPass = commandEncoder->beginRenderPass({
    .colorAttachments = { albedo, normal, position },
    .depthAttachment = gBufferDepth  // Explicit custom depth
});

// Lighting pass reads depth
auto lightingPass = commandEncoder->beginRenderPass({
    .colorAttachments = { swapChain->getCurrentTextureView() },
    .depthAttachment = {
        .view = gBufferDepth,
        .depthLoadOp = LoadOp::Load,      // Preserve existing
        .depthStoreOp = StoreOp::DontCare  // Read-only
    }
});
```

### Post-Processing
```cpp
// Post-process typically doesn't need depth testing
auto postPass = commandEncoder->beginRenderPass({
    .colorAttachments = { swapChain->getCurrentTextureView() }
    // No depth attachment needed for fullscreen quad
});

// But can read depth as texture if needed
postProcessPipeline.bindTexture("depthTexture", previousDepth);
```

## Memory Management

### Single Buffer Reuse Strategy
- Each SwapChain maintains a single depth buffer
- Reused across frames (cleared each frame)
- Destroyed and recreated only on resize

### Format Selection
```cpp
TextureFormat getOptimalDepthFormat(IPhysicalDevice* device) {
    // Prefer 24-bit depth + 8-bit stencil for compatibility
    if (device->isFormatSupported(TextureFormat::Depth24PlusStencil8)) {
        return TextureFormat::Depth24PlusStencil8;
    }
    
    // Fallback to depth-only
    if (device->isFormatSupported(TextureFormat::Depth24Plus)) {
        return TextureFormat::Depth24Plus;
    }
    
    // Platform-specific optimal format
    return TextureFormat::Depth32Float;
}
```

## Performance Considerations

### Optimization Strategies
1. **Lazy Creation**: Depth buffer created only when first accessed
2. **Format Optimization**: Use platform-optimal depth format
3. **Clear Operation**: Use `LoadOp::Clear` for better tile-based GPU performance
4. **Early-Z Optimization**: Maintain `Less` comparison for hardware optimization

### Mobile Considerations
- Prefer `Depth24Plus` format (no stencil) when stencil not needed
- Use `StoreOp::DontCare` when depth won't be read later
- Consider MSAA resolve implications

## Validation and Error Handling

### Common Error Scenarios
1. **Pipeline-RenderPass Mismatch**: Pipeline expects depth but RenderPass has none
   - Solution: Automatic attachment or validation error

2. **Format Incompatibility**: Pipeline depth format doesn't match attachment
   - Solution: Validate formats during pipeline creation

3. **Resize During Frame**: Window resized while rendering
   - Solution: Defer resize until frame boundary

## Future Enhancements

### Planned Features
1. **Depth Buffer Pool**: For multiple shadow cascades
2. **Temporal Depth**: Previous frame depth for temporal effects
3. **Variable Rate Shading**: Depth-based shading rate
4. **Depth Bounds Test**: Additional culling optimization

### API Extensions
```cpp
// Future: Advanced depth management
class DepthBufferManager {
    // Pool management for special cases
    std::shared_ptr<ITextureView> acquireTemporaryDepth(
        uint32_t width, 
        uint32_t height,
        TextureFormat format
    );
    
    void releaseTemporaryDepth(std::shared_ptr<ITextureView> depth);
};
```

## References

- [WebGPU Specification - Depth/Stencil](https://www.w3.org/TR/webgpu/#depth-stencil)
- [Vulkan Best Practices - Depth Buffer](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/depth.adoc)
- [Unity Depth Texture Documentation](https://docs.unity3d.com/Manual/SL-CameraDepthTexture.html)
- [Unreal Engine Depth Buffer Overview](https://docs.unrealengine.com/5.0/en-US/depth-buffer-visualization)