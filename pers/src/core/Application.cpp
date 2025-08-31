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

bool Application::initialize(std::shared_ptr<IWindowFactory> windowFactory, 
                            std::shared_ptr<pers::IGraphicsBackendFactory> graphicsFactory) {
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "=== Application Initialization ===", PERS_SOURCE_LOC);
    
    // Store factories
    _windowFactory = windowFactory;
    _graphicsFactory = graphicsFactory;
    
    if (!_windowFactory) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Invalid window factory provided", PERS_SOURCE_LOC);
        return false;
    }
    
    if (!_graphicsFactory) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Invalid graphics factory provided", PERS_SOURCE_LOC);
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
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Derived class initialization failed", PERS_SOURCE_LOC);
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
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Failed to create window", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "Application", PERS_SOURCE_LOC,
        "Window created: %dx%d", _windowWidth, _windowHeight);
    return true;
}

bool Application::setupWindowCallbacks() {
    if (!_window || !_window->isValid()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Invalid window", PERS_SOURCE_LOC);
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
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "Creating graphics instance", PERS_SOURCE_LOC);
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
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Failed to create instance", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "Instance created successfully", PERS_SOURCE_LOC);
    return true;
}

pers::NativeSurfaceHandle Application::createSurface() const {
    if (!_instance) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Instance not initialized", PERS_SOURCE_LOC);
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    if (!_window) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Window not initialized", PERS_SOURCE_LOC);
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    // Get native handle from window
    pers::NativeWindowHandle nativeHandle = _window->getNativeHandle();
    
    // Create surface using the instance
    pers::NativeSurfaceHandle surface = _instance->createSurface(&nativeHandle);
    if (!surface.isValid()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "Application", "Failed to create surface", PERS_SOURCE_LOC);
        return pers::NativeSurfaceHandle(nullptr);
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "Surface created successfully", PERS_SOURCE_LOC);
    return surface;
}

void Application::cleanup() {
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "Starting cleanup", PERS_SOURCE_LOC);
    
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
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "Application", "Cleanup completed", PERS_SOURCE_LOC);
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