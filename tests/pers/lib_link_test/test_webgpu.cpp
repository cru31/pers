#include <pers/graphics/core.h>
#include <pers/graphics/webgpu.h>
#include <iostream>

int main() {
    std::cout << "=== Testing WebGPU Integration ===" << std::endl;
    
    // Test 1: Check if WebGPU is available
    if (pers::graphics::webgpu::testWebGPUAvailable()) {
        std::cout << "✓ WebGPU is available" << std::endl;
    } else {
        std::cerr << "✗ WebGPU is not available" << std::endl;
        return 1;
    }
    
    // Test 2: Create WebGPU context
    pers::graphics::webgpu::WebGPUContext context;
    std::cout << "WebGPU Context created" << std::endl;
    
    // Test 3: Initialize WebGPU
    pers::graphics::webgpu::WebGPUConfig config;
    config.applicationName = "Pers WebGPU Test";
    config.enableValidation = true;
    
    if (context.initialize(config)) {
        std::cout << "✓ WebGPU initialized successfully" << std::endl;
    } else {
        std::cerr << "✗ Failed to initialize WebGPU" << std::endl;
        return 1;
    }
    
    // Test 4: Check initialization status
    if (context.isInitialized()) {
        std::cout << "✓ WebGPU context is initialized" << std::endl;
    } else {
        std::cerr << "✗ WebGPU context is not initialized" << std::endl;
        return 1;
    }
    
    // Test 5: Get adapter info
    std::string adapterInfo = context.getAdapterInfo();
    std::cout << "Adapter Info: " << adapterInfo << std::endl;
    
    // Test 6: Shutdown
    context.shutdown();
    std::cout << "✓ WebGPU context shutdown" << std::endl;
    
    // Test 7: Verify shutdown
    if (!context.isInitialized()) {
        std::cout << "✓ WebGPU context properly shutdown" << std::endl;
    } else {
        std::cerr << "✗ WebGPU context not properly shutdown" << std::endl;
        return 1;
    }
    
    // Test 8: Re-initialize after shutdown
    if (context.initialize(config)) {
        std::cout << "✓ WebGPU re-initialized after shutdown" << std::endl;
        context.shutdown();
    } else {
        std::cerr << "✗ Failed to re-initialize WebGPU" << std::endl;
        return 1;
    }
    
    std::cout << "=== WebGPU Integration Test PASSED ===" << std::endl;
    return 0;
}