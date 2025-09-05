#include "pers/graphics/backends/webgpu/WebGPUPhysicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>

namespace pers {

// RAII wrapper for WGPUSupportedFeatures
struct SupportedFeaturesGuard {
    WGPUSupportedFeatures features;
    
    SupportedFeaturesGuard() {
        features.features = nullptr;
        features.featureCount = 0;
    }
    
    ~SupportedFeaturesGuard() {
        if (features.features) {
            wgpuSupportedFeaturesFreeMembers(features);
        }
    }
    // Delete copy/move to prevent double-free
    SupportedFeaturesGuard(const SupportedFeaturesGuard&) = delete;
    SupportedFeaturesGuard& operator=(const SupportedFeaturesGuard&) = delete;
    SupportedFeaturesGuard(SupportedFeaturesGuard&&) = delete;
    SupportedFeaturesGuard& operator=(SupportedFeaturesGuard&&) = delete;
};

// Static feature mapping table
static const std::unordered_map<DeviceFeature, WGPUFeatureName> g_featureMap = {
    {DeviceFeature::DepthClipControl, WGPUFeatureName_DepthClipControl},
    {DeviceFeature::Depth32FloatStencil8, WGPUFeatureName_Depth32FloatStencil8},
    {DeviceFeature::TimestampQuery, WGPUFeatureName_TimestampQuery},
    // PipelineStatisticsQuery is not available in current WebGPU spec
    {DeviceFeature::TextureCompressionBC, WGPUFeatureName_TextureCompressionBC},
    {DeviceFeature::TextureCompressionETC2, WGPUFeatureName_TextureCompressionETC2},
    {DeviceFeature::TextureCompressionASTC, WGPUFeatureName_TextureCompressionASTC},
    {DeviceFeature::IndirectFirstInstance, WGPUFeatureName_IndirectFirstInstance},
    {DeviceFeature::ShaderF16, WGPUFeatureName_ShaderF16},
    {DeviceFeature::RG11B10UfloatRenderable, WGPUFeatureName_RG11B10UfloatRenderable},
    {DeviceFeature::BGRA8UnormStorage, WGPUFeatureName_BGRA8UnormStorage},
    {DeviceFeature::Float32Filterable, WGPUFeatureName_Float32Filterable}
};

// Helper function to convert DeviceLimits to WGPULimits
static WGPULimits convertToWGPULimits(const DeviceLimits& limits) {
    WGPULimits wgpuLimits = {};
    wgpuLimits.maxTextureDimension1D = limits.maxTextureDimension1D;
    wgpuLimits.maxTextureDimension2D = limits.maxTextureDimension2D;
    wgpuLimits.maxTextureDimension3D = limits.maxTextureDimension3D;
    wgpuLimits.maxTextureArrayLayers = limits.maxTextureArrayLayers;
    wgpuLimits.maxBindGroups = limits.maxBindGroups;
    wgpuLimits.maxBindingsPerBindGroup = limits.maxBindingsPerBindGroup;
    wgpuLimits.maxDynamicUniformBuffersPerPipelineLayout = limits.maxDynamicUniformBuffersPerPipelineLayout;
    wgpuLimits.maxDynamicStorageBuffersPerPipelineLayout = limits.maxDynamicStorageBuffersPerPipelineLayout;
    wgpuLimits.maxSampledTexturesPerShaderStage = limits.maxSampledTexturesPerShaderStage;
    wgpuLimits.maxSamplersPerShaderStage = limits.maxSamplersPerShaderStage;
    wgpuLimits.maxStorageBuffersPerShaderStage = limits.maxStorageBuffersPerShaderStage;
    wgpuLimits.maxStorageTexturesPerShaderStage = limits.maxStorageTexturesPerShaderStage;
    wgpuLimits.maxUniformBuffersPerShaderStage = limits.maxUniformBuffersPerShaderStage;
    wgpuLimits.maxUniformBufferBindingSize = limits.maxUniformBufferBindingSize;
    wgpuLimits.maxStorageBufferBindingSize = limits.maxStorageBufferBindingSize;
    wgpuLimits.maxVertexBuffers = limits.maxVertexBuffers;
    wgpuLimits.maxVertexAttributes = limits.maxVertexAttributes;
    wgpuLimits.maxVertexBufferArrayStride = limits.maxVertexBufferArrayStride;
    wgpuLimits.maxInterStageShaderVariables = limits.maxInterStageShaderVariables;
    wgpuLimits.maxComputeWorkgroupStorageSize = limits.maxComputeWorkgroupStorageSize;
    wgpuLimits.maxComputeInvocationsPerWorkgroup = limits.maxComputeInvocationsPerWorkgroup;
    wgpuLimits.maxComputeWorkgroupSizeX = limits.maxComputeWorkgroupSizeX;
    wgpuLimits.maxComputeWorkgroupSizeY = limits.maxComputeWorkgroupSizeY;
    wgpuLimits.maxComputeWorkgroupSizeZ = limits.maxComputeWorkgroupSizeZ;
    wgpuLimits.maxComputeWorkgroupsPerDimension = limits.maxComputeWorkgroupsPerDimension;
    return wgpuLimits;
}

WebGPUPhysicalDevice::WebGPUPhysicalDevice(WGPUAdapter adapter)
    : _adapter(adapter) {
    
    if (_adapter) {
        wgpuAdapterAddRef(_adapter);
        queryAdapterInfo(); // Query info once during construction
        LOG_INFO("WebGPUPhysicalDevice", 
            "Created with adapter");
    }
}

WebGPUPhysicalDevice::~WebGPUPhysicalDevice() {
    if (_adapter) {
        // Only free adapter info if it was successfully queried
        if (_adapterInfoValid) {
            wgpuAdapterInfoFreeMembers(_adapterInfo);
        }
        wgpuAdapterRelease(_adapter);
        _adapter = nullptr;
    }
}

void WebGPUPhysicalDevice::queryAdapterInfo() {
    if (!_adapter) {
        return;
    }
    
    WGPUStatus status = wgpuAdapterGetInfo(_adapter, &_adapterInfo);
    if (status == WGPUStatus_Success) {
        _adapterInfoValid = true;
        LOG_INFO("WebGPUPhysicalDevice", 
            "Adapter info queried successfully");
    } else {
        _adapterInfoValid = false;
        LOG_ERROR("WebGPUPhysicalDevice", 
            "Failed to get adapter info");
    }
}

PhysicalDeviceCapabilities WebGPUPhysicalDevice::getCapabilities() const {
    std::lock_guard<std::mutex> lock(_cacheMutex);
    
    // Return cached capabilities if available
    if (_cachedCapabilities.has_value()) {
        return *_cachedCapabilities;
    }
    
    // Query and cache capabilities
    _cachedCapabilities = queryCapabilities();
    return *_cachedCapabilities;
}

PhysicalDeviceCapabilities WebGPUPhysicalDevice::queryCapabilities() const {
    PhysicalDeviceCapabilities caps = {};
    
    if (!_adapter) {
        return caps;
    }
    
    // Fill basic info from adapter (already queried in constructor)
    // WGPUStringView has data and length members
    if (_adapterInfoValid) {
        if (_adapterInfo.device.data) {
            caps.deviceName = std::string(_adapterInfo.device.data, _adapterInfo.device.length);
        }
        if (_adapterInfo.description.data) {
            caps.driverInfo = std::string(_adapterInfo.description.data, _adapterInfo.description.length);
        }
    }
    
    // Query limits
    WGPULimits limits = {};
    WGPUStatus status = wgpuAdapterGetLimits(_adapter, &limits);
    if (status == WGPUStatus_Success) {
        caps.maxTextureSize2D = limits.maxTextureDimension2D;
        caps.maxTextureSize3D = limits.maxTextureDimension3D;
        caps.maxTextureLayers = limits.maxTextureArrayLayers;
    }
    
    // Query features using RAII wrapper
    SupportedFeaturesGuard featuresGuard;
    wgpuAdapterGetFeatures(_adapter, &featuresGuard.features);
    
    // WebGPU always supports compute
    caps.supportsCompute = true;
    
    // Features not supported in WebGPU (keeping for API compatibility)
    caps.supportsRayTracing = false;     // WebGPU doesn't have ray tracing support
    caps.supportsTessellation = false;   // WebGPU doesn't have tessellation support
    caps.supportsPipelineStatisticsQuery = false; // Not in current WebGPU spec
    
    if (featuresGuard.features.featureCount > 0 && featuresGuard.features.features) {
        for (size_t i = 0; i < featuresGuard.features.featureCount; ++i) {
            WGPUFeatureName feature = featuresGuard.features.features[i];
            
            // Map WebGPU features to capabilities
            switch (feature) {
                case WGPUFeatureName_ShaderF16:
                    caps.supportsShaderF16 = true;
                    break;
                case WGPUFeatureName_DepthClipControl:
                    caps.supportsDepthClipControl = true;
                    break;
                case WGPUFeatureName_Depth32FloatStencil8:
                    caps.supportsDepth32FloatStencil8 = true;
                    break;
                case WGPUFeatureName_TimestampQuery:
                    caps.supportsTimestampQuery = true;
                    break;
                case WGPUFeatureName_TextureCompressionBC:
                    caps.supportsTextureCompressionBC = true;
                    break;
                case WGPUFeatureName_TextureCompressionETC2:
                    caps.supportsTextureCompressionETC2 = true;
                    break;
                case WGPUFeatureName_TextureCompressionASTC:
                    caps.supportsTextureCompressionASTC = true;
                    break;
                case WGPUFeatureName_IndirectFirstInstance:
                    caps.supportsIndirectFirstInstance = true;
                    break;
                case WGPUFeatureName_RG11B10UfloatRenderable:
                    caps.supportsRG11B10UfloatRenderable = true;
                    break;
                case WGPUFeatureName_BGRA8UnormStorage:
                    caps.supportsBGRA8UnormStorage = true;
                    break;
                case WGPUFeatureName_Float32Filterable:
                    caps.supportsFloat32Filterable = true;
                    break;
                default:
                    // Unknown or unsupported feature - log for debugging
                    Logger::Instance().LogFormat(LogLevel::Debug, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
                        "Unknown or unmapped feature detected: %d", static_cast<int>(feature));
                    break;
            }
        }
    }
    
    // RAII wrapper automatically cleans up features
    
    // WebGPU doesn't directly expose memory info
    // These would need platform-specific queries
    
    return caps;
}

std::vector<QueueFamily> WebGPUPhysicalDevice::getQueueFamilies() const {
    std::lock_guard<std::mutex> lock(_cacheMutex);
    
    // Return cached queue families if available
    if (_cachedQueueFamilies.has_value()) {
        return *_cachedQueueFamilies;
    }
    
    std::vector<QueueFamily> families;
    
    // WebGPU has a simplified queue model
    // All queues support all operations
    QueueFamily family;
    family.index = 0;
    family.queueCount = 1;
    family.supportsGraphics = true;
    family.supportsCompute = true;
    family.supportsTransfer = true;
    family.supportsSparse = false; // WebGPU doesn't support sparse resources
    
    families.push_back(family);
    
    // Cache the result
    _cachedQueueFamilies = families;
    
    return families;
}

bool WebGPUPhysicalDevice::supportsSurface(const NativeSurfaceHandle& surface) const {
    if (!_adapter || !surface.isValid()) {
        return false;
    }
    
    WGPUSurface wgpuSurface = surface.as<WGPUSurface>();
    if (!wgpuSurface) {
        LOG_WARNING("WebGPUPhysicalDevice",
            "Invalid surface handle");
        return false;
    }
    
    // Try to get surface capabilities to properly check support
    WGPUSurfaceCapabilities capabilities = {};
    wgpuSurfaceGetCapabilities(wgpuSurface, _adapter, &capabilities);
    
    // Check if we got valid capabilities
    bool isSupported = false;
    if (capabilities.formatCount > 0 && capabilities.formats != nullptr) {
        // Surface is supported if we have at least one compatible format
        isSupported = true;
        
        Logger::Instance().LogFormat(LogLevel::Debug, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
            "Surface supported with %zu formats, %zu present modes", 
            capabilities.formatCount, capabilities.presentModeCount);
        
        // Free the allocated arrays
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    } else {
        // No formats available means surface is not compatible
        LOG_WARNING("WebGPUPhysicalDevice",
            "Surface not supported by this adapter (no compatible formats)");
    }
    
    return isSupported;
}

std::shared_ptr<ILogicalDevice> WebGPUPhysicalDevice::createLogicalDevice(
    const LogicalDeviceDesc& desc) {
    
    if (!_adapter) {
        LOG_ERROR("WebGPUPhysicalDevice", 
            "Cannot create logical device: adapter is null");
        return nullptr;
    }
    
    // Validate and prepare features
    std::vector<WGPUFeatureName> requiredFeatures;
    if (!desc.requiredFeatures.empty()) {
        if (!checkFeatureSupport(desc.requiredFeatures, requiredFeatures)) {
            LOG_ERROR("WebGPUPhysicalDevice",
                "One or more required features are not supported");
            return nullptr;
        }
    }
    
    // Setup device descriptor
    WGPUDeviceDescriptor deviceDesc = {};
    
    // Set device label if provided (keep string alive for async callback)
    std::string labelCopy;
    if (!desc.debugName.empty()) {
        labelCopy = desc.debugName;
        deviceDesc.label.data = labelCopy.c_str();
        deviceDesc.label.length = labelCopy.length();
    }
    
    // Set required features
    if (!requiredFeatures.empty()) {
        deviceDesc.requiredFeatures = requiredFeatures.data();
        deviceDesc.requiredFeatureCount = requiredFeatures.size();
    }
    
    // Setup required limits - always start with adapter defaults
    WGPULimits requiredLimits = {};
    WGPUStatus status = wgpuAdapterGetLimits(_adapter, &requiredLimits);
    if (status != WGPUStatus_Success) {
        LOG_ERROR("WebGPUPhysicalDevice",
            "Failed to query adapter limits");
        return nullptr;
    }
    
    if (desc.requiredLimits) {
        // Override specific limits with user-specified values
        // Only override non-zero values to keep adapter defaults for unspecified fields
        if (desc.requiredLimits->maxTextureDimension1D > 0) 
            requiredLimits.maxTextureDimension1D = desc.requiredLimits->maxTextureDimension1D;
        if (desc.requiredLimits->maxTextureDimension2D > 0) 
            requiredLimits.maxTextureDimension2D = desc.requiredLimits->maxTextureDimension2D;
        if (desc.requiredLimits->maxTextureDimension3D > 0) 
            requiredLimits.maxTextureDimension3D = desc.requiredLimits->maxTextureDimension3D;
        if (desc.requiredLimits->maxTextureArrayLayers > 0) 
            requiredLimits.maxTextureArrayLayers = desc.requiredLimits->maxTextureArrayLayers;
        if (desc.requiredLimits->maxBindGroups > 0) 
            requiredLimits.maxBindGroups = desc.requiredLimits->maxBindGroups;
        if (desc.requiredLimits->maxBindingsPerBindGroup > 0) 
            requiredLimits.maxBindingsPerBindGroup = desc.requiredLimits->maxBindingsPerBindGroup;
        if (desc.requiredLimits->maxDynamicUniformBuffersPerPipelineLayout > 0) 
            requiredLimits.maxDynamicUniformBuffersPerPipelineLayout = desc.requiredLimits->maxDynamicUniformBuffersPerPipelineLayout;
        if (desc.requiredLimits->maxDynamicStorageBuffersPerPipelineLayout > 0) 
            requiredLimits.maxDynamicStorageBuffersPerPipelineLayout = desc.requiredLimits->maxDynamicStorageBuffersPerPipelineLayout;
        if (desc.requiredLimits->maxSampledTexturesPerShaderStage > 0) 
            requiredLimits.maxSampledTexturesPerShaderStage = desc.requiredLimits->maxSampledTexturesPerShaderStage;
        if (desc.requiredLimits->maxSamplersPerShaderStage > 0) 
            requiredLimits.maxSamplersPerShaderStage = desc.requiredLimits->maxSamplersPerShaderStage;
        if (desc.requiredLimits->maxStorageBuffersPerShaderStage > 0) 
            requiredLimits.maxStorageBuffersPerShaderStage = desc.requiredLimits->maxStorageBuffersPerShaderStage;
        if (desc.requiredLimits->maxStorageTexturesPerShaderStage > 0) 
            requiredLimits.maxStorageTexturesPerShaderStage = desc.requiredLimits->maxStorageTexturesPerShaderStage;
        if (desc.requiredLimits->maxUniformBuffersPerShaderStage > 0) 
            requiredLimits.maxUniformBuffersPerShaderStage = desc.requiredLimits->maxUniformBuffersPerShaderStage;
        if (desc.requiredLimits->maxUniformBufferBindingSize > 0) 
            requiredLimits.maxUniformBufferBindingSize = desc.requiredLimits->maxUniformBufferBindingSize;
        if (desc.requiredLimits->maxStorageBufferBindingSize > 0) 
            requiredLimits.maxStorageBufferBindingSize = desc.requiredLimits->maxStorageBufferBindingSize;
        if (desc.requiredLimits->maxVertexBuffers > 0) 
            requiredLimits.maxVertexBuffers = desc.requiredLimits->maxVertexBuffers;
        if (desc.requiredLimits->maxVertexAttributes > 0) 
            requiredLimits.maxVertexAttributes = desc.requiredLimits->maxVertexAttributes;
        if (desc.requiredLimits->maxVertexBufferArrayStride > 0) 
            requiredLimits.maxVertexBufferArrayStride = desc.requiredLimits->maxVertexBufferArrayStride;
        if (desc.requiredLimits->maxInterStageShaderVariables > 0) 
            requiredLimits.maxInterStageShaderVariables = desc.requiredLimits->maxInterStageShaderVariables;
        if (desc.requiredLimits->maxComputeWorkgroupStorageSize > 0) 
            requiredLimits.maxComputeWorkgroupStorageSize = desc.requiredLimits->maxComputeWorkgroupStorageSize;
        if (desc.requiredLimits->maxComputeInvocationsPerWorkgroup > 0) 
            requiredLimits.maxComputeInvocationsPerWorkgroup = desc.requiredLimits->maxComputeInvocationsPerWorkgroup;
        if (desc.requiredLimits->maxComputeWorkgroupSizeX > 0) 
            requiredLimits.maxComputeWorkgroupSizeX = desc.requiredLimits->maxComputeWorkgroupSizeX;
        if (desc.requiredLimits->maxComputeWorkgroupSizeY > 0) 
            requiredLimits.maxComputeWorkgroupSizeY = desc.requiredLimits->maxComputeWorkgroupSizeY;
        if (desc.requiredLimits->maxComputeWorkgroupSizeZ > 0) 
            requiredLimits.maxComputeWorkgroupSizeZ = desc.requiredLimits->maxComputeWorkgroupSizeZ;
        if (desc.requiredLimits->maxComputeWorkgroupsPerDimension > 0) 
            requiredLimits.maxComputeWorkgroupsPerDimension = desc.requiredLimits->maxComputeWorkgroupsPerDimension;
        
        LOG_INFO("WebGPUPhysicalDevice", 
            "Using adapter defaults with user-specified overrides");
    } else {
        LOG_INFO("WebGPUPhysicalDevice", 
            "Using adapter's default limits");
    }
    
    deviceDesc.requiredLimits = &requiredLimits;
    
    // Setup uncaptured error callback
    deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const * device, WGPUErrorType type, 
                                                         WGPUStringView message, void* userdata1, void* userdata2) {
        const char* errorTypeStr = "Unknown";
        switch (type) {
            case WGPUErrorType_NoError:
                errorTypeStr = "NoError";
                break;
            case WGPUErrorType_Validation:
                errorTypeStr = "Validation";
                break;
            case WGPUErrorType_OutOfMemory:
                errorTypeStr = "OutOfMemory";
                break;
            case WGPUErrorType_Internal:
                errorTypeStr = "Internal";
                break;
            case WGPUErrorType_Unknown:
                errorTypeStr = "Unknown";
                break;
            // Note: DeviceLost is handled separately via deviceLostCallback
        }
        
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUDevice", PERS_SOURCE_LOC,
            "Uncaptured device error: Type=%s, Message=%s", 
            errorTypeStr, message.data ? message.data : "No message");
    };
    
    // Setup device lost callback
    deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, 
                                                    WGPUStringView message, void* userdata1, void* userdata2) {
        const char* reasonStr = "Unknown";
        switch (reason) {
            case WGPUDeviceLostReason_Unknown:
                reasonStr = "Unknown";
                break;
            case WGPUDeviceLostReason_Destroyed:
                reasonStr = "Destroyed";
                break;
            case WGPUDeviceLostReason_InstanceDropped:
                reasonStr = "InstanceDropped";
                break;
            case WGPUDeviceLostReason_FailedCreation:
                reasonStr = "FailedCreation";
                break;
        }
        
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUDevice", PERS_SOURCE_LOC,
            "Device lost: Reason=%s, Message=%s",
            reasonStr, message.data ? message.data : "No message");
    };
    
    // Setup callback data with proper synchronization
    struct CallbackData {
        WGPUDevice device = nullptr;
        WGPURequestDeviceStatus status = WGPURequestDeviceStatus_Unknown;
        std::string errorMessage;
        std::mutex mutex;
        std::condition_variable cv;
        bool received = false;
    } callbackData;
    
    WGPURequestDeviceCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device,
                               WGPUStringView message, void* userdata1, void* userdata2) {
        auto* data = static_cast<CallbackData*>(userdata1);
        
        {
            std::lock_guard<std::mutex> lock(data->mutex);
            data->status = status;
            data->device = device;
            
            if (status == WGPURequestDeviceStatus_Success) {
                LOG_INFO("WebGPUPhysicalDevice",
                    "Device obtained successfully");
            } else {
                data->errorMessage = message.data ? std::string(message.data, message.length) : "Unknown error";
                const char* statusStr = "Unknown";
                switch (status) {
                    case WGPURequestDeviceStatus_Error:
                        statusStr = "Error";
                        break;
                    case WGPURequestDeviceStatus_Unknown:
                        statusStr = "Unknown";
                        break;
                }
                Logger::Instance().LogFormat(LogLevel::Error, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
                    "Failed to request device: Status=%s, Message=%s",
                    statusStr, data->errorMessage.c_str());
            }
            data->received = true;
        }
        data->cv.notify_one();
    };
    callbackInfo.userdata1 = &callbackData;
    
    // Request device
    wgpuAdapterRequestDevice(_adapter, &deviceDesc, callbackInfo);
    
    // Wait for callback with user-specified timeout
    std::unique_lock<std::mutex> lock(callbackData.mutex);
    
    bool success = callbackData.cv.wait_for(lock, desc.timeout, 
        [&callbackData] { return callbackData.received; });
    
    if (!success) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
            "Timeout waiting for device (timeout: %lld ms)", desc.timeout.count());
        return nullptr;
    }
    
    if (callbackData.status != WGPURequestDeviceStatus_Success) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
            "Failed to create device: %s", callbackData.errorMessage.c_str());
        // Clean up device if it was created despite error
        if (callbackData.device) {
            wgpuDeviceRelease(callbackData.device);
        }
        return nullptr;
    }
    
    if (!callbackData.device) {
        LOG_ERROR("WebGPUPhysicalDevice",
            "Device creation succeeded but device is null");
        return nullptr;
    }
    
    // Create and return logical device
    auto logicalDevice = std::make_shared<WebGPULogicalDevice>(callbackData.device, _adapter);
    
    // Release our reference as WebGPULogicalDevice will add its own
    wgpuDeviceRelease(callbackData.device);
    
    LOG_INFO("WebGPUPhysicalDevice",
        "Logical device created successfully");
    
    return logicalDevice;
}

