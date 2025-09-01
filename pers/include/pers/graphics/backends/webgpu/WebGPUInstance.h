#pragma once

#include "pers/graphics/IInstance.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"  // For InstanceDesc
#include <webgpu/webgpu.h>

namespace pers {

/**
 * @brief WebGPU implementation of IInstance
 */
class WebGPUInstance : public IInstance {
public:
    WebGPUInstance();
    ~WebGPUInstance() override;
    
    /**
     * @brief Initialize the WebGPU instance
     * @param desc Instance descriptor
     * @return true on success, false on failure
     */
    bool initialize(const InstanceDesc& desc);
    
    /**
     * @brief Request a physical device (adapter)
     * @param options Physical device options
     * @return Shared pointer to physical device or nullptr if failed
     */
    std::shared_ptr<IPhysicalDevice> requestPhysicalDevice(
        const PhysicalDeviceOptions& options) override;
    
    /**
     * @brief Create a surface from a native window handle
     * @param windowHandle Native window handle (GLFWwindow*)
     * @return Native surface handle or nullptr if failed
     */
    NativeSurfaceHandle createSurface(void* windowHandle) override;
    
private:
    WGPUInstance _instance = nullptr;
    InstanceDesc _desc; // Stored instance configuration
};

} // namespace pers