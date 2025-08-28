# Graphics Backend Factory Interface Design

## Overview

The Graphics Backend Factory provides an abstraction layer for creating graphics API objects (Device, Buffer, Pipeline, etc.) without the engine having any direct dependency on specific graphics APIs like WebGPU, Vulkan, or D3D12.

## Architecture

```
┌─────────────────┐          ┌──────────────────────────────────┐
│   Application   │          │         Pers Engine              │
│                 │          │                                  │
│                 │  selects │  ┌────────────────────────────┐ │
│                 │──────────┼─►│    Backend Selection       │ │
│                 │ backend  │  │  - WebGPUBackendFactory    │ │
│                 │          │  │  - VulkanBackendFactory    │ │
│                 │          │  │  - MetalBackendFactory     │ │
│                 │          │  └────────────────────────────┘ │
│                 │          │            │                    │
│                 │          │            ▼                    │
│                 │          │  ┌────────────────────────────┐ │
│                 │          │  │    Renderer                │ │
│                 │          │  │  (uses IGraphicsBackend    │ │
│                 │          │  │   Factory interface)       │ │
│                 │          │  └────────────────────────────┘ │
└─────────────────┘          └──────────────────────────────────┘
```

## Core Interfaces

### IGraphicsBackendFactory

```cpp
namespace pers {
namespace renderer {

// Forward declarations for abstracted graphics types
class IInstance;
class ILogicalDevice;
class IPhysicalDevice;
class IQueue;
class ICommandEncoder;
class IRenderPipeline;
class IComputePipeline;
class IBuffer;
class ITexture;
class IBindGroup;
class IBindGroupLayout;
class IPipelineLayout;
class IShaderModule;
class ISwapChain;
class ISampler;
class IQuerySet;

// Main factory interface - Engine provides concrete implementations
class IGraphicsBackendFactory {
public:
    virtual ~IGraphicsBackendFactory() = default;
    
    // Create graphics instance (entry point for graphics API)
    virtual std::shared_ptr<IInstance> CreateInstance(
        const InstanceDescriptor& desc) = 0;
    
    // Get backend information
    virtual GraphicsBackendType GetBackendType() const = 0;
    virtual BackendCapabilities GetCapabilities() const = 0;
    virtual std::string GetBackendVersion() const = 0;
    virtual std::string GetBackendName() const = 0;
};

// Graphics backend types
enum class GraphicsBackendType {
    WebGPU,
    Vulkan,
    Metal,
    D3D12,
    D3D11,
    OpenGL,
    OpenGLES
};

// Backend capabilities
struct BackendCapabilities {
    // Feature support
    bool supportsComputeShaders = false;
    bool supportsGeometryShaders = false;
    bool supportsTessellation = false;
    bool supportsRaytracing = false;
    bool supportsMultiDrawIndirect = false;
    bool supportsTimestampQueries = false;
    bool supportsOcclusionQueries = false;
    bool supportsPipelineStatistics = false;
    
    // Limits
    uint32_t maxTextureSize1D = 0;
    uint32_t maxTextureSize2D = 0;
    uint32_t maxTextureSize3D = 0;
    uint32_t maxTextureArrayLayers = 0;
    uint32_t maxBindGroups = 0;
    uint32_t maxBindingsPerBindGroup = 0;
    uint32_t maxUniformBufferSize = 0;
    uint32_t maxStorageBufferSize = 0;
    uint32_t maxVertexAttributes = 0;
    uint32_t maxVertexBuffers = 0;
    uint32_t maxColorAttachments = 0;
    uint32_t maxComputeWorkgroupSizeX = 0;
    uint32_t maxComputeWorkgroupSizeY = 0;
    uint32_t maxComputeWorkgroupSizeZ = 0;
};

} // namespace renderer
} // namespace pers
```

### IInstance

