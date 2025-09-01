#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"

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
     * @return true if submission succeeded
     */
    virtual bool submit(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) = 0;
    
    /**
     * @brief Submit a single command buffer for execution
     * @param commandBuffer Command buffer to submit
     * @return true if submission succeeded
     */
    virtual bool submit(const std::shared_ptr<ICommandBuffer>& commandBuffer) = 0;
    
    /**
     * @brief Submit multiple command buffers as a batch
     * @param commandBuffers Array of command buffers to submit
     * @return true if submission succeeded
     */
    virtual bool submitBatch(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) = 0;
    
    /**
     * @brief Write data to a buffer
     * @param desc Buffer write descriptor
     * @return true if write succeeded
     */
    virtual bool writeBuffer(const BufferWriteDesc& desc) = 0;
    
    /**
     * @brief Write data to a texture
     * @param texture Target texture
     * @param data Source data
     * @param dataSize Size of data in bytes
     * @param mipLevel Target mip level
     * @return true if write succeeded
     */
    virtual bool writeTexture(const std::shared_ptr<ITexture>& texture, 
                             const void* data, 
                             uint64_t dataSize,
                             uint32_t mipLevel = 0) = 0;
    
    /**
     * @brief Wait for all submitted work to complete
     * @return true if wait succeeded
     */
    virtual bool waitIdle() = 0;
    
    /**
     * @brief Get native queue handle for backend-specific operations
     * @return Native queue handle (WGPUQueue for WebGPU)
     */
    virtual NativeQueueHandle getNativeQueueHandle() const = 0;
};

} // namespace pers