#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <webgpu/webgpu.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Callback for adapter request
void handleRequestAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestAdapterStatus_Success) {
        std::cout << "Got adapter!" << std::endl;
        *static_cast<WGPUAdapter*>(userdata1) = adapter;
    } else {
        std::cout << "Failed to get adapter: ";
        if (message.data) {
            std::cout.write(message.data, message.length);
        } else {
            std::cout << "unknown error";
        }
        std::cout << std::endl;
    }
}

// Callback for device request  
void handleRequestDevice(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestDeviceStatus_Success) {
        std::cout << "Got device!" << std::endl;
        *static_cast<WGPUDevice*>(userdata1) = device;
    } else {
        std::cout << "Failed to get device: ";
        if (message.data) {
            std::cout.write(message.data, message.length);
        } else {
            std::cout << "unknown error";
        }
        std::cout << std::endl;
    }
}

// Error callback
void handleError(WGPUDevice const * device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2) {
    std::cout << "Error: ";
    if (message.data) {
        std::cout.write(message.data, message.length);
    } else {
        std::cout << "unknown error";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=== WebGPU Native Test ===" << std::endl;

    // Create instance
    WGPUInstanceDescriptor instanceDesc = {};
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        return 1;
    }
    std::cout << "WebGPU instance created successfully" << std::endl;

    // Request adapter
    WGPUAdapter adapter = nullptr;
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    
    // Set up callback info for v25 API
    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = handleRequestAdapter;
    callbackInfo.userdata1 = &adapter;
    
    WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapterOptions, callbackInfo);
    
    // Poll the instance to process callbacks
    // wgpu-native v25 requires polling instead of waitAny
    for (int i = 0; i < 100 && !adapter; ++i) {
        wgpuInstanceProcessEvents(instance);
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    if (!adapter) {
        std::cerr << "Failed to get adapter" << std::endl;
        wgpuInstanceRelease(instance);
        return 1;
    }

    // Get adapter info
    WGPUAdapterInfo adapterInfo = {};
    wgpuAdapterGetInfo(adapter, &adapterInfo);
    std::cout << "Adapter info:" << std::endl;
    std::cout << "  Vendor: ";
    if (adapterInfo.vendor.data) {
        std::cout.write(adapterInfo.vendor.data, adapterInfo.vendor.length);
    }
    std::cout << std::endl;
    std::cout << "  Device: ";
    if (adapterInfo.device.data) {
        std::cout.write(adapterInfo.device.data, adapterInfo.device.length);
    }
    std::cout << std::endl;
    std::cout << "  Backend: " << adapterInfo.backendType << std::endl;
    
    // Free the adapter info strings
    wgpuAdapterInfoFreeMembers(adapterInfo);

    // Request device
    WGPUDevice device = nullptr;
    WGPUDeviceDescriptor deviceDesc = {};
    WGPUStringView labelView = {"Test Device", WGPU_STRLEN};
    deviceDesc.label = labelView;
    
    // Set up error callback in device descriptor
    deviceDesc.uncapturedErrorCallbackInfo.callback = handleError;
    deviceDesc.uncapturedErrorCallbackInfo.userdata1 = nullptr;
    
    // Set up callback info for device request
    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallbackInfo.callback = handleRequestDevice;
    deviceCallbackInfo.userdata1 = &device;
    
    WGPUFuture deviceFuture = wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);
    
    // Poll the instance to process callbacks
    // wgpu-native v25 requires polling instead of waitAny
    for (int i = 0; i < 100 && !device; ++i) {
        wgpuInstanceProcessEvents(instance);
        #ifdef _WIN32
            Sleep(10);
        #else
            usleep(10000);
        #endif
    }
    
    if (!device) {
        std::cerr << "Failed to get device" << std::endl;
        wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
        return 1;
    }

    // Get queue
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    std::cout << "Got queue!" << std::endl;

    // Create a simple compute pipeline to test
    const char* computeShader = R"(
        @group(0) @binding(0) var<storage, read> input: array<f32>;
        @group(0) @binding(1) var<storage, read_write> output: array<f32>;
        
        @compute @workgroup_size(1)
        fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
            let index = global_id.x;
            output[index] = input[index] * 2.0;
        }
    )";

    // Create shader module using WGSL directly
    WGPUShaderSourceWGSL wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    WGPUStringView codeView = {computeShader, WGPU_STRLEN};
    wgslDesc.code = codeView;

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = &wgslDesc.chain;
    WGPUStringView shaderLabelView = {"Compute Shader", WGPU_STRLEN};
    shaderDesc.label = shaderLabelView;

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    if (shaderModule) {
        std::cout << "Compute shader module created successfully!" << std::endl;
        
        // Create compute pipeline
        WGPUComputePipelineDescriptor pipelineDesc = {};
        WGPUStringView pipelineLabelView = {"Test Pipeline", WGPU_STRLEN};
        pipelineDesc.label = pipelineLabelView;
        pipelineDesc.compute.module = shaderModule;
        WGPUStringView entryPointView = {"main", WGPU_STRLEN};
        pipelineDesc.compute.entryPoint = entryPointView;
        
        WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(device, &pipelineDesc);
        if (pipeline) {
            std::cout << "Compute pipeline created successfully!" << std::endl;
            wgpuComputePipelineRelease(pipeline);
        }
        
        wgpuShaderModuleRelease(shaderModule);
    } else {
        std::cout << "Failed to create shader module (this is okay for basic init test)" << std::endl;
    }

    // Test creating a buffer
    WGPUBufferDescriptor bufferDesc = {};
    WGPUStringView bufferLabelView = {"Test Buffer", WGPU_STRLEN};
    bufferDesc.label = bufferLabelView;
    bufferDesc.size = 256;
    bufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    bufferDesc.mappedAtCreation = false;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    if (buffer) {
        std::cout << "Buffer created successfully!" << std::endl;
        wgpuBufferRelease(buffer);
    }

    // Cleanup
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);

    std::cout << "=== Test completed successfully ===" << std::endl;
    return 0;
}