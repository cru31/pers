#include "pers/core/Application.h"
#include "pers/core/IWindow.h"
#include "pers/core/IWindowFactory.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include <iostream>
#include <chrono>

Application::Application() = default;

Application::~Application() {
    cleanup();
}

bool Application::initialize(std::shared_ptr<IWindowFactory> windowFactory, 
                            std::shared_ptr<pers::IGraphicsBackendFactory> graphicsFactory) {
    std::cout << "=== Application Initialization ===" << std::endl;
    
    // Store factories
    _windowFactory = windowFactory;
    _graphicsFactory = graphicsFactory;
    
    if (!_windowFactory) {
        std::cerr << "[Application] Invalid window factory provided" << std::endl;
        return false;
    }
    
    if (!_graphicsFactory) {
        std::cerr << "[Application] Invalid graphics factory provided" << std::endl;
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
    
    // Call derived class initialization
    if (!onInitialize()) {
        std::cerr << "[Application] Derived class initialization failed" << std::endl;
        return false;
    }
    
    return true;
}

void Application::run() {
    auto lastTime = std::chrono::steady_clock::now();
    
    while (!_window->shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Poll events
        _window->pollEvents();
        
        // Update and render
        onUpdate(deltaTime);
        onRender();
    }
}

glm::ivec2 Application::getFramebufferSize() const {
    if (!_window) {
        return glm::ivec2(0, 0);
    }
    return _window->getFramebufferSize();
}

bool Application::createWindow() {
    std::cout << "[Application] Creating window using " << _windowFactory->getFactoryName() << " factory" << std::endl;
    
    _window = _windowFactory->createWindow(_windowWidth, _windowHeight, _windowTitle);
    if (!_window || !_window->isValid()) {
        std::cerr << "[Application] Failed to create window" << std::endl;
        return false;
    }
    
    std::cout << "[Application] Window created: " << _windowWidth << "x" << _windowHeight << std::endl;
    return true;
}

bool Application::setupWindowCallbacks() {
    if (!_window || !_window->isValid()) {
        std::cerr << "[Application] Invalid window" << std::endl;
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

bool Application::createInstance() {
    std::cout << "[Application] Creating graphics instance" << std::endl;
    std::cout << "[Application] Using backend: " << _graphicsFactory->getBackendName() << std::endl;
    
    // Create graphics instance
    pers::InstanceDesc instanceDesc;
    instanceDesc.applicationName = _windowTitle;
    instanceDesc.applicationVersion = 1;
    instanceDesc.engineName = "Pers Graphics Engine";
    instanceDesc.engineVersion = 1;
    instanceDesc.enableValidation = true;
    instanceDesc.preferHighPerformanceGPU = true;
    
    _instance = _graphicsFactory->createInstance(instanceDesc);
    if (!_instance) {
        std::cerr << "[Application] Failed to create instance" << std::endl;
        return false;
    }
    
    std::cout << "[Application] Instance created successfully" << std::endl;
    return true;
}

void* Application::createSurface() const {
    if (!_instance) {
        std::cerr << "[Application] Instance not initialized" << std::endl;
        return nullptr;
    }
    
    if (!_window) {
        std::cerr << "[Application] Window not initialized" << std::endl;
        return nullptr;
    }
    
    // Get native handle from window
    pers::NativeWindowHandle nativeHandle = _window->getNativeHandle();
    
    // Create surface using the instance
    void* surface = _instance->createSurface(&nativeHandle);
    if (!surface) {
        std::cerr << "[Application] Failed to create surface" << std::endl;
        return nullptr;
    }
    
    std::cout << "[Application] Surface created successfully" << std::endl;
    return surface;
}

void Application::cleanup() {
    std::cout << "[Application] Starting cleanup" << std::endl;
    
    // Call derived class cleanup first
    onCleanup();
    
    // Clean up graphics instance
    _instance.reset();
    
    // Clean up graphics factory (shared, may still be referenced elsewhere)
    _graphicsFactory.reset();
    
    // Clean up window
    _window.reset();
    
    // Clean up window factory (shared, may still be referenced elsewhere)
    _windowFactory.reset();
    
    std::cout << "[Application] Cleanup completed" << std::endl;
}

void Application::handleResize(int width, int height) {
    // Forward to virtual method
    onResize(width, height);
}

void Application::handleKeyPress(int key, int scancode, int action, int mods) {
    // Default ESC key handling
    if (action == 1 && key == 256) { // GLFW_PRESS = 1, GLFW_KEY_ESCAPE = 256
        _window->setShouldClose(true);
    }
    
    // Forward to virtual method
    onKeyPress(key, scancode, action, mods);
}