#pragma once

#include <memory>

struct GLFWwindow;

namespace pers {
    class IGraphicsBackendFactory;
    class IInstance;
}

class TriangleRenderer {
public:
    TriangleRenderer() = default;
    ~TriangleRenderer();
    
    // Initialize the renderer with the window
    bool initialize(GLFWwindow* window);
    
    // Create triangle resources (vertex buffer, shaders, pipeline)
    bool createTriangleResources();
    
    // Render a frame
    void renderFrame();
    
    // Handle window resize
    void onResize(int width, int height);
    
    // Cleanup resources
    void cleanup();
    
private:
    GLFWwindow* _window = nullptr;
    int _width = 800;
    int _height = 600;
    
    // Pers graphics resources
    std::shared_ptr<pers::IGraphicsBackendFactory> _factory;
    std::shared_ptr<pers::IInstance> _instance;
    void* _surface = nullptr; // Opaque surface handle
};