#pragma once

#include <memory>
#include <cstdint>

namespace pers {

// Forward declarations
class IQueue;
class ICommandEncoder;
class ISwapChain;
class IResourceFactory;

/**
 * @brief Swap chain descriptor
 */
struct SwapChainDesc {
    uint32_t width = 800;
    uint32_t height = 600;
    uint32_t presentMode = 0;  // 0: Fifo (VSync), 1: Immediate, 2: Mailbox
    // Note: format will be determined by the backend based on surface capabilities
};

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
        void* surface, const SwapChainDesc& desc) = 0;
    
    /**
     * @brief Wait for all GPU operations to complete
     */
    virtual void waitIdle() = 0;
    
    /**
     * @brief Get native handle for backend-specific operations
     * @return Native device handle (WGPUDevice for WebGPU)
     */
    virtual void* getNativeHandle() const = 0;
};

} // namespace pers