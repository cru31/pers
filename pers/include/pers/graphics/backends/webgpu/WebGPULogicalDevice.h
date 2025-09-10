#pragma once

#include "pers/graphics/ILogicalDevice.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <vector>

namespace pers {

/**
 * @brief WebGPU implementation of ILogicalDevice
 * 
 * This class manages a WebGPU device and follows RAII principles.
 * Copy operations are deleted to prevent double-free issues.
 * Move operations are deleted as this class is meant to be used via shared_ptr.
 */
class WebGPULogicalDevice : public ILogicalDevice, 
                           public std::enable_shared_from_this<WebGPULogicalDevice> {
public:
    /**
     * @brief Constructor
     * @param device WebGPU device handle (takes ownership)
     * @param physicalDevice The physical device this logical device was created from
     */
    WebGPULogicalDevice(WGPUDevice device,
                       const std::shared_ptr<IPhysicalDevice>& physicalDevice);
    ~WebGPULogicalDevice() override;
    
    // Delete copy operations to prevent double-free
    WebGPULogicalDevice(const WebGPULogicalDevice&) = delete;
    WebGPULogicalDevice& operator=(const WebGPULogicalDevice&) = delete;
    
    // Delete move operations - this class should be used via shared_ptr
    WebGPULogicalDevice(WebGPULogicalDevice&&) = delete;
    WebGPULogicalDevice& operator=(WebGPULogicalDevice&&) = delete;
    
    // Queue operations
    std::shared_ptr<IQueue> getQueue() const override;
    
    // Resource creation factory
    std::shared_ptr<IResourceFactory> getResourceFactory() const override;
    
    // Command operations
    std::shared_ptr<ICommandEncoder> createCommandEncoder() override;
    
    // Swap chain creation
    std::shared_ptr<ISwapChain> createSwapChain(
        const NativeSurfaceHandle& surface,
        const SwapChainDesc& desc) override;
    
    // Synchronization
    void waitIdle() override;
    
    // Native handle access
    NativeDeviceHandle getNativeDeviceHandle() const override;
    
    // Physical device access
    std::shared_ptr<IPhysicalDevice> getPhysicalDevice() const override;
    
    // SwapChain management (for depth buffer auto-linking)
    void setCurrentSwapChain(const std::shared_ptr<ISwapChain>& swapChain);
    std::shared_ptr<ISwapChain> getCurrentSwapChain() const;

private:
    WGPUDevice _device = nullptr;
    std::weak_ptr<IPhysicalDevice> _physicalDevice;  // The physical device this was created from
    std::shared_ptr<IQueue> _defaultQueue;  // WebGPU has single queue
    mutable std::shared_ptr<IResourceFactory> _resourceFactory;  // Cached factory
    std::weak_ptr<ISwapChain> _currentSwapChain;  // Track current SwapChain for auto depth buffer
    
    bool createDefaultQueue();
};

} // namespace pers