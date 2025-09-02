#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/ICommandBuffer.h"
#include "pers/graphics/IBuffer.h"
#include "pers/graphics/ITexture.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace pers {

WebGPUQueue::WebGPUQueue(WGPUQueue queue)
    : _queue(queue) {
    
    if (_queue) {
        wgpuQueueAddRef(_queue);
        LOG_INFO("WebGPUQueue", "Created with queue");
    } else {
        LOG_ERROR("WebGPUQueue", "Created with null queue!");
    }
}

WebGPUQueue::~WebGPUQueue() {
    if (_queue) {
        wgpuQueueRelease(_queue);
        _queue = nullptr;
    }
}

bool WebGPUQueue::submit(const std::shared_ptr<ICommandBuffer>& commandBuffer) {
    if (!_queue) {
        LOG_ERROR("WebGPUQueue", "Cannot submit: queue is null");
        return false;
    }
    
    if (!commandBuffer) {
        LOG_ERROR("WebGPUQueue", "Cannot submit null command buffer");
        return false;
    }
    
    // Get native command buffer handle
    NativeCommandBufferHandle nativeHandle = commandBuffer->getNativeCommandBufferHandle();
    if (!nativeHandle.isValid()) {
        LOG_ERROR("WebGPUQueue", "Command buffer has invalid native handle");
        return false;
    }
    
    // Convert to WebGPU command buffer
    WGPUCommandBuffer wgpuCmdBuffer = nativeHandle.as<WGPUCommandBuffer>();
    
    // Submit to queue
    wgpuQueueSubmit(_queue, 1, &wgpuCmdBuffer);
    
    // Command buffer submitted successfully
    return true;
}

bool WebGPUQueue::submit(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) {
    if (!_queue) {
        LOG_ERROR("WebGPUQueue", "Cannot submit: queue is null");
        return false;
    }
    
    if (commandBuffers.empty()) {
        return true; // Empty batch is OK
    }
    
    // Collect native handles
    std::vector<WGPUCommandBuffer> wgpuBuffers;
    wgpuBuffers.reserve(commandBuffers.size());
    
    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        if (!commandBuffers[i]) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUQueue", PERS_SOURCE_LOC, "Null command buffer at index %zu", i);
            return false;
        }
        
        NativeCommandBufferHandle nativeHandle = commandBuffers[i]->getNativeCommandBufferHandle();
        if (!nativeHandle.isValid()) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUQueue", PERS_SOURCE_LOC, "Invalid native handle at index %zu", i);
            return false;
        }
        
        wgpuBuffers.push_back(nativeHandle.as<WGPUCommandBuffer>());
    }
    
    // Submit batch to queue
    wgpuQueueSubmit(_queue, static_cast<uint32_t>(wgpuBuffers.size()), wgpuBuffers.data());
    
    // Command buffers submitted successfully
    return true;
}

bool WebGPUQueue::submitBatch(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) {
    // Just delegate to submit
    return submit(commandBuffers);
}

bool WebGPUQueue::writeBuffer(const BufferWriteDesc& desc) {
    if (!_queue) {
        LOG_ERROR("WebGPUQueue", "Cannot write buffer: queue is null");
        return false;
    }
    
    if (!desc.buffer || !desc.data || desc.size == 0) {
        LOG_ERROR("WebGPUQueue", "Invalid buffer write parameters");
        return false;
    }
    
    // Get native buffer handle
    NativeBufferHandle nativeHandle = desc.buffer->getNativeBufferHandle();
    if (!nativeHandle.isValid()) {
        LOG_ERROR("WebGPUQueue", "Invalid buffer handle");
        return false;
    }
    
    WGPUBuffer wgpuBuffer = nativeHandle.as<WGPUBuffer>();
    wgpuQueueWriteBuffer(_queue, wgpuBuffer, desc.offset, desc.data, desc.size);
    
    return true;
}

bool WebGPUQueue::writeTexture(const std::shared_ptr<ITexture>& texture, 
                               const void* data, 
                               uint64_t dataSize,
                               uint32_t mipLevel) {
    if (!_queue) {
        LOG_ERROR("WebGPUQueue", "Cannot write texture: queue is null");
        return false;
    }
    
    TODO_OR_DIE(
        "WebGPUQueue::writeTexture",
        "Implement texture data upload via queue. Steps: "
        "1. Get native texture handle from ITexture, "
        "2. Set up WGPUImageCopyTexture descriptor, "
        "3. Set up WGPUTextureDataLayout, "
        "4. Call wgpuQueueWriteTexture"
    );
    
    return false;
}

bool WebGPUQueue::waitIdle() {
    if (!_queue) {
        return false;
    }
    
    // Use condition variable for proper synchronization
    struct CallbackData {
        bool isComplete = false;
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    CallbackData callbackData;
    
    // Set up callback info
    WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
        CallbackData* data = static_cast<CallbackData*>(userdata1);
        if (data) {
            {
                std::lock_guard<std::mutex> lock(data->mutex);
                data->isComplete = true;
                
                if (status != WGPUQueueWorkDoneStatus_Success) {
                    LOG_WARNING("WebGPUQueue", 
                                         "Queue work done with non-success status");
                }
            }
            data->cv.notify_one();
        }
    };
    callbackInfo.userdata1 = &callbackData;
    callbackInfo.userdata2 = nullptr;
    
    // Submit the callback
    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(_queue, callbackInfo);
    
    // Wait for completion with timeout
    std::unique_lock<std::mutex> lock(callbackData.mutex);
    const auto timeout = std::chrono::seconds(30); // 30 second timeout for GPU work
    
    bool success = callbackData.cv.wait_for(lock, timeout, [&callbackData]() {
        return callbackData.isComplete;
    });
    
    if (!success) {
        LOG_ERROR("WebGPUQueue", 
                              "Timeout waiting for queue to idle");
        return false;
    }
    
    return true;
}

NativeQueueHandle WebGPUQueue::getNativeQueueHandle() const {
    return NativeQueueHandle::fromBackend(_queue);
}

void WebGPUQueue::writeBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, uint64_t size) {
    if (!_queue) {
        LOG_ERROR("WebGPUQueue", "Cannot write buffer: queue is null");
        return;
    }
    
    if (!buffer || !data || size == 0) {
        LOG_ERROR("WebGPUQueue", "Invalid buffer write parameters");
        return;
    }
    
    wgpuQueueWriteBuffer(_queue, buffer, offset, data, size);
    // Buffer write completed
}

} // namespace pers