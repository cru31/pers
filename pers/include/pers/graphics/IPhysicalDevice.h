#pragma once

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

class ILogicalDevice;

/**
 * @brief Physical device (adapter) capabilities
 */
struct PhysicalDeviceCapabilities {
    std::string deviceName;
    std::string driverInfo;
    uint64_t dedicatedVideoMemory = 0;
    uint64_t dedicatedSystemMemory = 0;
    uint64_t sharedSystemMemory = 0;
    
    // Core capabilities
    bool supportsCompute = false;
    bool supportsRayTracing = false;      // Note: Not supported in WebGPU, always false
    bool supportsTessellation = false;    // Note: Not supported in WebGPU, always false
    
    // Shader capabilities
    bool supportsShaderF16 = false;  // 16-bit float support in shaders
    
    // Texture capabilities
    bool supportsTextureCompressionBC = false;   // BC1-BC7 compression
    bool supportsTextureCompressionETC2 = false; // ETC2/EAC compression
    bool supportsTextureCompressionASTC = false; // ASTC compression
    
    // Rendering capabilities
    bool supportsDepth32FloatStencil8 = false;   // 32-bit float depth with 8-bit stencil
    bool supportsDepthClipControl = false;       // Depth clip control
    bool supportsRG11B10UfloatRenderable = false; // RG11B10 format renderable
    bool supportsBGRA8UnormStorage = false;      // BGRA8 storage support
    bool supportsFloat32Filterable = false;      // 32-bit float filtering
    
    // Query capabilities
    bool supportsTimestampQuery = false;         // GPU timestamp queries
    bool supportsPipelineStatisticsQuery = false; // Pipeline statistics queries (not in WebGPU spec yet)
    bool supportsIndirectFirstInstance = false;  // First instance in indirect draw
    
    // Limits
    uint32_t maxTextureSize2D = 0;
    uint32_t maxTextureSize3D = 0;
    uint32_t maxTextureLayers = 0;
};

/**
 * @brief Queue family properties
 */
struct QueueFamily {
    uint32_t index = 0;
    uint32_t queueCount = 0;
    bool supportsGraphics = false;
    bool supportsCompute = false;
    bool supportsTransfer = false;
    bool supportsSparse = false;
};

/**
 * @brief Device features that can be requested
 */
enum class DeviceFeature {
    DepthClipControl,
    Depth32FloatStencil8,
    TimestampQuery,
    PipelineStatisticsQuery,
    TextureCompressionBC,
    TextureCompressionETC2,
    TextureCompressionASTC,
    IndirectFirstInstance,
    ShaderF16,
    RG11B10UfloatRenderable,
    BGRA8UnormStorage,
    Float32Filterable
};

/**
 * @brief Device resource limits
 */
struct DeviceLimits {
    uint32_t maxTextureDimension1D = 0;
    uint32_t maxTextureDimension2D = 0;
    uint32_t maxTextureDimension3D = 0;
    uint32_t maxTextureArrayLayers = 0;
    uint32_t maxBindGroups = 0;
    uint32_t maxBindingsPerBindGroup = 0;
    uint32_t maxDynamicUniformBuffersPerPipelineLayout = 0;
    uint32_t maxDynamicStorageBuffersPerPipelineLayout = 0;
    uint32_t maxSampledTexturesPerShaderStage = 0;
    uint32_t maxSamplersPerShaderStage = 0;
    uint32_t maxStorageBuffersPerShaderStage = 0;
    uint32_t maxStorageTexturesPerShaderStage = 0;
    uint32_t maxUniformBuffersPerShaderStage = 0;
    uint32_t maxUniformBufferBindingSize = 0;
    uint32_t maxStorageBufferBindingSize = 0;
    uint32_t maxVertexBuffers = 0;
    uint32_t maxVertexAttributes = 0;
    uint32_t maxVertexBufferArrayStride = 0;
    uint32_t maxInterStageShaderVariables = 0;
    uint32_t maxComputeWorkgroupStorageSize = 0;
    uint32_t maxComputeInvocationsPerWorkgroup = 0;
    uint32_t maxComputeWorkgroupSizeX = 0;
    uint32_t maxComputeWorkgroupSizeY = 0;
    uint32_t maxComputeWorkgroupSizeZ = 0;
    uint32_t maxComputeWorkgroupsPerDimension = 0;
};

/**
 * @brief Logical device descriptor
 */
struct LogicalDeviceDesc {
    bool enableValidation = true;
    std::string debugName;  // Optional debug name for the device
    std::vector<DeviceFeature> requiredFeatures;
    std::vector<std::string> requiredExtensions;  // Backend-specific extensions
    std::vector<QueueFamily> queueFamilies;
    
    // Optional: If not provided, adapter's default limits will be used
    std::shared_ptr<DeviceLimits> requiredLimits;
    
    // Optional: Timeout for device creation (default: 5 seconds)
    std::chrono::milliseconds timeout = std::chrono::seconds(5);
};

/**
 * @brief Physical device interface (GPU adapter)
 */
class IPhysicalDevice {
public:
    virtual ~IPhysicalDevice() = default;
    
    /**
     * @brief Get device capabilities
     * @return Device capabilities structure
     */
    virtual PhysicalDeviceCapabilities getCapabilities() const = 0;
    
    /**
     * @brief Get available queue families
     * @return Vector of queue families
     */
    virtual std::vector<QueueFamily> getQueueFamilies() const = 0;
    
    /**
     * @brief Check if device supports surface presentation
     * @param surface Surface handle to check
     * @return true if surface is supported
     */
    virtual bool supportsSurface(const NativeSurfaceHandle& surface) const = 0;
    
    /**
     * @brief Create a logical device
     * @param desc Logical device descriptor
     * @return Shared pointer to logical device or nullptr if failed
     */
    virtual std::shared_ptr<ILogicalDevice> createLogicalDevice(
        const LogicalDeviceDesc& desc) = 0;
    
    /**
     * @brief Get native adapter handle for backend-specific operations
     * @return Native adapter handle (WGPUAdapter for WebGPU)
     */
    virtual NativeAdapterHandle getNativeAdapterHandle() const = 0;
};

} // namespace pers