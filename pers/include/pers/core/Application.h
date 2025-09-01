#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include "pers/graphics/GraphicsTypes.h"

class IWindow;
class IWindowFactory;

namespace pers {
    class IGraphicsBackendFactory;
    class IInstance;
}

/**
 * Base application class providing common functionality for graphics applications
 * Handles window creation, graphics initialization, and lifecycle management
 */
class Application {
public:
    Application();
    virtual ~Application();
    
    // Initialize application with factories
    bool initialize(const std::shared_ptr<IWindowFactory>& windowFactory, 
                   const std::shared_ptr<pers::IGraphicsBackendFactory>& graphicsFactory);
    
    // Main run loop
    void run();
    
    // Accessor methods
    glm::ivec2 getFramebufferSize() const;
    
protected:
    // Virtual methods for derived classes to override
    virtual bool onInitialize() { return true; }  // Called after window and instance are created
    virtual void onUpdate(float deltaTime) {}      // Called each frame
    virtual void onRender() {}                     // Called each frame for rendering
    virtual void onResize(int width, int height) {} // Window resize event
    virtual void onKeyPress(int key, int scancode, int action, int mods) {} // Key press event
    virtual void onCleanup() {}                    // Called before cleanup
    
    // Protected accessors for derived classes
    IWindow* getWindow() const { return _window.get(); }
    std::shared_ptr<pers::IInstance> getInstance() const { return _instance; }
    
    // Helper method for surface creation
    pers::NativeSurfaceHandle createSurface() const;
    
private:
    // Initialization methods
    bool createWindow();
    bool setupWindowCallbacks();
    bool createInstance();
    
    // Runtime methods
    void cleanup();
    
    // Event handlers (forward to virtual methods)
    void handleResize(int width, int height);
    void handleKeyPress(int key, int scancode, int action, int mods);
    
protected:
    // Default window size constants
    static constexpr int DEFAULT_WIDTH = 800;
    static constexpr int DEFAULT_HEIGHT = 600;
    
    // Window configuration (can be overridden before initialize)
    int _windowWidth = DEFAULT_WIDTH;
    int _windowHeight = DEFAULT_HEIGHT;
    std::string _windowTitle = "Application";
    
private:
    // Factories
    std::shared_ptr<IWindowFactory> _windowFactory;
    std::shared_ptr<pers::IGraphicsBackendFactory> _graphicsFactory;
    
    // Created resources
    std::unique_ptr<IWindow> _window;
    std::shared_ptr<pers::IInstance> _instance;
};