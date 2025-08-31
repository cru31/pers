#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/graphics/backends/webgpu/WebGPUPhysicalDevice.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"
#include "pers/utils/TodoOrDie.h"
#include "pers/utils/Logger.h"
#include "pers/core/platform/NativeWindowHandle.h"
#include <webgpu.h>
#include <wgpu.h>  // For wgpu-native specific extensions
#include <cstring>  // For strlen
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __linux__
    #include <X11/Xlib.h>
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
    Logger::Instance().LogFormat(LogLevel::Info, "WebGPUInstance", PERS_SOURCE_LOC,
        "Initializing for: %s", desc.applicationName.c_str());
    Logger::Instance().LogFormat(LogLevel::Info, "WebGPUInstance", PERS_SOURCE_LOC,
        "Engine: %s v%u", desc.engineName.c_str(), desc.engineVersion);
    
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
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Software renderer allowed, using all backends", PERS_SOURCE_LOC);
    } else {
        // Only use primary hardware-accelerated backends
        extras.backends = WGPUInstanceBackend_Primary;
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Using primary hardware backends only (Vulkan, Metal, DX12, BrowserWebGPU)", PERS_SOURCE_LOC);
    }
    
    // Configure instance flags based on InstanceDesc
    extras.flags = WGPUInstanceFlag_Default;
    
    if (desc.enableValidation) {
        extras.flags |= WGPUInstanceFlag_Validation;
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Validation enabled", PERS_SOURCE_LOC);
        
        // Debug flag provides more detailed error messages when validation is enabled
        extras.flags |= WGPUInstanceFlag_Debug;
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Debug mode enabled for detailed validation messages", PERS_SOURCE_LOC);
    }
    
    // Configure DX12 compiler preference (Windows only)
#ifdef _WIN32
    extras.dx12ShaderCompiler = WGPUDx12Compiler_Dxc;  // Prefer DXC over FXC
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
        "Using DXC shader compiler for DX12", PERS_SOURCE_LOC);
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
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance",
            "Failed to create WGPUInstance", PERS_SOURCE_LOC);
        return false;
    }
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
        "Created successfully with configured settings", PERS_SOURCE_LOC);
    
    // Log actual configuration used
    if (desc.enableGPUBasedValidation) {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Note: GPU-based validation will be enabled at device creation", PERS_SOURCE_LOC);
    }
    if (desc.enableSynchronizationValidation) {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Note: Synchronization validation will be enabled at device creation", PERS_SOURCE_LOC);
    }
    
    return true;
}