```cpp
class IInstance {
public:
    virtual ~IInstance() = default;
    
    // Request physical device based on requirements
    // surfaceHandle is an opaque handle from ISurfaceProvider
    virtual std::shared_ptr<IPhysicalDevice> RequestPhysicalDevice(
        const PhysicalDeviceOptions& options,
        void* compatibleSurfaceHandle = nullptr) = 0;
    
    // Enumerate all available physical devices
    virtual std::vector<std::shared_ptr<IPhysicalDevice>> EnumeratePhysicalDevices() = 0;
    
    // Instance properties
    virtual bool HasExtension(const std::string& extension) const = 0;
    virtual std::vector<std::string> GetEnabledExtensions() const = 0;
};

// Physical device request options
struct PhysicalDeviceOptions {
    enum class PowerPreference {
        LowPower,
        HighPerformance,
        Default
    };
    
    PowerPreference powerPreference = PowerPreference::Default;
    bool forceFallbackAdapter = false;
    bool requireDedicatedGPU = false;
};
```

### IPhysicalDevice

```cpp
class IPhysicalDevice {
public:
    virtual ~IPhysicalDevice() = default;
    
    // Create logical device from this physical device
    virtual std::shared_ptr<ILogicalDevice> CreateLogicalDevice(
        const LogicalDeviceDescriptor& desc) = 0;
    
    // Get physical device properties
    virtual PhysicalDeviceInfo GetInfo() const = 0;
    virtual PhysicalDeviceLimits GetLimits() const = 0;
    virtual PhysicalDeviceFeatures GetFeatures() const = 0;
    
    // Query surface support (opaque handle from ISurfaceProvider)
    virtual bool IsSurfaceSupported(void* surfaceHandle) const = 0;
    virtual std::vector<TextureFormat> GetSurfacePreferredFormats(
        void* surfaceHandle) const = 0;
};

// Physical device information
struct PhysicalDeviceInfo {
    std::string vendor;
    std::string device;
    std::string driver;
    PhysicalDeviceType type;
    uint32_t vendorID;
    uint32_t deviceID;
    uint64_t dedicatedVideoMemory;
    uint64_t dedicatedSystemMemory;
    uint64_t sharedSystemMemory;
};

enum class PhysicalDeviceType {
    DiscreteGPU,
    IntegratedGPU,
    VirtualGPU,
    CPU,
    Unknown
};
```

### ILogicalDevice

```cpp
class ILogicalDevice {
public:
    virtual ~ILogicalDevice() = default;
    
    // Resource creation methods
    virtual std::shared_ptr<IBuffer> CreateBuffer(
        const BufferDescriptor& desc) = 0;
    
    virtual std::shared_ptr<ITexture> CreateTexture(
        const TextureDescriptor& desc) = 0;
    
    virtual std::shared_ptr<ISampler> CreateSampler(
        const SamplerDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IShaderModule> CreateShaderModule(
        const ShaderModuleDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IBindGroupLayout> CreateBindGroupLayout(
        const BindGroupLayoutDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IPipelineLayout> CreatePipelineLayout(
        const PipelineLayoutDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IRenderPipeline> CreateRenderPipeline(
        const RenderPipelineDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IComputePipeline> CreateComputePipeline(
        const ComputePipelineDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IBindGroup> CreateBindGroup(
        const BindGroupDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IQuerySet> CreateQuerySet(
        const QuerySetDescriptor& desc) = 0;
    
    // Swap chain creation (surface handle from ISurfaceProvider)
    virtual std::shared_ptr<ISwapChain> CreateSwapChain(
        void* surfaceHandle,
        const SwapChainDescriptor& desc) = 0;
    
    // Command submission
    virtual std::shared_ptr<ICommandEncoder> CreateCommandEncoder(
        const CommandEncoderDescriptor& desc) = 0;
    
    virtual std::shared_ptr<IQueue> GetQueue() = 0;
    
    // Device operations
    virtual void WaitIdle() = 0;
    virtual DeviceInfo GetInfo() const = 0;
    virtual bool HasFeature(const std::string& feature) const = 0;
    
    // Error handling
    virtual void SetUncapturedErrorCallback(
        std::function<void(ErrorType, const std::string&)> callback) = 0;
    virtual void SetDeviceLostCallback(
        std::function<void(DeviceLostReason, const std::string&)> callback) = 0;
};
```

## Engine-Side Backend Implementations

### WebGPU Backend Factory

