#include "PersTriangleApp.h"
#include "TriangleRenderer.h"
#include "IWindow.h"
#include "IWindowFactory.h"
#include "pers/graphics/backends/IGraphicsInstanceFactory.h"
#include "pers/graphics/IInstance.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

PersTriangleApp::PersTriangleApp() = default;

PersTriangleApp::~PersTriangleApp() {
    cleanup();
}

bool PersTriangleApp::initialize(std::shared_ptr<IWindowFactory> windowFactory, std::shared_ptr<pers::IGraphicsInstanceFactory> graphicsFactory) {
    std::cout << "=== PERS RAL Triangle Example ===" << std::endl;
    
    // Store factories
    _windowFactory = windowFactory;
    _graphicsFactory = graphicsFactory;
    
    if (!_windowFactory) {
        std::cerr << "Invalid window factory provided" << std::endl;
        return false;
    }
    
    if (!_graphicsFactory) {
        std::cerr << "Invalid graphics factory provided" << std::endl;
        return false;
    }
    
    // Create window
    if (!createWindow()) {
        return false;
    }
    
    // Setup window callbacks
    if (!setupWindowCallbacks()) {
        return false;
    }
    
    // Create graphics instance after window is ready
    if (!createInstance()) {
        return false;
    }
    
    // Initialize renderer
    if (!initializeRenderer()) {
        return false;
    }
    
    // Create triangle resources
    if (!createTriangle()) {
        return false;
    }
    
    return true;
}

glm::ivec2 PersTriangleApp::getFramebufferSize() const {
    if (!_window) {
        return glm::ivec2(0, 0);
    }
    
    return _window->getFramebufferSize();
}

void PersTriangleApp::run() {
    // In CI environment, run for 5 seconds only
    const char* ciEnv = std::getenv("CI");
    bool isCI = (ciEnv != nullptr);
    auto startTime = std::chrono::steady_clock::now();
    const int maxCISeconds = 5;
    
    while (!_window->shouldClose()) {
        _window->pollEvents();
        render();
        
        // Exit after 5 seconds in CI environment
        if (isCI) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
            if (elapsed >= maxCISeconds) {
                std::cout << "[PersTriangleApp] CI mode: Exiting after " << maxCISeconds << " seconds" << std::endl;
                break;
            }
        }
    }
}

bool PersTriangleApp::createWindow() {
    std::cout << "[PersTriangleApp] Creating window using " << _windowFactory->getFactoryName() << " factory" << std::endl;
    
    _window = _windowFactory->createWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "PERS Triangle");
    if (!_window || !_window->isValid()) {
        std::cerr << "[PersTriangleApp] Failed to create window" << std::endl;
        return false;
    }
    
    std::cout << "[PersTriangleApp] Window created successfully" << std::endl;
    return true;
}

bool PersTriangleApp::setupWindowCallbacks() {
    // Window is already created and passed in via initialize()
    // Just set up callbacks
    if (!_window || !_window->isValid()) {
        std::cerr << "Invalid window" << std::endl;
        return false;
    }
    
    // Set callbacks
    _window->setResizeCallback([this](int width, int height) {
        handleResize(width, height);
    });
    
    _window->setKeyCallback([this](int key, int scancode, int action, int mods) {
        handleKeyPress(key, scancode, action, mods);
    });
    
    return true;
}

bool PersTriangleApp::createInstance() {
    std::cout << "[PersTriangleApp] Creating graphics instance" << std::endl;
    std::cout << "[PersTriangleApp] Using backend: " << _graphicsFactory->getBackendName() << std::endl;
    
    // Create graphics instance
    pers::InstanceDesc instanceDesc;
    instanceDesc.applicationName = "Pers Triangle Demo";
    instanceDesc.applicationVersion = 1;
    instanceDesc.engineName = "Pers Graphics Engine";
    instanceDesc.engineVersion = 1;
    instanceDesc.enableValidation = true;
    instanceDesc.preferHighPerformanceGPU = true;
    
    // In CI environments, allow software renderer as fallback
    const char* ciEnv = std::getenv("CI");
    if (ciEnv && std::string(ciEnv) == "true") {
        instanceDesc.allowSoftwareRenderer = true;
        std::cout << "[PersTriangleApp] CI environment detected, allowing software renderer" << std::endl;
    }
    
    _instance = _graphicsFactory->createInstance(instanceDesc);
    if (!_instance) {
        std::cerr << "[PersTriangleApp] Failed to create instance" << std::endl;
        return false;
    }
    
    std::cout << "[PersTriangleApp] Instance created successfully" << std::endl;
    return true;
}

