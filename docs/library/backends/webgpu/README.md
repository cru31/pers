# WebGPU Backend Documentation

> **Status**: Beta
> **Since**: v1.0.0-alpha
> **Platform Support**: Windows, Linux, macOS

## Overview

The WebGPU backend provides a modern, safe, and performant graphics API implementation for the Pers Graphics Engine. Built on top of wgpu-native, it offers cross-platform support with a consistent API surface across all platforms.

## Table of Contents

- [Architecture](#architecture)
- [Device Management](#device-management)
- [Feature Support](#feature-support)
- [Platform Notes](#platform-notes)
- [Limitations](#limitations)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [API Reference](#api-reference)

## Architecture

### Component Hierarchy
```
WebGPUInstance
    ├─ WebGPUPhysicalDevice (Adapter)
    │   └─ WebGPULogicalDevice
    │       └─ WebGPUQueue
    └─ Surface Management
```

### Backend Implementation
The WebGPU backend uses wgpu-native, a native implementation of the WebGPU specification that provides:
- **Windows**: Direct3D 12 backend
- **macOS**: Metal backend
- **Linux**: Vulkan backend
- **Fallback**: OpenGL ES 3.0 (software renderer)

## Device Management

### Instance Creation
The WebGPU instance manages adapter enumeration and physical device creation.

```cpp
InstanceDesc desc;
desc.applicationName = "MyApp";
desc.enableValidation = true;
desc.allowSoftwareRenderer = false;

auto instance = factory->createInstance(desc);
```

### Physical Device Selection
Physical devices (adapters) represent available GPUs. The selection process considers:
- Power preference (high performance, low power)
- Surface compatibility
- Required features
- Adapter type (discrete, integrated, software)

```cpp
PhysicalDeviceOptions options;
options.powerPreference = PowerPreference::HighPerformance;
options.compatibleSurface = surface;

auto physicalDevice = instance->requestPhysicalDevice(options);
```

### Logical Device Creation
Logical devices provide access to GPU resources and command queues.

```cpp
LogicalDeviceDesc desc;
desc.enableValidation = true;
desc.requiredFeatures = {
    DeviceFeature::TimestampQuery,
    DeviceFeature::Depth32FloatStencil8
};

auto logicalDevice = physicalDevice->createLogicalDevice(desc);
```

For detailed implementation guide, see [Device Creation Guide](device-creation.md).

## Feature Support

### Core Features
| Feature | Status | Notes |
|---------|--------|-------|
| Basic Rendering | ✅ Implemented | Full support for vertex/fragment shaders |
| Compute Shaders | ✅ Implemented | Compute pipeline support |
| Surface Presentation | ✅ Implemented | Native window system integration |
| Multi-Queue | ✅ Implemented | Single default queue per device |
| Timestamp Queries | ✅ Supported | GPU timing measurements |
| Validation Layers | ✅ Implemented | Debug and validation support |

### Optional Features
| Feature | Detection | Notes |
|---------|-----------|-------|
| Depth Clip Control | Runtime | Control depth clipping behavior |
| Depth32Float-Stencil8 | Runtime | 32-bit float depth with stencil |
| Shader F16 | Runtime | 16-bit float support in shaders |
| Texture Compression BC | Runtime | BC1-BC7 formats |
| Texture Compression ETC2 | Runtime | ETC2/EAC formats |
| Texture Compression ASTC | Runtime | ASTC formats |
| Indirect First Instance | Runtime | First instance in indirect draws |
| RG11B10Ufloat Renderable | Runtime | RG11B10 format rendering |
| BGRA8Unorm Storage | Runtime | BGRA8 storage textures |
| Float32 Filterable | Runtime | 32-bit float texture filtering |

### Unsupported Features
| Feature | Reason | Alternative |
|---------|--------|-------------|
| Ray Tracing | Not in WebGPU spec | Use compute shaders |
| Tessellation | Not in WebGPU spec | Use geometry shaders or compute |
| Geometry Shaders | Not in WebGPU spec | Use vertex pulling |
| Pipeline Statistics | Not standardized yet | Use timestamp queries |

## Platform Notes

### Windows
- Uses Direct3D 12 backend by default
- Requires Windows 10 version 1903 or later
- Supports both discrete and integrated GPUs
- Software renderer available via WARP

### Linux
- Uses Vulkan backend
- Requires Vulkan 1.1 or later
- May fall back to OpenGL ES 3.0
- X11 and Wayland support

### macOS
- Uses Metal backend
- Requires macOS 10.15 or later
- Full support for Apple Silicon
- Rosetta 2 compatibility for Intel apps on ARM

## Limitations

### Current Limitations
- **Single Queue**: Only one queue per device (WebGPU limitation)
- **No Multi-GPU**: Explicit multi-GPU not supported
- **Limited Push Constants**: No push constant support
- **No Sparse Resources**: Sparse textures not available
- **Buffer Size**: Maximum buffer size limited to 256MB by default

### API Differences from Native APIs
- No direct memory management
- Simplified synchronization model
- Limited pipeline state control
- No custom allocators

## Performance

### Optimization Tips
1. **Batch Draw Calls**: Minimize state changes between draws
2. **Use Bind Group Caching**: Reuse bind groups when possible
3. **Prefer Indexed Drawing**: Use index buffers for geometry
4. **Minimize Barriers**: WebGPU handles most barriers automatically
5. **Use Appropriate Formats**: Choose optimal texture formats

### Memory Management
- Automatic reference counting for all resources
- No manual memory allocation required
- Resources freed when last reference dropped
- GPU memory tracked internally

### Profiling
- Use timestamp queries for GPU timing
- Enable validation for correctness checking
- Monitor adapter memory usage via capabilities

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "No compatible adapter" | No GPU meets requirements | Lower requirements or enable software renderer |
| "Surface not supported" | Window system incompatible | Check platform-specific surface creation |
| "Feature not available" | Optional feature unsupported | Check capabilities before requesting |
| "Device lost" | GPU reset or driver crash | Recreate device and resources |
| "Out of memory" | GPU memory exhausted | Reduce resource usage |

### Debug Tips
1. **Enable Validation**: Always enable during development
2. **Check Logs**: Review pers::Logger output for warnings
3. **Verify Features**: Query capabilities before use
4. **Test Fallbacks**: Test with software renderer

## API Reference

### Core Classes
- [WebGPUInstance](../../api-reference/graphics/webgpu-instance.md)
- [WebGPUPhysicalDevice](../../api-reference/graphics/webgpu-physical-device.md)
- [WebGPULogicalDevice](../../api-reference/graphics/webgpu-logical-device.md)
- [WebGPUQueue](../../api-reference/graphics/webgpu-queue.md)

### Implementation Details
- [Device Creation](device-creation.md)
- [Feature Detection](feature-detection.md)
- [Surface Management](surface-management.md)
- [Resource Creation](resource-creation.md)

## Version History

| Version | Changes | Notes |
|---------|---------|-------|
| v1.0.0-alpha | Initial WebGPU backend | Basic rendering support |

## See Also
- [WebGPU Specification](https://www.w3.org/TR/webgpu/)
- [wgpu-native Documentation](https://github.com/gfx-rs/wgpu-native)
- [Learn WebGPU Tutorial](https://eliemichel.github.io/LearnWebGPU/)

---

*Last Updated: 2024-08-30*