std::shared_ptr<IPhysicalDevice> WebGPUInstance::requestPhysicalDevice(
    const PhysicalDeviceOptions& options) {
    
    if (!_instance) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance", 
            "Cannot request physical device: instance is null", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Setup adapter options
    WGPURequestAdapterOptions adapterOptions = {};
    
    // Map power preference
    switch (options.powerPreference) {
        case PowerPreference::LowPower:
            adapterOptions.powerPreference = WGPUPowerPreference_LowPower;
            break;
        case PowerPreference::HighPerformance:
            adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
            break;
        case PowerPreference::Default:
        default:
            adapterOptions.powerPreference = WGPUPowerPreference_Undefined;
            break;
    }
    
    // Set compatible surface if provided
    if (options.compatibleSurface.isValid()) {
        adapterOptions.compatibleSurface = options.compatibleSurface.as<WGPUSurface>();
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance", 
            "Requesting adapter with compatible surface", PERS_SOURCE_LOC);
    }
    
    // Force fallback adapter based on options
    adapterOptions.forceFallbackAdapter = options.forceFallbackAdapter;
    
    // Setup callback data with synchronization
    struct CallbackData {
        WGPUAdapter adapter = nullptr;
        bool received = false;
        std::mutex mutex;
        std::condition_variable cv;
    } callbackData;
    
    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
                              WGPUStringView message, void* userdata1, void* userdata2) {
        auto* data = static_cast<CallbackData*>(userdata1);
        
        if (status == WGPURequestAdapterStatus_Success) {
            data->adapter = adapter;
            Logger::Instance().Log(LogLevel::Info, "WebGPUInstance", 
                "Adapter obtained successfully", PERS_SOURCE_LOC);
        } else {
            const char* statusStr = "Unknown";
            switch (status) {
                case WGPURequestAdapterStatus_Unavailable:
                    statusStr = "Unavailable";
                    break;
                case WGPURequestAdapterStatus_Error:
                    statusStr = "Error";
                    break;
                case WGPURequestAdapterStatus_Unknown:
                    statusStr = "Unknown";
                    break;
            }
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUInstance", PERS_SOURCE_LOC,
                "Failed to request adapter: Status=%s, Message=%s", 
                statusStr, message.data ? message.data : "No message");
        }
        
        // Notify waiting thread
        {
            std::lock_guard<std::mutex> lock(data->mutex);
            data->received = true;
        }
        data->cv.notify_one();
    };
    callbackInfo.userdata1 = &callbackData;
    
    // Request adapter
    wgpuInstanceRequestAdapter(_instance, &adapterOptions, callbackInfo);
    
    // Wait for callback with timeout using condition variable
    std::unique_lock<std::mutex> lock(callbackData.mutex);
    
    // Process events in a separate thread to ensure callback is triggered
    std::atomic<bool> stopProcessing{false};
    std::thread eventThread([this, &callbackData, &stopProcessing]() {
        while (!stopProcessing.load()) {
            wgpuInstanceProcessEvents(_instance);
            
            // Check if received (with lock)
            {
                std::lock_guard<std::mutex> lock(callbackData.mutex);
                if (callbackData.received) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    const auto timeout = std::chrono::seconds(5);
    bool success = callbackData.cv.wait_for(lock, timeout, [&callbackData]() {
        return callbackData.received;
    });
    
    // Signal event thread to stop and wait for it
    stopProcessing.store(true);
    if (eventThread.joinable()) {
        eventThread.join();
    }
    
    if (!success) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance", 
            "Timeout waiting for adapter", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    if (!callbackData.adapter) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance", 
            "Failed to get adapter", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    // Check adapter properties against instance preferences
    WGPUAdapterInfo adapterInfo = {};
    wgpuAdapterGetInfo(callbackData.adapter, &adapterInfo);
    
    // Log adapter info
    if (adapterInfo.device.data && adapterInfo.device.length > 0) {
        Logger::Instance().LogFormat(LogLevel::Info, "WebGPUInstance", PERS_SOURCE_LOC,
            "Adapter device: %s", adapterInfo.device.data);
    } else {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Adapter device: (empty/nullptr)", PERS_SOURCE_LOC);
    }
    
    if (adapterInfo.description.data && adapterInfo.description.length > 0) {
        Logger::Instance().LogFormat(LogLevel::Info, "WebGPUInstance", PERS_SOURCE_LOC,
            "Adapter description: %s", adapterInfo.description.data);
    } else {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Adapter description: (empty/nullptr)", PERS_SOURCE_LOC);
    }
    
    // Check if adapter type matches preferences
    bool isSoftware = (adapterInfo.adapterType == WGPUAdapterType_CPU);
    if (isSoftware && !_desc.allowSoftwareRenderer) {
        Logger::Instance().Log(LogLevel::Warning, "WebGPUInstance", 
            "Software adapter obtained but software rendering is disabled", PERS_SOURCE_LOC);
        wgpuAdapterInfoFreeMembers(adapterInfo);  // Free adapter info members
        wgpuAdapterRelease(callbackData.adapter);
        return nullptr;
    }
    
    // Free adapter info members before creating physical device
    wgpuAdapterInfoFreeMembers(adapterInfo);
    
    // Create and return physical device
    auto physicalDevice = std::make_shared<WebGPUPhysicalDevice>(callbackData.adapter);
    
    // We can release our reference as WebGPUPhysicalDevice will add its own
    wgpuAdapterRelease(callbackData.adapter);
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance", 
        "Physical device created successfully", PERS_SOURCE_LOC);
    
    return physicalDevice;
}

NativeSurfaceHandle WebGPUInstance::createSurface(void* windowHandle) {
    if (!_instance) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance",
            "Instance not initialized", PERS_SOURCE_LOC);
        return NativeSurfaceHandle(nullptr);
    }
    
    if (!windowHandle) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance",
            "Invalid window handle", PERS_SOURCE_LOC);
        return NativeSurfaceHandle(nullptr);
    }
    
    // Cast to NativeWindowHandle
    const NativeWindowHandle* nativeHandle = static_cast<const NativeWindowHandle*>(windowHandle);
    
    WGPUSurface surface = nullptr;
    
