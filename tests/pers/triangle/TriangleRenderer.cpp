#include "TriangleRenderer.h"
#include "pers/graphics/IInstance.h"
#include <iostream>

TriangleRenderer::~TriangleRenderer() {
    std::cout << "[TriangleRenderer] Destructor called" << std::endl;
    cleanup();
}

bool TriangleRenderer::initialize(std::shared_ptr<pers::IInstance> instance, const glm::ivec2& size) {
    _windowSize = size;
    _instance = instance;
    
    if (!_instance) {
        std::cerr << "[TriangleRenderer] Invalid instance provided" << std::endl;
        return false;
    }
    
    std::cout << "[TriangleRenderer] Initializing with size: " 
              << _windowSize.x << "x" << _windowSize.y << std::endl;
    
    // Surface will be created and set by the app
    
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
    glm::ivec2 newSize(width, height);
    if (_windowSize == newSize) {
        return; // No change
    }
    
    _windowSize = newSize;
    
    std::cout << "[TriangleRenderer] Resized to: " << _windowSize.x << "x" << _windowSize.y << std::endl;
    
    // TODO: Recreate swap chain with new size
}

void TriangleRenderer::cleanup() {
    // Clean up in reverse order of creation
    
    // TODO: Release pipeline, buffers, swap chain, device
    
    // Release surface (WebGPU will handle this)
    _surface = nullptr;
    
    // Release instance
    _instance.reset();
    
    std::cout << "[TriangleRenderer] Cleanup completed" << std::endl;
}