# WebGPU Feature Detection Guide

> **Status**: Stable
> **Since**: v1.0.0-alpha
> **Backend**: WebGPU

## Overview

This guide explains how to detect and request optional WebGPU features in the Pers Graphics Engine. Proper feature detection ensures your application works across different hardware while taking advantage of available capabilities.

## Table of Contents
- [Key Concepts](#key-concepts)
- [Available Features](#available-features)
- [Feature Detection](#feature-detection)
- [Feature Requirements](#feature-requirements)
- [Platform Support](#platform-support)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [See Also](#see-also)

## Key Concepts

### Feature Categories

1. **Core Features**: Always available in WebGPU
   - Basic rendering (vertex/fragment shaders)
   - Compute shaders
   - Basic texture formats
   - Standard buffer operations

2. **Optional Features**: Hardware/driver dependent
   - Advanced texture formats
   - Specialized shader capabilities
   - Extended limits
   - Query types

3. **Unsupported Features**: Not in WebGPU specification
   - Ray tracing
   - Tessellation
   - Geometry shaders
   - Mesh shaders

### Feature Detection Flow
```
1. Query Physical Device Capabilities
2. Check Required vs Available Features
3. Request Only Available Features
4. Handle Missing Features Gracefully
```

## Available Features

### Shader Features

#### `ShaderF16`
**WebGPU Feature**: `shader-f16`
- 16-bit floating point support in shaders
- Enables `f16` type in WGSL
- Improves performance for certain workloads
- Common on modern GPUs

```cpp
if (capabilities.supportsShaderF16) {
    desc.requiredFeatures.push_back(DeviceFeature::ShaderF16);
    // Can use f16 in shaders
}
```

### Texture Compression Features

#### `TextureCompressionBC`
**WebGPU Feature**: `texture-compression-bc`
- BC1-BC7 compression formats
- Common on desktop GPUs
- Reduces texture memory usage
- Not typically available on mobile

```cpp
if (capabilities.supportsTextureCompressionBC) {
    desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionBC);
    // Can use BC compressed textures
}
```

#### `TextureCompressionETC2`
**WebGPU Feature**: `texture-compression-etc2`
- ETC2 and EAC compression formats
- Common on mobile GPUs
- OpenGL ES 3.0 standard
- May not be available on desktop

```cpp
if (capabilities.supportsTextureCompressionETC2) {
    desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionETC2);
    // Can use ETC2 compressed textures
}
```

#### `TextureCompressionASTC`
**WebGPU Feature**: `texture-compression-astc`
- Adaptive Scalable Texture Compression
- High quality compression
- Variable block sizes
- Newer GPUs only

```cpp
if (capabilities.supportsTextureCompressionASTC) {
    desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionASTC);
    // Can use ASTC compressed textures
}
```

### Depth/Stencil Features

#### `Depth32FloatStencil8`
**WebGPU Feature**: `depth32float-stencil8`
- 32-bit float depth + 8-bit stencil format
- High precision depth buffer
- Combined depth/stencil attachment
- Widely supported

```cpp
if (capabilities.supportsDepth32FloatStencil8) {
    desc.requiredFeatures.push_back(DeviceFeature::Depth32FloatStencil8);
    // Can use D32_FLOAT_S8 format
}
```

#### `DepthClipControl`
**WebGPU Feature**: `depth-clip-control`
- Control depth clipping behavior
- Disable near/far plane clipping
- Useful for special rendering techniques
- Hardware dependent

```cpp
if (capabilities.supportsDepthClipControl) {
    desc.requiredFeatures.push_back(DeviceFeature::DepthClipControl);
    // Can control depth clipping
}
```

### Rendering Features

#### `RG11B10UfloatRenderable`
**WebGPU Feature**: `rg11b10ufloat-renderable`
- RG11B10 format as render target
- HDR rendering support
- Reduced memory vs RGBA16F
- Common on modern GPUs

```cpp
if (capabilities.supportsRG11B10UfloatRenderable) {
    desc.requiredFeatures.push_back(DeviceFeature::RG11B10UfloatRenderable);
    // Can render to RG11B10 format
}
```

#### `BGRA8UnormStorage`
**WebGPU Feature**: `bgra8unorm-storage`
- BGRA8 format for storage textures
- Direct write from compute shaders
- Platform-specific optimization
- Check availability

```cpp
if (capabilities.supportsBGRA8UnormStorage) {
    desc.requiredFeatures.push_back(DeviceFeature::BGRA8UnormStorage);
    // Can use BGRA8 storage textures
}
```

#### `Float32Filterable`
**WebGPU Feature**: `float32-filterable`
- Linear filtering for 32-bit float textures
- High precision filtering
- Performance impact
- Not universally supported

```cpp
if (capabilities.supportsFloat32Filterable) {
    desc.requiredFeatures.push_back(DeviceFeature::Float32Filterable);
    // Can filter 32-bit float textures
}
```

### Query Features

#### `TimestampQuery`
**WebGPU Feature**: `timestamp-query`
- GPU timestamp measurements
- Performance profiling
- Frame timing analysis
- Widely supported

```cpp
if (capabilities.supportsTimestampQuery) {
    desc.requiredFeatures.push_back(DeviceFeature::TimestampQuery);
    // Can measure GPU timings
}
```

#### `PipelineStatisticsQuery`
**WebGPU Feature**: `pipeline-statistics-query`
- Detailed pipeline statistics
- Vertex/fragment counts
- Not in WebGPU spec yet
- Currently unsupported

```cpp
// Note: Not currently available in WebGPU
if (capabilities.supportsPipelineStatisticsQuery) {
    // Would enable pipeline statistics
}
```

### Indirect Drawing Features

#### `IndirectFirstInstance`
**WebGPU Feature**: `indirect-first-instance`
- First instance in indirect draw calls
- GPU-driven rendering
- Instancing optimization
- Check before use

```cpp
if (capabilities.supportsIndirectFirstInstance) {
    desc.requiredFeatures.push_back(DeviceFeature::IndirectFirstInstance);
    // Can use first_instance in indirect draws
}
```

## Feature Detection

### Basic Pattern
```cpp
// Step 1: Get capabilities
auto caps = physicalDevice->getCapabilities();

// Step 2: Check what's available
LogicalDeviceDesc desc;
desc.enableValidation = true;

// Step 3: Request only available features
if (caps.supportsDepth32FloatStencil8) {
    desc.requiredFeatures.push_back(DeviceFeature::Depth32FloatStencil8);
}

if (caps.supportsTimestampQuery) {
    desc.requiredFeatures.push_back(DeviceFeature::TimestampQuery);
}

// Step 4: Create device
auto device = physicalDevice->createLogicalDevice(desc);
```

### Conditional Feature Usage
```cpp
class Renderer {
    bool _hasBC = false;
    bool _hasTimestamps = false;
    
    void initialize(const PhysicalDeviceCapabilities& caps) {
        _hasBC = caps.supportsTextureCompressionBC;
        _hasTimestamps = caps.supportsTimestampQuery;
    }
    
    std::shared_ptr<Texture> loadTexture(const std::string& path) {
        if (_hasBC && path.ends_with(".dds")) {
            return loadBCTexture(path);
        } else {
            return loadUncompressedTexture(path);
        }
    }
    
    void beginGPUTimer() {
        if (_hasTimestamps) {
            // Start timestamp query
        }
    }
};
```

## Feature Requirements

### Minimum Requirements
Applications should work with minimal features:
```cpp
// Minimal device - works everywhere
LogicalDeviceDesc minimalDesc;
minimalDesc.enableValidation = false;
// No required features
auto device = physicalDevice->createLogicalDevice(minimalDesc);
```

### Recommended Features
Common features for modern rendering:
```cpp
// Recommended for most applications
std::vector<DeviceFeature> recommended = {
    DeviceFeature::Depth32FloatStencil8,  // High precision depth
    DeviceFeature::TimestampQuery,        // Profiling
};

for (auto feature : recommended) {
    if (isFeatureAvailable(caps, feature)) {
        desc.requiredFeatures.push_back(feature);
    }
}
```

### Advanced Features
For specific use cases:
```cpp
// HDR rendering requirements
if (needsHDR) {
    if (caps.supportsRG11B10UfloatRenderable) {
        desc.requiredFeatures.push_back(
            DeviceFeature::RG11B10UfloatRenderable);
    } else {
        // Fall back to RGBA16F
    }
}

// Compressed textures for large worlds
if (hasLargeTextures) {
    if (caps.supportsTextureCompressionBC) {
        desc.requiredFeatures.push_back(
            DeviceFeature::TextureCompressionBC);
    } else if (caps.supportsTextureCompressionASTC) {
        desc.requiredFeatures.push_back(
            DeviceFeature::TextureCompressionASTC);
    }
}
```

## Platform Support

### Desktop GPUs (Windows/Linux)
| Feature | NVIDIA | AMD | Intel |
|---------|--------|-----|-------|
| ShaderF16 | ✅ Common | ✅ Common | ⚠️ Varies |
| TextureCompressionBC | ✅ Always | ✅ Always | ✅ Always |
| TextureCompressionETC2 | ❌ Rare | ❌ Rare | ❌ Rare |
| TextureCompressionASTC | ⚠️ Newer | ⚠️ Newer | ⚠️ Newer |
| Depth32FloatStencil8 | ✅ Always | ✅ Always | ✅ Always |
| TimestampQuery | ✅ Always | ✅ Always | ✅ Common |

### Mobile GPUs
| Feature | Apple | Qualcomm | Mali |
|---------|-------|----------|------|
| ShaderF16 | ✅ Common | ✅ Common | ✅ Common |
| TextureCompressionBC | ❌ Never | ❌ Never | ❌ Never |
| TextureCompressionETC2 | ✅ Always | ✅ Always | ✅ Always |
| TextureCompressionASTC | ✅ Always | ✅ Common | ⚠️ Varies |
| Depth32FloatStencil8 | ✅ Common | ✅ Common | ⚠️ Varies |
| TimestampQuery | ✅ Common | ⚠️ Varies | ⚠️ Varies |

## Best Practices

### 1. Feature Detection Strategy
```cpp
class FeatureManager {
public:
    struct Features {
        bool hasDepth32FloatStencil8 = false;
        bool hasTimestamps = false;
        bool hasBC = false;
        bool hasETC2 = false;
        bool hasASTC = false;
    };
    
    Features detect(const PhysicalDeviceCapabilities& caps) {
        Features features;
        features.hasDepth32FloatStencil8 = caps.supportsDepth32FloatStencil8;
        features.hasTimestamps = caps.supportsTimestampQuery;
        features.hasBC = caps.supportsTextureCompressionBC;
        features.hasETC2 = caps.supportsTextureCompressionETC2;
        features.hasASTC = caps.supportsTextureCompressionASTC;
        
        logFeatures(features);
        return features;
    }
    
private:
    void logFeatures(const Features& f) {
        Logger::Instance().LogFormat(LogLevel::Info, "Features", 
            PERS_SOURCE_LOC,
            "Features: D32FS8=%d, Timestamps=%d, BC=%d, ETC2=%d, ASTC=%d",
            f.hasDepth32FloatStencil8, f.hasTimestamps, 
            f.hasBC, f.hasETC2, f.hasASTC);
    }
};
```

### 2. Graceful Degradation
- Always have fallbacks for optional features
- Test without features during development
- Provide quality settings for users
- Log when features are missing

### 3. Feature Groups
Group related features for user settings:
```cpp
enum class QualityLevel {
    Low,     // No optional features
    Medium,  // Basic features (depth32float)
    High,    // + Texture compression
    Ultra    // All available features
};

std::vector<DeviceFeature> getFeaturesForQuality(
    QualityLevel level, 
    const PhysicalDeviceCapabilities& caps) {
    
    std::vector<DeviceFeature> features;
    
    if (level >= QualityLevel::Medium) {
        if (caps.supportsDepth32FloatStencil8) {
            features.push_back(DeviceFeature::Depth32FloatStencil8);
        }
    }
    
    if (level >= QualityLevel::High) {
        if (caps.supportsTextureCompressionBC) {
            features.push_back(DeviceFeature::TextureCompressionBC);
        }
    }
    
    // ... more features for Ultra
    
    return features;
}
```

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Feature not available | Hardware doesn't support | Implement fallback |
| Device creation fails | Requested unavailable feature | Check capabilities first |
| Different behavior across GPUs | Feature availability varies | Test on multiple devices |
| Performance regression | Feature has overhead | Make features optional |

### Debug Logging
```cpp
void logMissingFeatures(
    const std::vector<DeviceFeature>& requested,
    const PhysicalDeviceCapabilities& available) {
    
    for (auto feature : requested) {
        if (!isFeatureAvailable(available, feature)) {
            Logger::Instance().LogFormat(LogLevel::Warning, 
                "Features", PERS_SOURCE_LOC,
                "Feature not available: %s", 
                getFeatureName(feature));
        }
    }
}
```

## See Also
- [Device Creation Guide](device-creation.md)
- [Physical Device API](../../api-reference/graphics/physical-device.md)
- [WebGPU Specification - Features](https://www.w3.org/TR/webgpu/#features)
- [Performance Optimization](../../guides/performance.md)

---

*Last Updated: 2024-08-30*