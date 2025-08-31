# SwapChain

> **Status**: Stable  
> **Since**: v1.0.0  
> **Backend**: WebGPU

## Overview

The SwapChain manages a chain of textures for presenting rendered frames to a surface. It handles frame buffering, synchronization, and presentation to the display. The WebGPU implementation uses the modern Surface API instead of the deprecated SwapChain API.

## Table of Contents
- [Key Concepts](#key-concepts)
- [API Reference](#api-reference)
- [Usage](#usage)
- [Platform Notes](#platform-notes)
- [Limitations](#limitations)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [See Also](#see-also)

## Key Concepts

### Surface vs SwapChain
In modern WebGPU (wgpu-native v0.19+), the traditional SwapChain API has been replaced with a Surface-based approach:
- **Surface**: Represents the destination for rendering (window, canvas, etc.)
- **Surface Configuration**: Defines format, size, and presentation mode
- **Surface Texture**: The current frame's texture obtained from the surface

### Presentation Modes
- **Fifo**: VSync enabled, frames presented in order (default)
- **Immediate**: No VSync, minimal latency
- **Mailbox**: Triple buffering with VSync
- **FifoRelaxed**: Adaptive VSync

### Texture Formats
Common swap chain formats:
- `BGRA8Unorm`: Standard 8-bit sRGB (most compatible)
- `BGRA8UnormSrgb`: sRGB color space
- `RGBA8Unorm`: Alternative 8-bit format
- `RGBA16Float`: HDR rendering

## API Reference

### Classes/Interfaces

#### `ISwapChain`
Abstract interface for swap chain management.

**Header**: `pers/graphics/ISwapChain.h`

#### `WebGPUSwapChain`
WebGPU implementation using Surface API.

**Header**: `pers/graphics/backends/webgpu/WebGPUSwapChain.h`

### Methods

#### `getCurrentTextureView`
```cpp
std::shared_ptr<ITextureView> getCurrentTextureView();
```

**Returns:**
- Valid texture view for the current frame
- `nullptr` if surface is lost or needs reconfiguration

**Thread Safety:**
- Not thread-safe, call from rendering thread only

**Example:**
```cpp
// Get texture view for current frame
auto textureView = swapChain->getCurrentTextureView();
if (!textureView) {
    // Handle surface loss/resize
    swapChain->resize(newWidth, newHeight);
    textureView = swapChain->getCurrentTextureView();
}
```

#### `present`
```cpp
void present();
```

**Description:**
Presents the current frame to the display. Must be called after rendering is complete.

**Thread Safety:**
- Not thread-safe, call from rendering thread only

#### `resize`
```cpp
void resize(uint32_t width, uint32_t height);
```

**Parameters:**
- `width`: New width in pixels (must be > 0)
- `height`: New height in pixels (must be > 0)

**Notes:**
- Automatically reconfigures the surface
- Invalidates current texture view

### Structures

#### `SwapChainDesc`
```cpp
struct SwapChainDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::BGRA8Unorm;
    PresentMode presentMode = PresentMode::Fifo;
    CompositeAlphaMode alphaMode = CompositeAlphaMode::Opaque;
    std::string debugName;
};
```

#### `SurfaceCapabilities`
```cpp
struct SurfaceCapabilities {
    std::vector<TextureFormat> formats;
    std::vector<PresentMode> presentModes;
    std::vector<CompositeAlphaMode> alphaModes;
    uint32_t minWidth, maxWidth;
    uint32_t minHeight, maxHeight;
    uint32_t minImageCount, maxImageCount;
};
```

### Builder Pattern

#### `SwapChainDescBuilder`
Provides capability negotiation and automatic configuration.

```cpp
SwapChainDescBuilder builder;
builder.withSurfaceCapabilities(caps)
       .withSize(1920, 1080)
       .preferredFormat(TextureFormat::BGRA8UnormSrgb)
       .preferredPresentMode(PresentMode::Mailbox);

auto result = builder.build();
if (result.isValid) {
    auto swapChain = device->createSwapChain(surface, result.desc);
}
```

## Usage

### Basic Usage
```cpp
// Create swap chain with default settings
SwapChainDesc desc;
desc.width = 1280;
desc.height = 720;
desc.format = TextureFormat::BGRA8Unorm;
desc.presentMode = PresentMode::Fifo;

auto swapChain = device->createSwapChain(surface, desc);

// Render loop
while (running) {
    auto textureView = swapChain->getCurrentTextureView();
    if (textureView) {
        // Render to textureView
        renderFrame(textureView);
        swapChain->present();
    }
}
```

### Advanced Usage with Capability Negotiation
```cpp
// Query surface capabilities
auto caps = WebGPUSwapChain::querySurfaceCapabilities(
    device->getNativeDeviceHandle().as<WGPUDevice>(),
    adapter,
    surface
);

// Build optimal configuration
SwapChainDescBuilder builder;
auto result = builder
    .withSurfaceCapabilities(caps)
    .withSize(window.width, window.height)
    .preferredFormat(TextureFormat::BGRA8UnormSrgb)
    .preferredPresentMode(PresentMode::Mailbox)
    .withFallbacks(true)
    .build();

if (!result.isValid) {
    // Handle configuration failure
    Logger::Instance().Log(LogLevel::Error, "SwapChain", 
                          result.getErrorMessage(), PERS_SOURCE_LOC);
    return;
}

auto swapChain = device->createSwapChain(surface, result.desc);
```

## Platform Notes

### Windows
- Preferred formats: `BGRA8Unorm`, `BGRA8UnormSrgb`
- Supports all presentation modes
- Window resizing requires explicit `resize()` call

### Linux
- X11 and Wayland supported
- Format support varies by GPU driver
- Mailbox mode may not be available on all systems

### macOS
- Metal backend uses `BGRA8Unorm` exclusively
- Limited to Fifo and Immediate modes
- Retina displays require special handling for backing scale

## Limitations

### Current Limitations
- Maximum texture dimensions: 8192x8192 (conservative default)
- No support for multi-sampling in swap chain textures
- Surface reconfiguration required on resize
- No direct access to native swap chain handle (Surface API)

### Not Supported
- Stereoscopic 3D presentation
- Variable refresh rate (VRR) explicit control
- HDR metadata attachment

## Performance

### Performance Characteristics
- Frame acquisition: O(1)
- Present operation: O(1) + GPU synchronization
- Resize operation: O(1) + surface reconfiguration

### Optimization Tips
- Use Mailbox mode for lowest latency with VSync
- Immediate mode for benchmarking and minimum latency
- Call `getCurrentTextureView()` only once per frame
- Handle suboptimal surface state proactively

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| `getCurrentTextureView()` returns nullptr | Surface lost or outdated | Call `resize()` with current dimensions |
| Black screen after resize | Surface not reconfigured | Ensure `resize()` is called on window resize |
| Tearing in Immediate mode | No VSync | Expected behavior, use Fifo for VSync |
| Suboptimal performance warning | Surface configuration mismatch | Recreate with queried capabilities |

### Surface Status Handling
```cpp
// In WebGPUSwapChain::getCurrentTextureView()
switch (surfaceTexture.status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        // Ideal state, continue
        break;
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        // Works but not optimal, consider reconfiguration
        Logger::Instance().Log(LogLevel::Debug, "SwapChain", 
                              "Surface suboptimal", PERS_SOURCE_LOC);
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        // Surface needs reconfiguration
        return nullptr;
}
```

## Version History

| Version | Changes | Migration Notes |
|---------|---------|-----------------|
| v1.0.0 | Initial WebGPU implementation | - |
| v1.1.0 | Migrated from WGPUSwapChain to Surface API | No API changes |
| v1.2.0 | Added SwapChainDescBuilder | Optional helper |

## See Also
- [Logical Device](logical-device.md)
- [Texture Views](texture-view.md)
- [Render Pass](../rendering/render-pass.md)
- [WebGPU Backend Guide](../../guides/webgpu-backend.md)