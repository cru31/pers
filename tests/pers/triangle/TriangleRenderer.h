#pragma once

#include <memory>
#include <glm/vec2.hpp>

namespace pers {
    class IGraphicsBackendFactory;
    class IInstance;
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
    void setSurface(void* surface) { _surface = surface; }
    
    // Create triangle resources (vertex buffer, shaders, pipeline)
    bool createTriangleResources();
    
    // Render a frame
    void renderFrame();
    
    // Handle window resize
    void onResize(int width, int height);
    
    // Cleanup resources
    void cleanup();
    
private:
    glm::ivec2 _windowSize = glm::ivec2(800, 600);
    
    // Pers graphics resources
    std::shared_ptr<pers::IInstance> _instance;
    void* _surface = nullptr; // Opaque surface handle
};