#pragma once

#include <memory>
#include <chrono>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/SurfaceFramebuffer.h"
#include "pers/graphics/RenderPassConfig.h"

namespace pers {
    class IGraphicsInstanceFactory;
    class IInstance;
    class IPhysicalDevice;
    class ILogicalDevice;
    class IQueue;
    class ISwapChain;
    class IBuffer;
    class IRenderPipeline;
    class ISurfaceFramebuffer;
    class IFramebuffer;
}

// Configuration structure - NO HARDCODING
struct TriangleRendererConfig {
    // Configurable formats
    pers::TextureFormat colorFormat = pers::TextureFormat::BGRA8Unorm;
    pers::TextureFormat depthFormat = pers::TextureFormat::Depth24PlusStencil8;
    
    // Configurable clear values
    glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float clearDepth = 1.0f;
    
    // Window settings
    glm::ivec2 windowSize = glm::ivec2(800, 600);
    pers::PresentMode presentMode = pers::PresentMode::Fifo;  // VSync
    
    // Device settings
    std::chrono::milliseconds deviceTimeout{10000};
    bool enableValidation = true;
    
    // Performance settings
    uint32_t frameLogInterval = 60;
};

class TriangleRenderer {
public:
    TriangleRenderer() = default;
    ~TriangleRenderer();
    
    // Initialize the renderer with instance and config
    bool initialize(const std::shared_ptr<pers::IInstance>& instance, const TriangleRendererConfig& config);
    
    // Get the graphics instance (for surface creation)
    std::shared_ptr<pers::IInstance> getInstance() const { return _instance; }
    
    // Initialize graphics (surface, device, swapchain)
    bool initializeGraphics(const pers::NativeSurfaceHandle& surface);
    
    // Create triangle resources (vertex buffer, shaders, pipeline)
    bool createTriangleResources();
    
    // Render a frame
    void renderFrame();
    
    // Handle window resize
    void onResize(int width, int height);
    
    // Cleanup resources
    void cleanup();
    
private:
    // Setters for storing objects
    void setSurface(const pers::NativeSurfaceHandle& surface);
    void setPhysicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice);
    void setLogicalDevice(const std::shared_ptr<pers::ILogicalDevice>& device);
    void setQueue(const std::shared_ptr<pers::IQueue>& queue);
    
    // Device initialization helpers
    std::shared_ptr<pers::IPhysicalDevice> requestPhysicalDevice(const pers::NativeSurfaceHandle& surface);
    std::shared_ptr<pers::ILogicalDevice> createLogicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice);
    
private:
    // Configuration
    TriangleRendererConfig _config;
    
    // Pers graphics resources
    std::shared_ptr<pers::IInstance> _instance;
    std::shared_ptr<pers::IPhysicalDevice> _physicalDevice;
    std::shared_ptr<pers::ILogicalDevice> _device;
    std::shared_ptr<pers::IQueue> _queue;
    pers::NativeSurfaceHandle _surface; // Surface handle
    
    // Rendering resources
    std::shared_ptr<pers::ISurfaceFramebuffer> _surfaceFramebuffer;  // Surface framebuffer interface
    // NO SEPARATE DEPTH BUFFER - SurfaceFramebuffer handles it internally (Review issue #1)
    std::shared_ptr<pers::IBuffer> _vertexBuffer;
    std::shared_ptr<pers::IRenderPipeline> _renderPipeline;
    
    // Render pass configuration (created once, reused every frame)
    std::unique_ptr<pers::RenderPassConfig> _renderPassConfig;
    
    // Frame counter for performance logging
    uint32_t _frameCounter = 0;
};