```cpp
// In engine code (pers/src/graphics/backends/webgpu/)
namespace pers {
namespace renderer {
namespace backends {
namespace webgpu {

class WebGPUBackendFactory final : public IGraphicsBackendFactory {
public:
    WebGPUBackendFactory() {
        // Initialize WebGPU-specific setup
    }
    
    std::shared_ptr<IInstance> CreateInstance(
        const InstanceDescriptor& desc) override {
        
        WGPUInstanceDescriptor wgpuDesc{};
        wgpuDesc.nextInChain = nullptr;
        
        // Setup instance extensions based on descriptor
        std::vector<const char*> enabledToggles;
        if (desc.enableValidation) {
            enabledToggles.push_back("enable_immediate_error_handling");
        }
        
        WGPUDawnTogglesDescriptor togglesDesc{};
        if (!enabledToggles.empty()) {
            togglesDesc.chain.sType = WGPUSType_DawnTogglesDescriptor;
            togglesDesc.enabledToggles = enabledToggles.data();
            togglesDesc.enabledToggleCount = enabledToggles.size();
            wgpuDesc.nextInChain = &togglesDesc.chain;
        }
        
        WGPUInstance instance = wgpuCreateInstance(&wgpuDesc);
        return std::make_shared<WebGPUInstance>(instance);
    }
    
    GraphicsBackendType GetBackendType() const override {
        return GraphicsBackendType::WebGPU;
    }
    
    BackendCapabilities GetCapabilities() const override {
        BackendCapabilities caps{};
        
        // WebGPU capabilities
        caps.supportsComputeShaders = true;
        caps.supportsGeometryShaders = false;  // Not in WebGPU
        caps.supportsTessellation = false;     // Not in WebGPU
        caps.supportsRaytracing = false;       // Not yet
        caps.supportsMultiDrawIndirect = true;
        caps.supportsTimestampQueries = true;
        caps.supportsOcclusionQueries = true;
        
        // WebGPU limits
        caps.maxTextureSize1D = 8192;
        caps.maxTextureSize2D = 8192;
        caps.maxTextureSize3D = 2048;
        caps.maxTextureArrayLayers = 256;
        caps.maxBindGroups = 4;
        caps.maxBindingsPerBindGroup = 1000;
        caps.maxUniformBufferSize = 65536;
        caps.maxStorageBufferSize = 134217728; // 128MB
        caps.maxVertexAttributes = 16;
        caps.maxVertexBuffers = 8;
        caps.maxColorAttachments = 8;
        caps.maxComputeWorkgroupSizeX = 256;
        caps.maxComputeWorkgroupSizeY = 256;
        caps.maxComputeWorkgroupSizeZ = 64;
        
        return caps;
    }
    
    std::string GetBackendVersion() const override {
        // Could query actual version from wgpu-native
        return "wgpu-native 0.19.0";
    }
    
    std::string GetBackendName() const override {
        return "WebGPU";
    }
};

} // namespace webgpu
} // namespace backends
} // namespace renderer
} // namespace pers
```

### WebGPU Wrapper Classes

