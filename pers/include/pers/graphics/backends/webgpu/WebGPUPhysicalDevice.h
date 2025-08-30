#pragma once

#include "pers/graphics/IPhysicalDevice.h"
#include <webgpu.h>
#include <optional>
#include <mutex>

namespace pers {

/**
 * @brief WebGPU implementation of IPhysicalDevice
 * 
 * This class manages a WebGPU adapter and follows RAII principles.
 * Copy operations are deleted to prevent double-free issues.
 * Move operations are deleted as this class is meant to be used via shared_ptr.
 */
class WebGPUPhysicalDevice : public IPhysicalDevice {
public:
    explicit WebGPUPhysicalDevice(WGPUAdapter adapter);
    ~WebGPUPhysicalDevice() override;
    
    // Delete copy operations to prevent double-free
    WebGPUPhysicalDevice(const WebGPUPhysicalDevice&) = delete;
    WebGPUPhysicalDevice& operator=(const WebGPUPhysicalDevice&) = delete;
    
    // Delete move operations - this class should be used via shared_ptr
    WebGPUPhysicalDevice(WebGPUPhysicalDevice&&) = delete;
    WebGPUPhysicalDevice& operator=(WebGPUPhysicalDevice&&) = delete;
    
    /**
     * @brief Get device capabilities
     * @return Device capabilities structure
     */
    PhysicalDeviceCapabilities getCapabilities() const override;
    
    /**
     * @brief Get available queue families
     * @return Vector of queue families
     */
    std::vector<QueueFamily> getQueueFamilies() const override;
    
    /**
     * @brief Check if device supports surface presentation
     * @param surface Surface handle to check
     * @return true if surface is supported
     */
    bool supportsSurface(const NativeSurfaceHandle& surface) const override;
    
    /**
     * @brief Create a logical device
     * @param desc Logical device descriptor
     * @return Shared pointer to logical device or nullptr if failed
     */
    std::shared_ptr<ILogicalDevice> createLogicalDevice(
        const LogicalDeviceDesc& desc) override;
    
    /**
     * @brief Get native adapter handle
     * @return Native WGPUAdapter handle
     */
    NativeAdapterHandle getNativeAdapterHandle() const override;
    
private:
    WGPUAdapter _adapter = nullptr;
    WGPUAdapterInfo _adapterInfo = {};  // Adapter info queried at construction
    bool _adapterInfoValid = false;     // Track if adapter info was successfully queried
    
    // Cached capabilities to avoid repeated queries
    mutable std::optional<PhysicalDeviceCapabilities> _cachedCapabilities;
    mutable std::optional<std::vector<QueueFamily>> _cachedQueueFamilies;
    mutable std::mutex _cacheMutex;  // Thread-safety for cache access
    
    void queryAdapterInfo();
    PhysicalDeviceCapabilities queryCapabilities() const;
    bool validateLimitsWithinCapability(const DeviceLimits& requested, const WGPULimits& available) const;
    bool checkFeatureSupport(const std::vector<DeviceFeature>& requiredFeatures, 
                            std::vector<WGPUFeatureName>& outWGPUFeatures) const;
};

} // namespace pers