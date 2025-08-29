#pragma once

#include "pers/graphics/IPhysicalDevice.h"
#include <webgpu.h>

namespace pers {

/**
 * @brief WebGPU implementation of IPhysicalDevice
 */
class WebGPUPhysicalDevice : public IPhysicalDevice {
public:
    explicit WebGPUPhysicalDevice(WGPUAdapter adapter);
    ~WebGPUPhysicalDevice() override;
    
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
    WGPUAdapterInfo _adapterInfo = {}; // Adapter info queried at construction
    
    void queryAdapterInfo();
};

} // namespace pers