bool PersTriangleApp::initializeRenderer() {
    // Create and initialize the renderer
    _renderer = std::make_unique<TriangleRenderer>();
    
    // Get framebuffer size to pass to renderer
    glm::ivec2 size = getFramebufferSize();
    
    // Initialize renderer with the instance we created
    if (!_renderer->initialize(_instance, size)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Create surface and set it on renderer
    void* surface = createSurface();
    if (!surface) {
        std::cerr << "Failed to create surface" << std::endl;
        return false;
    }
    
    _renderer->setSurface(surface);
    
    return true;
}

void* PersTriangleApp::createSurface() const {
    if (!_renderer) {
        std::cerr << "[PersTriangleApp] Renderer not initialized" << std::endl;
        return nullptr;
    }
    
    // Get instance from renderer
    auto instance = _renderer->getInstance();
    if (!instance) {
        std::cerr << "[PersTriangleApp] Failed to get instance from renderer" << std::endl;
        return nullptr;
    }
    
    // Get native handle from window
    void* nativeHandle = _window->getNativeHandle();
    if (!nativeHandle) {
        std::cerr << "[PersTriangleApp] Failed to get native handle from window" << std::endl;
        return nullptr;
    }
    
    // Create surface using the instance
    void* surface = instance->createSurface(nativeHandle);
    if (!surface) {
        std::cerr << "[PersTriangleApp] Failed to create surface" << std::endl;
        return nullptr;
    }
    
    std::cout << "[PersTriangleApp] Surface created successfully" << std::endl;
    return surface;
}

bool PersTriangleApp::createTriangle() {
    // Delegate to renderer
    if (!_renderer) {
        std::cerr << "Renderer not initialized" << std::endl;
        return false;
    }
    
    return _renderer->createTriangleResources();
}

void PersTriangleApp::render() {
    // Delegate to renderer
    if (_renderer) {
        _renderer->renderFrame();
    }
}

void PersTriangleApp::cleanup() {
    // Cleanup in reverse order of initialization
    // This ensures proper dependency management
    
    // 1. Triangle resources are cleaned up automatically by renderer destructor
    
    // 2. Renderer cleanup (owns surface reference, uses instance)
    // Must be destroyed before instance to avoid dangling references
    _renderer.reset();
    
    // 3. Surface is automatically cleaned up by instance when it's destroyed
    
    // 4. Instance cleanup (created from factory)
    // Must be destroyed before factory
    _instance.reset();
    
    // 5. Graphics factory cleanup (shared, may still be referenced elsewhere)
    _graphicsFactory.reset();
    
    // 6. Window cleanup (owned by app)
    _window.reset();
    
    // 7. Window factory cleanup (shared, may still be referenced elsewhere)
    _windowFactory.reset();
    
    std::cout << "[PersTriangleApp] Cleanup completed" << std::endl;
}

// Event handlers
void PersTriangleApp::handleResize(int width, int height) {
    // Forward to renderer
    if (_renderer) {
        _renderer->onResize(width, height);
    }
}

void PersTriangleApp::handleKeyPress(int key, int scancode, int action, int mods) {
    if (action == 1) { // GLFW_PRESS = 1
        switch (key) {
            case 256: // GLFW_KEY_ESCAPE = 256
                _window->setShouldClose(true);
                break;
            case 70: // GLFW_KEY_F = 70
                std::cout << "F key pressed" << std::endl;
                break;
        }
    }
}