#pragma once

#include <memory>
#include <vector>
#include <cstdint>

namespace pers {

// Forward declarations
class ICommandBuffer;
class IBuffer;
class ITexture;

/**
 * @brief Buffer write descriptor
 */
struct BufferWriteDesc {
    std::shared_ptr<IBuffer> buffer;
    uint64_t offset = 0;
    const void* data = nullptr;
    uint64_t size = 0;
};

/**
 * @brief Queue interface for GPU command submission
 * 
 * Represents a GPU command queue for submitting work.
 * Based on WebGPU Queue concept but abstracted for cross-platform use.
 */
class IQueue {
public:
    virtual ~IQueue() = default;
    
    /**
     * @brief Submit command buffers for execution
     * @param commandBuffers Array of command buffers to submit
     */
    virtual void submit(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) = 0;
    
    /**
     * @brief Submit a single command buffer for execution
     * @param commandBuffer Command buffer to submit
     */
    virtual void submit(std::shared_ptr<ICommandBuffer> commandBuffer) = 0;
    
    /**
     * @brief Write data to a buffer
     * @param desc Buffer write descriptor
     */
    virtual void writeBuffer(const BufferWriteDesc& desc) = 0;
    
    /**
     * @brief Write data to a texture
     * @param texture Target texture
     * @param data Source data
     * @param dataSize Size of data in bytes
     * @param mipLevel Target mip level
     */
    virtual void writeTexture(std::shared_ptr<ITexture> texture, 
                             const void* data, 
                             uint64_t dataSize,
                             uint32_t mipLevel = 0) = 0;
    
    /**
     * @brief Wait for all submitted work to complete
     */
    virtual void waitIdle() = 0;
    
    /**
     * @brief Get native handle for backend-specific operations
     * @return Native queue handle (WGPUQueue for WebGPU)
     */
    virtual void* getNativeHandle() const = 0;
};

} // namespace pers