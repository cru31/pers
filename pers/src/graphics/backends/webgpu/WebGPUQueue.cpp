#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/ICommandBuffer.h"
#include "pers/graphics/IBuffer.h"
#include "pers/graphics/ITexture.h"
#include "pers/utils/NotImplemented.h"
#include "pers/utils/Logger.h"
#include <webgpu.h>
#include <vector>
#include <thread>

namespace pers {

WebGPUQueue::WebGPUQueue(WGPUQueue queue)
    : _queue(queue) {
    
    if (_queue) {
        wgpuQueueAddRef(_queue);
        Logger::Instance().Log(LogLevel::Info, "WebGPUQueue", "Created with queue", PERS_SOURCE_LOC);
    } else {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Created with null queue!", PERS_SOURCE_LOC);
    }
}

WebGPUQueue::~WebGPUQueue() {
    if (_queue) {
        wgpuQueueRelease(_queue);
        _queue = nullptr;
    }
}

bool WebGPUQueue::submit(std::shared_ptr<ICommandBuffer> commandBuffer) {
    if (!_queue) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot submit: queue is null", PERS_SOURCE_LOC);
        return false;
    }
    
    if (!commandBuffer) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot submit null command buffer", PERS_SOURCE_LOC);
        return false;
    }
    
    // Get native command buffer handle
    NativeCommandBufferHandle nativeHandle = commandBuffer->getNativeCommandBufferHandle();
    if (!nativeHandle.isValid()) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Command buffer has invalid native handle", PERS_SOURCE_LOC);
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
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot submit: queue is null", PERS_SOURCE_LOC);
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
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot write buffer: queue is null", PERS_SOURCE_LOC);
        return false;
    }
    
    if (!desc.buffer || !desc.data || desc.size == 0) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Invalid buffer write parameters", PERS_SOURCE_LOC);
        return false;
    }
    
    // Get native buffer handle
    NativeBufferHandle nativeHandle = desc.buffer->getNativeBufferHandle();
    if (!nativeHandle.isValid()) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Invalid buffer handle", PERS_SOURCE_LOC);
        return false;
    }
    
    WGPUBuffer wgpuBuffer = nativeHandle.as<WGPUBuffer>();
    wgpuQueueWriteBuffer(_queue, wgpuBuffer, desc.offset, desc.data, desc.size);
    
    return true;
}

bool WebGPUQueue::writeTexture(std::shared_ptr<ITexture> texture, 
                               const void* data, 
                               uint64_t dataSize,
                               uint32_t mipLevel) {
    if (!_queue) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot write texture: queue is null", PERS_SOURCE_LOC);
        return false;
    }
    
    NotImplemented::Log(
        "WebGPUQueue::writeTexture",
        "Implement texture data upload via queue",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Get native texture handle from ITexture
    // 2. Set up WGPUImageCopyTexture descriptor
    // 3. Set up WGPUTextureDataLayout
    // 4. Call wgpuQueueWriteTexture
    
    return false;
}

bool WebGPUQueue::waitIdle() {
    if (!_queue) {
        return false;
    }
    
    // Use a simple synchronous approach for now
    // wgpuQueueOnSubmittedWorkDone with a callback to know when work is done
    
    struct CallbackData {
        bool isComplete = false;
    };
    
    CallbackData callbackData;
    
    // Set up callback info
    WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
        CallbackData* data = static_cast<CallbackData*>(userdata1);
        if (data) {
            if (status == WGPUQueueWorkDoneStatus_Success) {
                data->isComplete = true;
            } else {
                // Log error but still mark as complete to avoid infinite wait
                data->isComplete = true;
            }
        }
    };
    callbackInfo.userdata1 = &callbackData;
    callbackInfo.userdata2 = nullptr;
    
    // Submit the callback
    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(_queue, callbackInfo);
    
    // For now, we'll do a simple busy wait
    // TODO: In a production system, use proper synchronization primitives
    while (!callbackData.isComplete) {
        // Process events/callbacks - implementation dependent
        // For wgpu-native, there might not be a poll mechanism
        // so we just yield the thread
        std::this_thread::yield();
    }
    
    return true;
}

NativeQueueHandle WebGPUQueue::getNativeQueueHandle() const {
    return NativeQueueHandle::fromBackend(_queue);
}

void WebGPUQueue::writeBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, uint64_t size) {
    if (!_queue) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Cannot write buffer: queue is null", PERS_SOURCE_LOC);
        return;
    }
    
    if (!buffer || !data || size == 0) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUQueue", "Invalid buffer write parameters", PERS_SOURCE_LOC);
        return;
    }
    
    wgpuQueueWriteBuffer(_queue, buffer, offset, data, size);
    // Buffer write completed
}

} // namespace pers