#pragma once

#include <memory>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Forward declarations
class IQueue;
class ICommandEncoder;
class ISwapChain;
class IResourceFactory;
class IPhysicalDevice;
struct SwapChainDesc;

/**
 * @brief Logical device interface
 * 
 * Represents a logical GPU device created from a physical device.
 * Manages resources, command encoders, and queues.
 * Based on WebGPU Device concept but abstracted for cross-platform use.
 */
class ILogicalDevice {
public:
    virtual ~ILogicalDevice() = default;
    
    /**
     * @brief Get the device queue for command submission
     * @return Shared pointer to the queue
     */
    virtual std::shared_ptr<IQueue> getQueue() const = 0;
    
    /**
     * @brief Get the resource factory for creating GPU resources
     * @return Shared pointer to resource factory
     */
    virtual std::shared_ptr<IResourceFactory> getResourceFactory() const = 0;
    
    /**
     * @brief Create a command encoder for recording GPU commands
     * @return Shared pointer to command encoder
     */
    virtual std::shared_ptr<ICommandEncoder> createCommandEncoder() = 0;
    
    /**
     * @brief Create a swap chain for surface presentation
     * @param surface Surface handle created by IInstance
     * @param desc Swap chain descriptor
     * @return Shared pointer to swap chain or nullptr if failed
     */
    virtual std::shared_ptr<ISwapChain> createSwapChain(
        const NativeSurfaceHandle& surface, const SwapChainDesc& desc) = 0;
    
    /**
     * @brief Wait for all GPU operations to complete
     */
    virtual void waitIdle() = 0;
    
    /**
     * @brief Get native device handle for backend-specific operations
     * @return Native device handle (WGPUDevice for WebGPU)
     */
    virtual NativeDeviceHandle getNativeDeviceHandle() const = 0;
    
    /**
     * @brief Get the physical device this logical device was created from
     * @return Shared pointer to physical device or nullptr if expired
     */
    virtual std::shared_ptr<IPhysicalDevice> getPhysicalDevice() const = 0;
};

} // namespace pers