```cpp
// In engine code (pers/src/graphics/backends/webgpu/)
namespace pers {
namespace renderer {
namespace backends {
namespace webgpu {

// WebGPU Instance wrapper
class WebGPUInstance final : public IInstance {
private:
    WGPUInstance _instance;
    
public:
    explicit WebGPUInstance(WGPUInstance instance) 
        : _instance(instance) {}
    
    ~WebGPUInstance() {
        if (_instance) {
            wgpuInstanceRelease(_instance);
        }
    }
    
    std::shared_ptr<IPhysicalDevice> RequestPhysicalDevice(
        const PhysicalDeviceOptions& options,
        void* compatibleSurfaceHandle) override {
        
        WGPURequestAdapterOptions wgpuOptions{};
        wgpuOptions.compatibleSurface = static_cast<WGPUSurface>(compatibleSurfaceHandle);
        wgpuOptions.powerPreference = ConvertPowerPreference(options.powerPreference);
        wgpuOptions.forceFallbackAdapter = options.forceFallbackAdapter;
        
        struct CallbackData {
            WGPUAdapter adapter = nullptr;
            bool completed = false;
        } callbackData;
        
        auto callback = [](WGPURequestAdapterStatus status,
                          WGPUAdapter adapter,
                          const char* message,
                          void* userdata) {
            auto* data = static_cast<CallbackData*>(userdata);
            if (status == WGPURequestAdapterStatus_Success) {
                data->adapter = adapter;
            }
            data->completed = true;
        };
        
        wgpuInstanceRequestAdapter(_instance, &wgpuOptions, callback, &callbackData);
        
        // Wait for async completion (simplified)
        while (!callbackData.completed) {
            // In real implementation, use proper async handling
        }
        
        if (callbackData.adapter) {
            return std::make_shared<WebGPUPhysicalDevice>(callbackData.adapter);
        }
        
        return nullptr;
    }
    
    std::vector<std::shared_ptr<IPhysicalDevice>> EnumeratePhysicalDevices() override {
        // WebGPU doesn't have direct enumeration, request with different preferences
        std::vector<std::shared_ptr<IPhysicalDevice>> devices;
        
        // Try high performance
        PhysicalDeviceOptions highPerf{};
        highPerf.powerPreference = PhysicalDeviceOptions::PowerPreference::HighPerformance;
        if (auto device = RequestPhysicalDevice(highPerf, nullptr)) {
            devices.push_back(device);
        }
        
        // Try low power
        PhysicalDeviceOptions lowPower{};
        lowPower.powerPreference = PhysicalDeviceOptions::PowerPreference::LowPower;
        if (auto device = RequestPhysicalDevice(lowPower, nullptr)) {
            devices.push_back(device);
        }
        
        return devices;
    }
    
    bool HasExtension(const std::string& extension) const override {
        // Check WebGPU instance extensions
        return false;  // Simplified
    }
    
    std::vector<std::string> GetEnabledExtensions() const override {
        return {};  // Simplified
    }
    
    // Expose native handle for surface creation (used by surface provider)
    WGPUInstance GetNativeHandle() const { return _instance; }
    
private:
    static WGPUPowerPreference ConvertPowerPreference(
        PhysicalDeviceOptions::PowerPreference pref) {
        switch (pref) {
            case PhysicalDeviceOptions::PowerPreference::LowPower:
                return WGPUPowerPreference_LowPower;
            case PhysicalDeviceOptions::PowerPreference::HighPerformance:
                return WGPUPowerPreference_HighPerformance;
            default:
                return WGPUPowerPreference_Undefined;
        }
    }
};

} // namespace webgpu
} // namespace backends
} // namespace renderer
} // namespace pers
```

## Engine-Side Factory Creation Functions

```cpp
namespace pers {
namespace renderer {

// Factory creation functions provided by the engine
std::shared_ptr<IGraphicsBackendFactory> CreateWebGPUBackendFactory() {
    return std::make_shared<backends::webgpu::WebGPUBackendFactory>();
}

std::shared_ptr<IGraphicsBackendFactory> CreateVulkanBackendFactory() {
    #ifdef PERS_VULKAN_BACKEND
        return std::make_shared<backends::vulkan::VulkanBackendFactory>();
    #else
        throw std::runtime_error("Vulkan backend not compiled in");
    #endif
}

std::shared_ptr<IGraphicsBackendFactory> CreateMetalBackendFactory() {
    #ifdef PERS_METAL_BACKEND
        return std::make_shared<backends::metal::MetalBackendFactory>();
    #else
        throw std::runtime_error("Metal backend not compiled in");
    #endif
}

std::shared_ptr<IGraphicsBackendFactory> CreateD3D12BackendFactory() {
    #ifdef PERS_D3D12_BACKEND
        return std::make_shared<backends::d3d12::D3D12BackendFactory>();
    #else
        throw std::runtime_error("D3D12 backend not compiled in");
    #endif
}

} // namespace renderer
} // namespace pers
```

## Engine-Side Usage

