#include "graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"
#include "pers/utils/NotImplemented.h"
#include <webgpu.h>
#include <wgpu.h>  // For wgpu-native specific extensions
#include <iostream>

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
    // Surface creation requires platform-specific window system integration.
    // This should be handled by the application layer, not the core library.
    // Applications should create surfaces using platform-specific APIs and
    // pass the native handles directly.
    
    NotImplemented::Log(
        "WebGPUInstance::createSurface",
        "Platform-specific surface creation - should be handled by application layer",
        PERS_SOURCE_LOC
    );
    
    // TODO: Consider accepting a SurfaceDescriptor struct with platform-specific
    // native handles instead of assuming window handle types
    
    return nullptr;
}

} // namespace pers