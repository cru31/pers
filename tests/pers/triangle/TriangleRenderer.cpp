#include "TriangleRenderer.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include <GLFW/glfw3.h>
#include <iostream>

TriangleRenderer::~TriangleRenderer() {
    cleanup();
}

bool TriangleRenderer::initialize(GLFWwindow* window) {
    if (!window) {
        std::cerr << "[TriangleRenderer] Invalid window handle" << std::endl;
        return false;
    }
    
    _window = window;
    
    // Get initial window size
    glfwGetFramebufferSize(_window, &_width, &_height);
    std::cout << "[TriangleRenderer] Initializing with size: " 
              << _width << "x" << _height << std::endl;
    
    // Step 1: Create WebGPU backend factory
    _factory = std::make_shared<pers::WebGPUBackendFactory>();
    std::cout << "[TriangleRenderer] Using backend: " 
              << _factory->getBackendName() << std::endl;
    
    // Step 2: Create graphics instance
    pers::InstanceDesc instanceDesc;
    instanceDesc.applicationName = "Pers Triangle Demo";
    instanceDesc.applicationVersion = 1;
    instanceDesc.engineName = "Pers Graphics Engine";
    instanceDesc.engineVersion = 1;
    instanceDesc.enableValidation = true;
    instanceDesc.preferHighPerformanceGPU = true;
    
    _instance = _factory->createInstance(instanceDesc);
    if (!_instance) {
        std::cerr << "[TriangleRenderer] Failed to create instance" << std::endl;
        return false;
    }
    std::cout << "[TriangleRenderer] Instance created successfully" << std::endl;
    
    // Step 3: Create surface from GLFW window
    _surface = _instance->createSurface(_window);
    if (!_surface) {
        std::cerr << "[TriangleRenderer] Failed to create surface" << std::endl;
        return false;
    }
    std::cout << "[TriangleRenderer] Surface created successfully" << std::endl;
    
    // TODO: Continue with:
    // 4. Request Adapter (PhysicalDevice)
    // 5. Create Device (LogicalDevice)
    // 6. Create SwapChain
    
    std::cout << "[TriangleRenderer] Renderer initialized successfully" << std::endl;
    return true;
}

bool TriangleRenderer::createTriangleResources() {
    std::cout << "[TriangleRenderer] Creating triangle resources..." << std::endl;
    
    // TODO: Create triangle resources
    // 1. Create vertex buffer with triangle data
    // 2. Load/compile shaders
    // 3. Create render pipeline
    // 4. Create bind groups if needed
    
    std::cout << "[TriangleRenderer] Triangle resources created" << std::endl;
    return true;
}

void TriangleRenderer::renderFrame() {
    // TODO: Actual rendering
    // 1. Get current texture from swap chain
    // 2. Create command encoder
    // 3. Begin render pass
    // 4. Set pipeline
    // 5. Set vertex buffer
    // 6. Draw
    // 7. End render pass
    // 8. Submit commands
    // 9. Present swap chain
    
    // For now, just count frames
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {
        std::cout << "[TriangleRenderer] Frame: " << frameCount << std::endl;
    }
}

void TriangleRenderer::onResize(int width, int height) {
    if (_width == width && _height == height) {
        return; // No change
    }
    
    _width = width;
    _height = height;
    
    std::cout << "[TriangleRenderer] Resized to: " << width << "x" << height << std::endl;
    
    // TODO: Recreate swap chain with new size
}

void TriangleRenderer::cleanup() {
    // Clean up in reverse order of creation
    
    // TODO: Release pipeline, buffers, swap chain, device
    
    // Release surface (WebGPU will handle this)
    _surface = nullptr;
    
    // Release instance
    _instance.reset();
    
    // Release factory
    _factory.reset();
    
    std::cout << "[TriangleRenderer] Cleanup completed" << std::endl;
}