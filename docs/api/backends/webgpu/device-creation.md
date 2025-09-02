# WebGPU Device Creation Guide

> **Status**: Stable
> **Since**: v1.0.0-alpha
> **Backend**: WebGPU

## Overview

This guide covers the complete process of creating and managing WebGPU devices in the Pers Graphics Engine. Device creation follows a three-step hierarchy: Instance → Physical Device → Logical Device.

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

### Device Hierarchy
1. **Instance**: Entry point to the graphics system
2. **Physical Device**: Represents a GPU adapter
3. **Logical Device**: Provides access to GPU resources

### Adapter Selection
The WebGPU backend automatically selects the best available adapter based on:
- Power preference settings
- Surface compatibility requirements
- Adapter type (discrete > integrated > software)
- Available features and limits

### Asynchronous Operations
WebGPU device creation is inherently asynchronous. The Pers engine handles this internally using:
- Synchronous wrapper APIs for ease of use
- Internal callbacks with mutex protection
- Configurable timeouts for all async operations

## API Reference

### Classes/Interfaces

#### `WebGPUInstance`
Entry point for WebGPU graphics initialization.

**Header**: `pers/graphics/backends/webgpu/WebGPUInstance.h`

### Methods

#### `requestPhysicalDevice`
```cpp
std::shared_ptr<IPhysicalDevice> requestPhysicalDevice(
    const PhysicalDeviceOptions& options = {}) const;
```

**Parameters:**
- `options`: Selection criteria for physical device
  - `powerPreference`: HighPerformance, LowPower, or Default
  - `forceFallbackAdapter`: Force software renderer
  - `compatibleSurface`: Surface for compatibility checking

**Returns:**
- Shared pointer to physical device, or nullptr if none found

**Thread Safety:**
- Thread-safe, can be called from any thread

**Example:**
```cpp
// Request high-performance GPU compatible with our surface
PhysicalDeviceOptions options;
options.powerPreference = PowerPreference::HighPerformance;
options.compatibleSurface = surface;

auto physicalDevice = instance->requestPhysicalDevice(options);
if (!physicalDevice) {
    // Handle no compatible adapter
    return false;
}
```

#### `supportsSurface`
```cpp
bool supportsSurface(const NativeSurfaceHandle& surface) const;
```

**Parameters:**
- `surface`: Native surface handle to check

**Returns:**
- true if surface can be presented to, false otherwise

**Thread Safety:**
- Thread-safe

**Example:**
```cpp
// Check if adapter supports our window surface
if (!physicalDevice->supportsSurface(surface)) {
    LOG_ERROR("Device", 
        "Adapter doesn't support surface");
    return false;
}
```

#### `createLogicalDevice`
```cpp
std::shared_ptr<ILogicalDevice> createLogicalDevice(
    const LogicalDeviceDesc& desc);
```

**Parameters:**
- `desc`: Device configuration
  - `enableValidation`: Enable debug/validation layers
  - `debugName`: Optional debug name
  - `requiredFeatures`: Features that must be supported
  - `timeout`: Creation timeout (default: 5 seconds)

**Returns:**
- Shared pointer to logical device, or nullptr if creation failed

**Exceptions:**
- None (returns nullptr on failure)

**Thread Safety:**
- Thread-safe

**Example:**
```cpp
// Create logical device with validation and specific features
LogicalDeviceDesc desc;
desc.enableValidation = true;
desc.debugName = "MainDevice";
desc.requiredFeatures = {
    DeviceFeature::Depth32FloatStencil8,
    DeviceFeature::TimestampQuery
};

auto logicalDevice = physicalDevice->createLogicalDevice(desc);
if (!logicalDevice) {
    // Handle device creation failure
    return false;
}
```

## Usage

### Basic Usage
```cpp
// Step 1: Create graphics factory
auto factory = pers::createGraphicsFactory(BackendType::WebGPU);

// Step 2: Create instance
InstanceDesc instanceDesc;
instanceDesc.applicationName = "MyApp";
instanceDesc.enableValidation = true;

auto instance = factory->createInstance(instanceDesc);
if (!instance) {
    LOG_ERROR("Init", 
        "Failed to create instance");
    return false;
}

// Step 3: Request physical device
PhysicalDeviceOptions physicalOptions;
physicalOptions.powerPreference = PowerPreference::HighPerformance;
physicalOptions.compatibleSurface = surface;

auto physicalDevice = instance->requestPhysicalDevice(physicalOptions);
if (!physicalDevice) {
    LOG_ERROR("Init", 
        "No compatible adapter found");
    return false;
}

// Step 4: Query capabilities
auto capabilities = physicalDevice->getCapabilities();
Logger::Instance().LogFormat(LogLevel::Info, "Init", PERS_SOURCE_LOC,
    "Using GPU: %s", capabilities.deviceName.c_str());

// Step 5: Create logical device
LogicalDeviceDesc logicalDesc;
logicalDesc.enableValidation = true;

// Only request features we need and that are available
if (capabilities.supportsDepth32FloatStencil8) {
    logicalDesc.requiredFeatures.push_back(
        DeviceFeature::Depth32FloatStencil8);
}

auto logicalDevice = physicalDevice->createLogicalDevice(logicalDesc);
if (!logicalDevice) {
    LOG_ERROR("Init", 
        "Failed to create logical device");
    return false;
}

// Step 6: Get queue for command submission
auto queue = logicalDevice->getQueue();
```