NativeAdapterHandle WebGPUPhysicalDevice::getNativeAdapterHandle() const {
    return NativeAdapterHandle::fromBackend(_adapter);
}

bool WebGPUPhysicalDevice::validateLimitsWithinCapability(
    const DeviceLimits& requested, const WGPULimits& available) const {
    
    // Check if requested limits are within available limits
    if (requested.maxTextureDimension1D > available.maxTextureDimension1D ||
        requested.maxTextureDimension2D > available.maxTextureDimension2D ||
        requested.maxTextureDimension3D > available.maxTextureDimension3D ||
        requested.maxTextureArrayLayers > available.maxTextureArrayLayers ||
        requested.maxBindGroups > available.maxBindGroups ||
        requested.maxBindingsPerBindGroup > available.maxBindingsPerBindGroup ||
        requested.maxDynamicUniformBuffersPerPipelineLayout > available.maxDynamicUniformBuffersPerPipelineLayout ||
        requested.maxDynamicStorageBuffersPerPipelineLayout > available.maxDynamicStorageBuffersPerPipelineLayout ||
        requested.maxSampledTexturesPerShaderStage > available.maxSampledTexturesPerShaderStage ||
        requested.maxSamplersPerShaderStage > available.maxSamplersPerShaderStage ||
        requested.maxStorageBuffersPerShaderStage > available.maxStorageBuffersPerShaderStage ||
        requested.maxStorageTexturesPerShaderStage > available.maxStorageTexturesPerShaderStage ||
        requested.maxUniformBuffersPerShaderStage > available.maxUniformBuffersPerShaderStage ||
        requested.maxUniformBufferBindingSize > available.maxUniformBufferBindingSize ||
        requested.maxStorageBufferBindingSize > available.maxStorageBufferBindingSize ||
        requested.maxVertexBuffers > available.maxVertexBuffers ||
        requested.maxVertexAttributes > available.maxVertexAttributes ||
        requested.maxVertexBufferArrayStride > available.maxVertexBufferArrayStride ||
        requested.maxInterStageShaderVariables > available.maxInterStageShaderVariables ||
        requested.maxComputeWorkgroupStorageSize > available.maxComputeWorkgroupStorageSize ||
        requested.maxComputeInvocationsPerWorkgroup > available.maxComputeInvocationsPerWorkgroup ||
        requested.maxComputeWorkgroupSizeX > available.maxComputeWorkgroupSizeX ||
        requested.maxComputeWorkgroupSizeY > available.maxComputeWorkgroupSizeY ||
        requested.maxComputeWorkgroupSizeZ > available.maxComputeWorkgroupSizeZ ||
        requested.maxComputeWorkgroupsPerDimension > available.maxComputeWorkgroupsPerDimension) {
        
        return false;
    }
    
    return true;
}

