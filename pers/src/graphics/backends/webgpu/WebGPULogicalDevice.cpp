#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/backends/webgpu/WebGPUSwapChain.h"
#include "pers/graphics/backends/webgpu/WebGPUCommandEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUResourceFactory.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <iostream>

namespace pers {

WebGPULogicalDevice::WebGPULogicalDevice(WGPUDevice device, WGPUAdapter adapter)
    : _device(device), _adapter(adapter) {
    
    if (_device) {
        wgpuDeviceAddRef(_device);
        LOG_INFO("WebGPULogicalDevice", "Created with device");
        
        // Create default queue immediately
        if (!createDefaultQueue()) {
            LOG_ERROR("WebGPULogicalDevice", "Failed to create default queue");
            // Device is still valid but queue creation failed
            // Caller should check if queue is available
        }
    } else {
        LOG_ERROR("WebGPULogicalDevice", "Created with null device!");
    }
    
    if (_adapter) {
        wgpuAdapterAddRef(_adapter);
    }
}

WebGPULogicalDevice::~WebGPULogicalDevice() {
    _defaultQueue.reset();
    
    if (_device) {
        wgpuDeviceRelease(_device);
        _device = nullptr;
    }
    
    if (_adapter) {
        wgpuAdapterRelease(_adapter);
        _adapter = nullptr;
    }
}

bool WebGPULogicalDevice::createDefaultQueue() {
    if (!_device) {
        return false;
    }
    
    // WebGPU devices have a default queue
    WGPUQueue queue = wgpuDeviceGetQueue(_device);
    if (queue) {
        _defaultQueue = std::make_shared<WebGPUQueue>(queue);
        LOG_INFO("WebGPULogicalDevice", "Default queue created");
        return true;
    } else {
        LOG_ERROR("WebGPULogicalDevice", "Failed to get default queue");
        return false;
    }
}

std::shared_ptr<IQueue> WebGPULogicalDevice::getQueue() const {
    return _defaultQueue;
}

std::shared_ptr<IResourceFactory> WebGPULogicalDevice::getResourceFactory() const {
    if (!_device) {
        LOG_ERROR("WebGPULogicalDevice",
            "Cannot create resource factory without device");
        return nullptr;
    }
    
    // Create and cache resource factory on first access
    if (!_resourceFactory) {
        // Use const_cast to get non-const shared_ptr from const method
        auto sharedThis = const_cast<WebGPULogicalDevice*>(this)->shared_from_this();
        
        _resourceFactory = std::make_shared<webgpu::WebGPUResourceFactory>(sharedThis);
        LOG_DEBUG("WebGPULogicalDevice",
            "Created and cached resource factory");
    }
    
    return _resourceFactory;
}

std::shared_ptr<ICommandEncoder> WebGPULogicalDevice::createCommandEncoder() {
    if (!_device) {
        LOG_ERROR("WebGPULogicalDevice", 
                              "Cannot create command encoder with null device");
        return nullptr;
    }
    
    // Create command encoder descriptor
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = WGPUStringView{.data = "Command Encoder", .length = 15};
    
    // Create command encoder
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);
    if (!encoder) {
        LOG_ERROR("WebGPULogicalDevice", 
                              "Failed to create command encoder");
        return nullptr;
    }
    
    return std::make_shared<WebGPUCommandEncoder>(encoder);
}

std::shared_ptr<ISwapChain> WebGPULogicalDevice::createSwapChain(
    const NativeSurfaceHandle& surface,
    const SwapChainDesc& desc) {
    
    if (!_device) {
        LOG_ERROR("WebGPULogicalDevice", 
                              "Cannot create swap chain: device is null");
        return nullptr;
    }
    
    if (!surface.getRaw()) {
        LOG_ERROR("WebGPULogicalDevice", 
                              "Cannot create swap chain: surface is null");
        return nullptr;
    }
    
    // Create WebGPUSwapChain with our device, surface, and descriptor
    try {
        auto swapChain = std::make_shared<WebGPUSwapChain>(
            std::static_pointer_cast<WebGPULogicalDevice>(shared_from_this()),
            surface.as<WGPUSurface>(),
            desc
        );
        
        LOG_INFO("WebGPULogicalDevice", 
                              "Swap chain created successfully");
        
        return swapChain;
    } catch (const std::exception& e) {
        LOG_ERROR("WebGPULogicalDevice", 
                              std::string("Failed to create swap chain: ") + e.what());
        return nullptr;
    }
}

void WebGPULogicalDevice::waitIdle() {
    if (!_device) {
        return;
    }
    
    // WebGPU doesn't have a direct waitIdle equivalent like Vulkan's vkDeviceWaitIdle
    // We need to poll the device to process callbacks and wait for queue completion
    
    // Process any pending callbacks
    // Note: wgpuDevicePoll is the standard WebGPU way to process callbacks
    // but wgpu-native uses a different approach
    
    // For wgpu-native, we wait for the queue to complete all work
    if (_defaultQueue) {
        _defaultQueue->waitIdle();
    }
    
    TODO_OR_DIE("WebGPULogicalDevice::waitIdle", 
                   "Investigate if wgpu-native has a device poll mechanism. Standard WebGPU would use: wgpuDevicePoll(_device, true, nullptr)");
}

NativeDeviceHandle WebGPULogicalDevice::getNativeDeviceHandle() const {
    return NativeDeviceHandle::fromBackend(_device);
}

} // namespace pers