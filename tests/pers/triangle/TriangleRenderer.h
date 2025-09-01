#pragma once

#include <memory>
#include <glm/vec2.hpp>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {
    class IGraphicsBackendFactory;
    class IInstance;
    class IPhysicalDevice;
    class ILogicalDevice;
    class IQueue;
    class ISwapChain;
    class IBuffer;
    class IRenderPipeline;
}

class TriangleRenderer {
public:
    TriangleRenderer() = default;
    ~TriangleRenderer();
    
    // Initialize the renderer with instance and size
    bool initialize(std::shared_ptr<pers::IInstance> instance, const glm::ivec2& size);
    
    // Get the graphics instance (for surface creation)
    std::shared_ptr<pers::IInstance> getInstance() const { return _instance; }
    
    // Set the surface (created by the app)
    void setSurface(const pers::NativeSurfaceHandle& surface);
    
    // Create triangle resources (vertex buffer, shaders, pipeline)
    bool createTriangleResources();
    
    // Render a frame
    void renderFrame();
    
    // Handle window resize
    void onResize(int width, int height);
    
    // Cleanup resources
    void cleanup();
    
private:
    // Device initialization helpers
    bool requestPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapChain();
    
private:
    glm::ivec2 _windowSize = glm::ivec2(800, 600);
    
    // Pers graphics resources
    std::shared_ptr<pers::IInstance> _instance;
    std::shared_ptr<pers::IPhysicalDevice> _physicalDevice;
    std::shared_ptr<pers::ILogicalDevice> _device;
    std::shared_ptr<pers::IQueue> _queue;
    pers::NativeSurfaceHandle _surface; // Surface handle
    
    // Rendering resources
    std::shared_ptr<pers::ISwapChain> _swapChain;
    std::shared_ptr<pers::IBuffer> _vertexBuffer;
    std::shared_ptr<pers::IRenderPipeline> _renderPipeline;
};