```cpp
namespace pers {
namespace renderer {

class Renderer {
private:
    // Graphics backend - no knowledge of specific API
    std::shared_ptr<IGraphicsBackendFactory> _graphicsFactory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _device;
    std::shared_ptr<IQueue> _queue;
    std::shared_ptr<ISwapChain> _swapChain;
    
    // Surface handle from ISurfaceProvider (separate interface)
    void* _surfaceHandle = nullptr;
    
public:
    Renderer(std::shared_ptr<IGraphicsBackendFactory> factory)
        : _graphicsFactory(factory) {
        InitializeGraphicsBackend();
    }
    
private:
    void InitializeGraphicsBackend() {
        // 1. Create instance through factory
        InstanceDescriptor instanceDesc{};
        instanceDesc.enableValidation = true;
        instanceDesc.applicationName = "Pers Engine";
        _instance = _graphicsFactory->CreateInstance(instanceDesc);
        
        if (!_instance) {
            throw std::runtime_error("Failed to create graphics instance");
        }
        
        // 2. Request physical device (surface handle will be set separately)
        PhysicalDeviceOptions deviceOpts{};
        deviceOpts.powerPreference = PhysicalDeviceOptions::PowerPreference::HighPerformance;
        _physicalDevice = _instance->RequestPhysicalDevice(deviceOpts, _surfaceHandle);
        
        if (!_physicalDevice) {
            throw std::runtime_error("Failed to request physical device");
        }
        
        // 3. Log physical device info
        auto info = _physicalDevice->GetInfo();
        Logger::Info("Physical device: {} ({})", info.device, info.vendor);
        Logger::Info("Driver: {}", info.driver);
        Logger::Info("VRAM: {} MB", info.dedicatedVideoMemory / (1024 * 1024));
        
        // 4. Create logical device
        LogicalDeviceDescriptor deviceDesc{};
        deviceDesc.label = "Main Device";
        deviceDesc.requiredFeatures = {"texture-compression-bc"};
        _device = _physicalDevice->CreateLogicalDevice(deviceDesc);
        
        if (!_device) {
            throw std::runtime_error("Failed to create logical device");
        }
        
        // 5. Get queue
        _queue = _device->GetQueue();
        
        // 6. Set error callbacks
        _device->SetUncapturedErrorCallback(
            [](ErrorType type, const std::string& message) {
                Logger::Error("Graphics error: {}", message);
            });
        
        _device->SetDeviceLostCallback(
            [](DeviceLostReason reason, const std::string& message) {
                Logger::Error("Device lost: {}", message);
            });
    }
    
public:
    // Set surface for swap chain creation (called after surface is created)
    void SetSurface(void* surfaceHandle, uint32_t width, uint32_t height) {
        _surfaceHandle = surfaceHandle;
        
        // Create swap chain
        SwapChainDescriptor swapChainDesc{};
        swapChainDesc.width = width;
        swapChainDesc.height = height;
        swapChainDesc.format = TextureFormat::BGRA8UnormSrgb;
        swapChainDesc.usage = TextureUsage::RenderAttachment;
        swapChainDesc.presentMode = PresentMode::Fifo;
        
        _swapChain = _device->CreateSwapChain(_surfaceHandle, swapChainDesc);
    }
    
    // Rendering methods use only abstract interfaces
    void RenderFrame() {
        if (!_swapChain) return;
        
        // Get next texture from swap chain
        auto texture = _swapChain->GetCurrentTexture();
        if (!texture) {
            // Handle swap chain error
            return;
        }
        
        // Create command encoder
        CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "Frame Command Encoder";
        auto commandEncoder = _device->CreateCommandEncoder(encoderDesc);
        
        // Begin render pass
        RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachments.resize(1);
        renderPassDesc.colorAttachments[0].view = texture->CreateView({});
        renderPassDesc.colorAttachments[0].loadOp = LoadOp::Clear;
        renderPassDesc.colorAttachments[0].storeOp = StoreOp::Store;
        renderPassDesc.colorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        
        auto renderPass = commandEncoder->BeginRenderPass(renderPassDesc);
        
        // ... Draw commands ...
        
        renderPass->End();
        
        // Finish and submit
        auto commandBuffer = commandEncoder->Finish({});
        _queue->Submit({commandBuffer});
        
        // Present
        _swapChain->Present();
    }
};

} // namespace renderer
} // namespace pers
```

## Key Design Principles

1. **Pure Graphics Abstraction**: Only handles graphics API objects, no window system knowledge
2. **Opaque Surface Handles**: Surface is passed as `void*` from separate interface
3. **Factory Pattern**: Engine provides concrete implementations, application selects which to use
4. **No API Leakage**: Engine never sees WebGPU/Vulkan/D3D12 types
5. **Capability Query**: Engine can query backend capabilities for feature detection

## Benefits

- Complete isolation from graphics API specifics
- Easy to add new graphics backends (Vulkan, D3D12, Metal)
- Engine code remains clean and API-agnostic
- Testable with mock implementations
- Clear ownership and lifetime management with shared_ptr