### Advanced Usage

#### Feature Detection Pattern
```cpp
// Query what's available before requesting
auto caps = physicalDevice->getCapabilities();

LogicalDeviceDesc desc;
std::vector<DeviceFeature> wantedFeatures = {
    DeviceFeature::TimestampQuery,
    DeviceFeature::Depth32FloatStencil8,
    DeviceFeature::TextureCompressionBC
};

for (auto feature : wantedFeatures) {
    bool isSupported = false;
    
    switch (feature) {
    case DeviceFeature::TimestampQuery:
        isSupported = caps.supportsTimestampQuery;
        break;
    case DeviceFeature::Depth32FloatStencil8:
        isSupported = caps.supportsDepth32FloatStencil8;
        break;
    case DeviceFeature::TextureCompressionBC:
        isSupported = caps.supportsTextureCompressionBC;
        break;
    }
    
    if (isSupported) {
        desc.requiredFeatures.push_back(feature);
    } else {
        Logger::Instance().LogFormat(LogLevel::Warning, "Init", 
            PERS_SOURCE_LOC, "Feature not available: %d", 
            static_cast<int>(feature));
    }
}

auto device = physicalDevice->createLogicalDevice(desc);
```

#### Fallback to Software Renderer
```cpp
// Try hardware first, fall back to software
PhysicalDeviceOptions options;
options.powerPreference = PowerPreference::HighPerformance;

auto physicalDevice = instance->requestPhysicalDevice(options);
if (!physicalDevice) {
    LOG_WARNING("Init", 
        "No hardware adapter, trying software");
    
    // Enable software renderer in instance
    InstanceDesc softwareDesc = instanceDesc;
    softwareDesc.allowSoftwareRenderer = true;
    auto softwareInstance = factory->createInstance(softwareDesc);
    
    // Request software adapter
    options.forceFallbackAdapter = true;
    physicalDevice = softwareInstance->requestPhysicalDevice(options);
}
```

## Platform Notes

### Windows
- Direct3D 12 backend requires Windows 10 1903+
- WARP software renderer always available
- Discrete GPUs preferred over integrated

### Linux
- Vulkan backend requires drivers with Vulkan 1.1+
- May fall back to OpenGL ES 3.0
- Check for vulkan-icd-loader package

### macOS
- Metal backend requires macOS 10.15+
- Automatic support for Apple Silicon
- No software renderer fallback

## Limitations

### Current Limitations
- **Async Nature**: All operations internally async, wrapped synchronously
- **Single Adapter**: Cannot enumerate all adapters, only best match
- **Feature Set**: Limited to WebGPU standard features
- **Timeout**: Default 5-second timeout may be too short for some systems

### Not Supported
- Direct adapter enumeration
- Multi-adapter configurations
- Custom device extensions
- Direct memory allocation control

## Performance

### Performance Characteristics
- Instance creation: O(1), ~10-50ms
- Physical device request: O(n) adapters, ~50-200ms
- Logical device creation: O(1), ~100-500ms
- All times include async callback overhead

### Optimization Tips
- Create instance once at application start
- Cache physical device selection
- Reuse logical device for all resources
- Avoid creating multiple devices unless necessary

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Instance creation fails | Missing graphics drivers | Update GPU drivers |
| No compatible adapter | Requirements too strict | Relax requirements or enable software |
| Surface not supported | Wrong adapter selected | Ensure surface compatibility in options |
| Device creation timeout | Slow system or drivers | Increase timeout in LogicalDeviceDesc |
| Validation errors | Debug layers not available | Install graphics SDK or disable validation |

### Debug Checklist
1. Enable validation in InstanceDesc
2. Check Logger output for warnings
3. Verify surface creation before device
4. Test with software renderer
5. Update graphics drivers
6. Check WebGPU/wgpu-native version

## Version History
| Version | Changes | Migration Notes |
|---------|---------|-----------------|
| v1.0.0-alpha | Initial implementation | - |

## See Also
- [WebGPU Instance API](../../api-reference/graphics/webgpu-instance.md)
- [Physical Device API](../../api-reference/graphics/physical-device.md)
- [Logical Device API](../../api-reference/graphics/logical-device.md)
- [Surface Management](surface-management.md)
- [Feature Detection](feature-detection.md)

---

*Last Updated: 2024-08-30*