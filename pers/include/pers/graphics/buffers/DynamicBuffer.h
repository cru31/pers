#pragma once

#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/IMappableBuffer.h"
#include "pers/graphics/buffers/INativeMappableBuffer.h"
#include <memory>
#include <atomic>
#include <vector>

namespace pers {

class ILogicalDevice;

/**
 * Ring buffer for per-frame dynamic data updates
 * Automatically manages multiple buffer copies for frame overlap
 */
class DynamicBuffer : public IBuffer {
public:
    static constexpr uint32_t DEFAULT_FRAME_COUNT = 3;
    
    struct UpdateHandle {
        void* data;
        uint64_t size;
        uint32_t frameIndex;
    };
    
    DynamicBuffer();
    virtual ~DynamicBuffer();
    
    /**
     * Create and initialize the dynamic buffer
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags
     * @param device Logical device to create resources
     * @param frameCount Number of frames to buffer (default 3)
     * @param debugName Optional debug name
     * @return true if creation succeeded
     */
    bool create(uint64_t size,
                BufferUsage usage,
                const std::shared_ptr<ILogicalDevice>& device,
                uint32_t frameCount = DEFAULT_FRAME_COUNT,
                const std::string& debugName = "");
    
    /**
     * Destroy the dynamic buffer and release resources
     */
    void destroy();
    
    DynamicBuffer(const DynamicBuffer&) = delete;
    DynamicBuffer& operator=(const DynamicBuffer&) = delete;
    
    DynamicBuffer(DynamicBuffer&& other) noexcept;
    DynamicBuffer& operator=(DynamicBuffer&& other) noexcept;
    
    // Dynamic update interface
    UpdateHandle beginUpdate();
    void endUpdate(const UpdateHandle& handle);
    
    // Get current frame's buffer for GPU use
    std::shared_ptr<IBuffer> getCurrentFrameBuffer() const;
    uint32_t getCurrentFrameIndex() const;
    
    // Frame synchronization
    void nextFrame();
    
    // IBuffer interface
    virtual uint64_t getSize() const override;
    virtual BufferUsage getUsage() const override;
    virtual const std::string& getDebugName() const override;
    virtual NativeBufferHandle getNativeHandle() const override;
    virtual bool isValid() const override;
    virtual BufferState getState() const override;
    virtual MemoryLocation getMemoryLocation() const override;
    virtual AccessPattern getAccessPattern() const override;
    
protected:
    uint64_t _size;
    BufferUsage _usage;
    std::string _debugName;
    std::vector<std::shared_ptr<INativeMappableBuffer>> _buffers;
    std::atomic<uint32_t> _currentFrame;
    std::vector<bool> _mapped;
    uint32_t _frameCount;
    bool _created;
};

} // namespace pers

