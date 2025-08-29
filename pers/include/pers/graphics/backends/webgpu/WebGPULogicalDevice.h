#pragma once

#include "pers/graphics/ILogicalDevice.h"
#include <webgpu.h>
#include <memory>
#include <vector>

namespace pers {

/**
 * @brief WebGPU implementation of ILogicalDevice
 */
class WebGPULogicalDevice : public ILogicalDevice {
public:
    /**
     * @brief Constructor
     * @param device WebGPU device handle
     * @param adapter WebGPU adapter handle (kept for reference)
     */
    WebGPULogicalDevice(WGPUDevice device, WGPUAdapter adapter);
    ~WebGPULogicalDevice() override;
    
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
    
    bool createDefaultQueue();
};

} // namespace pers