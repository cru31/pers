#include "graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"
#include "pers/utils/NotImplemented.h"
#include <webgpu.h>
#include <wgpu.h>  // For wgpu-native specific extensions
#include <iostream>
#include <cstring>  // For strlen

#ifdef _WIN32
    #include <windows.h>
#endif

namespace pers {

WebGPUInstance::WebGPUInstance() = default;

WebGPUInstance::~WebGPUInstance() {
    if (_instance) {
        wgpuInstanceRelease(_instance);
        _instance = nullptr;
    }
}

bool WebGPUInstance::initialize(const InstanceDesc& desc) {
    std::cout << "[WebGPUInstance] Initializing for: " << desc.applicationName << std::endl;
    std::cout << "[WebGPUInstance] Engine: " << desc.engineName 
              << " v" << desc.engineVersion << std::endl;
    
    // Store instance configuration for later use
    _desc = desc;
    
    // Set up wgpu-native specific instance extras
    WGPUInstanceExtras extras = {};
    extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
    extras.chain.next = nullptr;
    
    // Configure backend selection based on preferences
    if (desc.allowSoftwareRenderer) {
        // Allow all backends including software fallbacks
        extras.backends = WGPUInstanceBackend_All;
        std::cout << "[WebGPUInstance] Software renderer allowed, using all backends" << std::endl;
    } else {
        // Only use primary hardware-accelerated backends
        extras.backends = WGPUInstanceBackend_Primary;
        std::cout << "[WebGPUInstance] Using primary hardware backends only (Vulkan, Metal, DX12, BrowserWebGPU)" << std::endl;
    }
    
    // Configure instance flags based on InstanceDesc
    extras.flags = WGPUInstanceFlag_Default;
    
    if (desc.enableValidation) {
        extras.flags |= WGPUInstanceFlag_Validation;
        std::cout << "[WebGPUInstance] Validation enabled" << std::endl;
        
        // Debug flag provides more detailed error messages when validation is enabled
        extras.flags |= WGPUInstanceFlag_Debug;
        std::cout << "[WebGPUInstance] Debug mode enabled for detailed validation messages" << std::endl;
    }
    
    // Configure DX12 compiler preference (Windows only)
#ifdef _WIN32
    extras.dx12ShaderCompiler = WGPUDx12Compiler_Dxc;  // Prefer DXC over FXC
    std::cout << "[WebGPUInstance] Using DXC shader compiler for DX12" << std::endl;
#else
    extras.dx12ShaderCompiler = WGPUDx12Compiler_Undefined;
#endif
    
    // GLES configuration for GL backend
    extras.gles3MinorVersion = WGPUGles3MinorVersion_Automatic;
    extras.glFenceBehaviour = WGPUGLFenceBehaviour_Normal;
    
    // No custom paths for DXIL/DXC
    extras.dxilPath = {nullptr, 0};
    extras.dxcPath = {nullptr, 0};
    extras.dxcMaxShaderModel = static_cast<WGPUDxcMaxShaderModel>(0);  // Use default/first value
    
    // Create WebGPU instance descriptor with extras
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&extras);
    
    _instance = wgpuCreateInstance(&instanceDesc);
    if (!_instance) {
        std::cerr << "[WebGPUInstance] Failed to create WGPUInstance" << std::endl;
        return false;
    }
    
    std::cout << "[WebGPUInstance] Created successfully with configured settings" << std::endl;
    
    // Log actual configuration used
    if (desc.enableGPUBasedValidation) {
        std::cout << "[WebGPUInstance] Note: GPU-based validation will be enabled at device creation" << std::endl;
    }
    if (desc.enableSynchronizationValidation) {
        std::cout << "[WebGPUInstance] Note: Synchronization validation will be enabled at device creation" << std::endl;
    }
    
    return true;
}

std::shared_ptr<IPhysicalDevice> WebGPUInstance::requestPhysicalDevice(
    const PhysicalDeviceOptions& options) {
    
    NotImplemented::Log(
        "WebGPUInstance::requestPhysicalDevice",
        "Request WebGPU adapter with options (power preference, force fallback adapter)",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WGPURequestAdapterOptions from PhysicalDeviceOptions
    //    - Map PowerPreference enum to WGPUPowerPreference
    //    - Set forceFallbackAdapter flag
    //    - Use compatibleSurface if provided
    // 2. Call wgpuInstanceRequestAdapter with callback
    // 3. Wait for adapter callback (synchronous wrapper)
    // 4. Apply InstanceDesc preferences:
    //    - Check if adapter is discrete/integrated
    //    - Verify it matches preferHighPerformanceGPU setting
    //    - Check if it's software and allowSoftwareRenderer setting
    // 5. Create and return WebGPUPhysicalDevice wrapping the adapter
    
    return nullptr;
}

void* WebGPUInstance::createSurface(void* windowHandle) {
    if (!_instance) {
        std::cerr << "[WebGPUInstance] Instance not initialized" << std::endl;
        return nullptr;
    }
    
    if (!windowHandle) {
        std::cerr << "[WebGPUInstance] Invalid window handle" << std::endl;
        return nullptr;
    }
    
    WGPUSurface surface = nullptr;
    
#ifdef _WIN32
    std::cout << "[WebGPUInstance] Creating Windows surface..." << std::endl;
    
    // windowHandle should be HWND (native Windows handle)
    HWND hwnd = static_cast<HWND>(windowHandle);
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    
    WGPUSurfaceSourceWindowsHWND surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    surfaceSource.hinstance = hinstance;
    surfaceSource.hwnd = hwnd;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
    
#elif defined(__linux__)
    std::cout << "[WebGPUInstance] Creating Linux/X11 surface..." << std::endl;
    
    // windowHandle should be a struct containing display and window
    // For now, not implemented - app needs to pass proper struct
    NotImplemented::Log(
        "WebGPUInstance::createSurface",
        "Linux X11 surface creation needs proper native handle struct",
        PERS_SOURCE_LOC
    );
    return nullptr;
    
#elif defined(__APPLE__)
    std::cout << "[WebGPUInstance] Creating macOS Metal surface..." << std::endl;
    
    // windowHandle should be CAMetalLayer* (native macOS Metal layer)
    // The application is responsible for creating the CAMetalLayer
    void* metalLayer = windowHandle;
    
    WGPUSurfaceSourceMetalLayer surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    surfaceSource.chain.next = nullptr;
    surfaceSource.layer = metalLayer;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    // WGPUStringView requires data pointer and length
    const char* labelStr = "macOS Metal Surface";
    surfaceDesc.label.data = labelStr;
    surfaceDesc.label.length = strlen(labelStr);
    
    surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
    
#else
    std::cerr << "[WebGPUInstance] Unsupported platform" << std::endl;
    return nullptr;
#endif
    
    if (!surface) {
        std::cerr << "[WebGPUInstance] Failed to create surface" << std::endl;
        return nullptr;
    }
    
    std::cout << "[WebGPUInstance] Surface created successfully" << std::endl;
    return surface;
}

} // namespace pers