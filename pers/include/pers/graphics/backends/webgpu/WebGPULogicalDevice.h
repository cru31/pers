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
     * @param adapter WebGPU adapter handle (kept for reference, not owned)
     */
    WebGPULogicalDevice(WGPUDevice device, WGPUAdapter adapter);
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
    
private:
    WGPUDevice _device = nullptr;
    WGPUAdapter _adapter = nullptr;  // Keep reference for queries
    std::shared_ptr<IQueue> _defaultQueue;  // WebGPU has single queue
    mutable std::shared_ptr<IResourceFactory> _resourceFactory;  // Cached factory
    
    bool createDefaultQueue();
};

} // namespace pers