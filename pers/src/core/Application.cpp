#include "pers/core/Application.h"
#include "pers/core/IWindow.h"
#include "pers/core/IWindowFactory.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/utils/Logger.h"
#include <chrono>

Application::Application() = default;

Application::~Application() {
    cleanup();
}

bool Application::initialize(const std::shared_ptr<IWindowFactory>& windowFactory, 
                            const std::shared_ptr<pers::IGraphicsBackendFactory>& graphicsFactory) {
    LOG_INFO("Application", "=== Application Initialization ===");
    
    // Store factories
    _windowFactory = windowFactory;
    _graphicsFactory = graphicsFactory;
    
    if (!_windowFactory) {
        LOG_ERROR("Application", "Invalid window factory provided");
        return false;
    }
    
    if (!_graphicsFactory) {
        LOG_ERROR("Application", "Invalid graphics factory provided");
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
        LOG_ERROR("Application", "Derived class initialization failed");
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
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "Application", PERS_SOURCE_LOC,
        "Creating window using %s factory", _windowFactory->getFactoryName());
    
    _window = _windowFactory->createWindow(_windowWidth, _windowHeight, _windowTitle);
    if (!_window || !_window->isValid()) {
        LOG_ERROR("Application", "Failed to create window");
        return false;
    }
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "Application", PERS_SOURCE_LOC,
        "Window created: %dx%d", _windowWidth, _windowHeight);
    return true;
}

bool Application::setupWindowCallbacks() {
    if (!_window || !_window->isValid()) {
        LOG_ERROR("Application", "Invalid window");
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
    LOG_INFO("Application", "Creating graphics instance");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "Application", PERS_SOURCE_LOC,
        "Using backend: %s", _graphicsFactory->getBackendName().c_str());
    
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
        LOG_ERROR("Application", "Failed to create instance");
        return false;
    }
    
    LOG_INFO("Application", "Instance created successfully");
    return true;
}

pers::NativeSurfaceHandle Application::createSurface() const {
    if (!_instance) {
        LOG_ERROR("Application", "Instance not initialized");
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    if (!_window) {
        LOG_ERROR("Application", "Window not initialized");
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    // Get native handle from window
    pers::NativeWindowHandle nativeHandle = _window->getNativeHandle();
    
    // Create surface using the instance
    pers::NativeSurfaceHandle surface = _instance->createSurface(&nativeHandle);
    if (!surface.isValid()) {
        LOG_ERROR("Application", "Failed to create surface");
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    LOG_INFO("Application", "Surface created successfully");
    return surface;
}

void Application::cleanup() {
    LOG_INFO("Application", "Starting cleanup");
    
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
    
    LOG_INFO("Application", "Cleanup completed");
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