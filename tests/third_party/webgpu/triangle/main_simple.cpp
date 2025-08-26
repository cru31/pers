#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <webgpu.h>
#include <wgpu.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Callback for adapter request
void handleRequestAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
    if (status == WGPURequestAdapterStatus_Success) {
        std::cout << "Got adapter!" << std::endl;
        *static_cast<WGPUAdapter*>(userdata) = adapter;
    } else {
        std::cout << "Failed to get adapter: " << (message ? message : "unknown error") << std::endl;
    }
}

// Callback for device request  
void handleRequestDevice(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
    if (status == WGPURequestDeviceStatus_Success) {
        std::cout << "Got device!" << std::endl;
        *static_cast<WGPUDevice*>(userdata) = device;
    } else {
        std::cout << "Failed to get device: " << (message ? message : "unknown error") << std::endl;
    }
}

// Error callback
void handleError(WGPUErrorType type, const char* message, void* userdata) {
    std::cout << "Error: " << (message ? message : "unknown error") << std::endl;
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
    
    wgpuInstanceRequestAdapter(instance, &adapterOptions, handleRequestAdapter, &adapter);
    
    // For wgpu-native v0.19.4.1, we need to process events manually
    // This is a workaround since there's no proper async handling yet
    for (int i = 0; i < 100 && !adapter; ++i) {
        // Small delay to allow async operation to complete
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

    // Get adapter properties
    WGPUAdapterProperties properties = {};
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << "  Name: " << properties.name << std::endl;
    std::cout << "  Driver: " << properties.driverDescription << std::endl;
    std::cout << "  Backend: " << properties.backendType << std::endl;

    // Request device
    WGPUDevice device = nullptr;
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = "Test Device";
    
    // Don't set required limits for now - use defaults
    // WGPURequiredLimits requiredLimits = {};
    // requiredLimits.limits.maxBindGroups = 1;
    // deviceDesc.requiredLimits = &requiredLimits;
    
    wgpuAdapterRequestDevice(adapter, &deviceDesc, handleRequestDevice, &device);
    
    // Wait for device creation
    for (int i = 0; i < 100 && !device; ++i) {
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

    // Set error callback
    wgpuDeviceSetUncapturedErrorCallback(device, handleError, nullptr);

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
    WGPUShaderModuleWGSLDescriptor wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgslDesc.code = computeShader;

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = &wgslDesc.chain;
    shaderDesc.label = "Compute Shader";

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    if (shaderModule) {
        std::cout << "Compute shader module created successfully!" << std::endl;
        
        // Create compute pipeline
        WGPUComputePipelineDescriptor pipelineDesc = {};
        pipelineDesc.label = "Test Pipeline";
        pipelineDesc.compute.module = shaderModule;
        pipelineDesc.compute.entryPoint = "main";
        
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
    bufferDesc.label = "Test Buffer";
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