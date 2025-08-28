# Pers Graphics Engine - User Guide

## Overview

Pers Graphics Engine is a modern, cross-platform graphics abstraction layer that provides a unified interface for multiple graphics APIs including WebGPU, Vulkan, Metal, and D3D12.

## Quick Start

### Basic Usage

```cpp
#include <pers/graphics/backends/IGraphicsBackendFactory.h>
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>

// Create a backend factory
auto factory = std::make_shared<pers::WebGPUBackendFactory>();

// Configure instance descriptor
pers::InstanceDesc desc;
desc.applicationName = "My Application";
desc.engineName = "My Engine";
desc.enableValidation = true;  // Enable validation in development

// Create graphics instance
auto instance = factory->createInstance(desc);
```

## Instance Configuration

### InstanceDesc Structure

The `InstanceDesc` structure provides configuration options that work across all graphics backends:

```cpp
struct InstanceDesc {
    // Application info (used by all APIs for debugging/profiling)
    std::string applicationName = "Pers Application";
    uint32_t applicationVersion = 1;
    std::string engineName = "Pers Graphics Engine";
    uint32_t engineVersion = 1;
    
    // Debugging and validation
    bool enableValidation = true;
    bool enableGPUBasedValidation = false;
    bool enableSynchronizationValidation = false;
    
    // Performance and features
    bool preferHighPerformanceGPU = true;
    bool allowSoftwareRenderer = false;
    
    // Required extensions/features (API-specific)
    std::vector<std::string> requiredExtensions;
    std::vector<std::string> optionalExtensions;
    
    // API version hints
    uint32_t apiVersionMajor = 0; // 0 = use latest
    uint32_t apiVersionMinor = 0;
    uint32_t apiVersionPatch = 0;
};
```

### Configuration Options Explained

#### Application Information
- **applicationName/Version**: Identifies your application in debugging tools
- **engineName/Version**: Identifies the engine version for profiling

#### Validation Settings
- **enableValidation**: Enables API validation layers
  - Vulkan: Validation layers
  - D3D12: Debug layer
  - WebGPU: Validation
  - Metal: Validation layer
  
- **enableGPUBasedValidation**: Enables GPU-assisted validation for deeper checks
  - More thorough but slower validation
  - Catches issues like uninitialized memory access
  
- **enableSynchronizationValidation**: Validates thread safety
  - Detects race conditions
  - Validates command buffer usage across threads

#### Performance Settings
- **preferHighPerformanceGPU**: Prefers discrete GPU over integrated
  - Automatically selects the most powerful GPU
  - Falls back to integrated if discrete unavailable
  
- **allowSoftwareRenderer**: Allows CPU-based rendering fallback
  - Vulkan: SwiftShader/LavaPipe
  - D3D12: WARP
  - WebGPU: Fallback adapter
  - Useful for CI/testing environments

#### Extensions
- **requiredExtensions**: Extensions that must be available
  - Application will fail to initialize if not supported
  
- **optionalExtensions**: Extensions used if available
  - Application continues without them if unsupported

## Surface Creation

### Platform-Specific Window Integration

The engine provides a `createSurface()` method that handles platform-specific surface creation:

```cpp
// Assuming you have a GLFW window
GLFWwindow* window = /* your window */;

// Create surface for the window
void* surface = instance->createSurface(window);
```

#### Supported Platforms
- **Windows**: Uses Win32 HWND
- **Linux**: Uses X11 Window
- **macOS**: Uses CAMetalLayer (currently requires manual setup)

#### Important Notes
- Surface creation requires the instance to be initialized first
- The window handle must be valid and from a supported windowing system
- Currently, GLFW is the primary supported windowing library

### Example: Complete Initialization

```cpp
#include <GLFW/glfw3.h>
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }
    
    // Create window (don't create OpenGL context)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Pers App", nullptr, nullptr);
    
    // Create graphics backend
    auto factory = std::make_shared<pers::WebGPUBackendFactory>();
    
    // Configure instance
    pers::InstanceDesc desc;
    desc.applicationName = "My Graphics App";
    desc.enableValidation = true;
    #ifdef NDEBUG
    desc.enableValidation = false;  // Disable in release builds
    #endif
    
    // Create instance
    auto instance = factory->createInstance(desc);
    if (!instance) {
        std::cerr << "Failed to create graphics instance" << std::endl;
        return -1;
    }
    
    // Create surface
    void* surface = instance->createSurface(window);
    if (!surface) {
        std::cerr << "Failed to create surface" << std::endl;
        return -1;
    }
    
    // Continue with physical device selection, etc.
    
    return 0;
}
```

## Physical Device Selection

### Requesting a Physical Device

```cpp
// Configure device selection
PhysicalDeviceOptions options;
options.powerPreference = PowerPreference::HighPerformance;
options.forceFallbackAdapter = false;

// Request adapter
auto physicalDevice = instance->requestPhysicalDevice(options);
```

### Power Preferences
- **HighPerformance**: Prefer discrete GPUs
- **LowPower**: Prefer integrated GPUs for battery life
- **Default**: Let the system choose

## Best Practices

### Development vs Production

```cpp
pers::InstanceDesc createInstanceDesc(bool isDebugBuild) {
    pers::InstanceDesc desc;
    desc.applicationName = "MyApp";
    
    if (isDebugBuild) {
        desc.enableValidation = true;
        desc.enableGPUBasedValidation = true;
        desc.enableSynchronizationValidation = true;
    } else {
        desc.enableValidation = false;
        desc.preferHighPerformanceGPU = true;
    }
    
    return desc;
}
```

### Error Handling

Always check return values:

```cpp
auto instance = factory->createInstance(desc);
if (!instance) {
    // Handle initialization failure
    std::cerr << "Failed to create instance" << std::endl;
    return false;
}

void* surface = instance->createSurface(window);
if (!surface) {
    // Handle surface creation failure
    std::cerr << "Failed to create surface" << std::endl;
    return false;
}
```

## Backend-Specific Notes

### WebGPU
- Uses wgpu-native implementation
- Supports Vulkan, Metal, D3D12, and OpenGL backends
- Validation is lightweight compared to native APIs

### Vulkan (Future)
- Requires Vulkan SDK for validation layers
- Supports extensive validation options
- Best debugging capabilities with RenderDoc

### D3D12 (Future)
- Windows-only
- Requires Windows SDK
- PIX for detailed profiling

### Metal (Future)
- macOS/iOS only
- Requires Xcode for debugging
- Metal Frame Capture for profiling

## Troubleshooting

### Common Issues

1. **"Failed to create instance"**
   - Check if graphics drivers are installed
   - Verify the graphics backend is available on your platform
   - Try enabling `allowSoftwareRenderer`

2. **"Failed to create surface"**
   - Ensure window was created without OpenGL context
   - Check platform-specific requirements
   - Verify window handle is valid

3. **Validation errors**
   - Enable validation to get detailed error messages
   - Check the console output for specific issues
   - Use GPU-based validation for memory issues

### Debug Output

Enable detailed logging:

```cpp
desc.enableValidation = true;  // Basic validation
desc.enableGPUBasedValidation = true;  // Deep validation
desc.enableSynchronizationValidation = true;  // Thread safety
```

## API Reference

See the [API Documentation](API_REFERENCE.md) for detailed class and method descriptions.

## Examples

Check the `samples/` directory for complete examples:
- `pers_triangle`: Basic triangle rendering
- `loadGlTF`: Loading and rendering 3D models
- More examples coming soon...