bool WebGPUPhysicalDevice::checkFeatureSupport(
    const std::vector<DeviceFeature>& requiredFeatures,
    std::vector<WGPUFeatureName>& outWGPUFeatures) const {
    
    if (!_adapter) {
        return false;
    }
    
    // Query supported features
    SupportedFeaturesGuard supportedGuard;
    wgpuAdapterGetFeatures(_adapter, &supportedGuard.features);
    
    // Build set of supported features for fast lookup
    std::unordered_set<WGPUFeatureName> supportedSet;
    if (supportedGuard.features.features) {
        for (size_t i = 0; i < supportedGuard.features.featureCount; ++i) {
            supportedSet.insert(supportedGuard.features.features[i]);
        }
    }
    
    // Check each required feature
    outWGPUFeatures.clear();
    for (const auto& feature : requiredFeatures) {
        auto it = g_featureMap.find(feature);
        if (it == g_featureMap.end()) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
                "Unknown device feature requested: %d", static_cast<int>(feature));
            return false;
        }
        
        WGPUFeatureName wgpuFeature = it->second;
        if (supportedSet.find(wgpuFeature) == supportedSet.end()) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUPhysicalDevice", PERS_SOURCE_LOC,
                "Feature not supported by adapter: %d", static_cast<int>(feature));
            return false;
        }
        
        outWGPUFeatures.push_back(wgpuFeature);
    }
    
    return true;
}

} // namespace pers