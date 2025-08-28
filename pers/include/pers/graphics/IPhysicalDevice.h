#pragma once

#include <memory>
#include <vector>
#include <string>

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
    
    bool supportsCompute = false;
    bool supportsRayTracing = false;
    bool supportsTessellation = false;
    
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
 * @brief Logical device descriptor
 */
struct LogicalDeviceDesc {
    bool enableValidation = true;
    std::vector<std::string> requiredExtensions;
    std::vector<QueueFamily> queueFamilies;
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
    virtual bool supportsSurface(void* surface) const = 0;
    
    /**
     * @brief Create a logical device
     * @param desc Logical device descriptor
     * @return Shared pointer to logical device or nullptr if failed
     */
    virtual std::shared_ptr<ILogicalDevice> createLogicalDevice(
        const LogicalDeviceDesc& desc) = 0;
    
    /**
     * @brief Get native handle for backend-specific operations
     * @return Native adapter handle (WGPUAdapter for WebGPU)
     */
    virtual void* getNativeHandle() const = 0;
};

} // namespace pers