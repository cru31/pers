#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <webgpu.h>
#include <wgpu.h>

// Helper function to create WGPUStringView from C string
WGPUStringView makeStringView(const char* str) {
    WGPUStringView view = {};
    view.data = str;
    view.length = strlen(str);
    return view;
}

// Helper function to print WGPUStringView
void printStringView(const WGPUStringView& str) {
    if (str.data && str.length > 0) {
        std::cout.write(str.data, str.length);
    }
}

void handleRequestAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestAdapterStatus_Success) {
        std::cout << "Got adapter!" << std::endl;
        *static_cast<WGPUAdapter*>(userdata1) = adapter;
    } else {
        std::cout << "Failed to get adapter: ";
        printStringView(message);
        std::cout << std::endl;
    }
}

void handleRequestDevice(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestDeviceStatus_Success) {
        std::cout << "Got device!" << std::endl;
        *static_cast<WGPUDevice*>(userdata1) = device;
    } else {
        std::cout << "Failed to get device: ";
        printStringView(message);
        std::cout << std::endl;
    }
}

void handleDeviceLost(WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message, void* userdata1, void* userdata2) {
    std::cout << "Device lost: ";
    printStringView(message);
    std::cout << std::endl;
}

void handleUncapturedError(WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2) {
    std::cout << "Uncaptured error: ";
    printStringView(message);
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
    
    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.callback = handleRequestAdapter;
    callbackInfo.userdata1 = &adapter;
    callbackInfo.userdata2 = nullptr;
    
    WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapterOptions, callbackInfo);
    
    // Process events instead of WaitAny (not implemented yet in wgpu-native)
    wgpuInstanceProcessEvents(instance);
    
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
    printStringView(adapterInfo.vendor);
    std::cout << std::endl;
    std::cout << "  Device: ";
    printStringView(adapterInfo.device);
    std::cout << std::endl;
    std::cout << "  Description: ";
    printStringView(adapterInfo.description);
    std::cout << std::endl;
    std::cout << "  Backend Type: " << adapterInfo.backendType << std::endl;

    // Request device
    WGPUDevice device = nullptr;
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = makeStringView("Test Device");
    
    WGPUDeviceLostCallbackInfo lostInfo = {};
    lostInfo.callback = handleDeviceLost;
    deviceDesc.deviceLostCallbackInfo = lostInfo;
    
    WGPUUncapturedErrorCallbackInfo errorInfo = {};
    errorInfo.callback = handleUncapturedError;
    deviceDesc.uncapturedErrorCallbackInfo = errorInfo;
    
    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = handleRequestDevice;
    deviceCallbackInfo.userdata1 = &device;
    deviceCallbackInfo.userdata2 = nullptr;
    
    WGPUFuture deviceFuture = wgpuAdapterRequestDevice(adapter, &deviceDesc, deviceCallbackInfo);
    
    // Process events instead of WaitAny (not implemented yet in wgpu-native)
    wgpuInstanceProcessEvents(instance);
    
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

    // For wgpu-native, we can use the WGSL extension
    WGPUShaderSourceWGSL wgslSource = {};
    wgslSource.chain.next = nullptr;
    wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSource.code = makeStringView(computeShader);

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.label = makeStringView("Compute Shader");
    shaderDesc.nextInChain = &wgslSource.chain;

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    if (shaderModule) {
        std::cout << "Compute shader module created successfully!" << std::endl;
    } else {
        std::cout << "Failed to create compute shader module" << std::endl;
    }

    // Cleanup
    if (shaderModule) wgpuShaderModuleRelease(shaderModule);
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);

    std::cout << "=== Test completed successfully ===" << std::endl;
    return 0;
}