#ifdef _WIN32
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
        "Creating Windows surface...", PERS_SOURCE_LOC);
    
    // Get HWND from NativeWindowHandle
    HWND hwnd = static_cast<HWND>(nativeHandle->hwnd);
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    
    Logger::Instance().LogFormat(LogLevel::Debug, "WebGPUInstance", PERS_SOURCE_LOC,
        "HWND: %p, HINSTANCE: %p", hwnd, hinstance);
    
    WGPUSurfaceSourceWindowsHWND surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    surfaceSource.chain.next = nullptr;
    surfaceSource.hinstance = hinstance;
    surfaceSource.hwnd = hwnd;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
    
#elif defined(__linux__)
    // Check which Linux windowing system we're using
    if (nativeHandle->type == NativeWindowHandle::X11) {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Creating Linux X11 surface...", PERS_SOURCE_LOC);
        
        Display* display = static_cast<Display*>(nativeHandle->display);
        Window window = reinterpret_cast<Window>(nativeHandle->window);
        
        WGPUSurfaceSourceXlibWindow surfaceSource = {};
        surfaceSource.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        surfaceSource.chain.next = nullptr;
        surfaceSource.display = display;
        surfaceSource.window = window;
        
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
        
        const char* labelStr = "Linux X11 Surface";
        surfaceDesc.label.data = labelStr;
        surfaceDesc.label.length = strlen(labelStr);
        
        surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
        
    } else if (nativeHandle->type == NativeWindowHandle::Wayland) {
        Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
            "Creating Linux Wayland surface...", PERS_SOURCE_LOC);
        
        struct wl_display* display = static_cast<struct wl_display*>(nativeHandle->display);
        struct wl_surface* wlSurface = static_cast<struct wl_surface*>(nativeHandle->window);
        
        WGPUSurfaceSourceWaylandSurface surfaceSource = {};
        surfaceSource.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        surfaceSource.chain.next = nullptr;
        surfaceSource.display = display;
        surfaceSource.surface = wlSurface;
        
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
        
        const char* labelStr = "Linux Wayland Surface";
        surfaceDesc.label.data = labelStr;
        surfaceDesc.label.length = strlen(labelStr);
        
        surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
        
    } else {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUInstance", PERS_SOURCE_LOC,
            "Unknown Linux windowing system type: %d", nativeHandle->type);
        return NativeSurfaceHandle(nullptr);
    }
    
#elif defined(__APPLE__)
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
        "Creating macOS Metal surface...", PERS_SOURCE_LOC);
    
    // Get CAMetalLayer from NativeWindowHandle
    void* metalLayer = nativeHandle->metalLayer;
    
    WGPUSurfaceSourceMetalLayer surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    surfaceSource.chain.next = nullptr;
    surfaceSource.layer = metalLayer;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    const char* labelStr = "macOS Metal Surface";
    surfaceDesc.label.data = labelStr;
    surfaceDesc.label.length = strlen(labelStr);
    
    surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
    
#else
    Logger::Instance().Log(LogLevel::Error, "WebGPUInstance",
        "Unsupported platform", PERS_SOURCE_LOC);
    return NativeSurfaceHandle(nullptr);
#endif
    
    if (!surface) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUInstance",
            "Failed to create surface", PERS_SOURCE_LOC);
        return NativeSurfaceHandle(nullptr);
    }
    
    Logger::Instance().Log(LogLevel::Info, "WebGPUInstance",
        "Surface created successfully", PERS_SOURCE_LOC);
    return NativeSurfaceHandle::fromBackend(surface);
}

} // namespace pers