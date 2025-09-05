#include "PersTriangleApp.h"
#include "TriangleRenderer.h"
#include "pers/core/IWindow.h"
#include "pers/utils/Logger.h"
#include <cstdlib>

PersTriangleApp::PersTriangleApp() {
    // Configure window before initialization
    _windowTitle = "PERS Triangle Demo";
    
    // Check if running in CI or test environment
    const char* ciEnv = std::getenv("CI");
    const char* testEnv = std::getenv("PERS_TEST_MODE");
    _isCI = (ciEnv != nullptr) || (testEnv != nullptr);
}

PersTriangleApp::~PersTriangleApp() = default;

bool PersTriangleApp::onInitialize() {
    LOG_INFO("PersTriangleApp",
        "=== PERS Triangle Application ===");
    
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

bool PersTriangleApp::initializeRenderer() {
    // Create and initialize the renderer
    _renderer = std::make_unique<TriangleRenderer>();
    
    // Get framebuffer size to pass to renderer
    glm::ivec2 size = getFramebufferSize();
    
    // Initialize renderer with the instance we got from base class
    if (!_renderer->initialize(getInstance(), size)) {
        LOG_ERROR("PersTriangleApp",
            "Failed to initialize renderer");
        return false;
    }
    
    // Create surface using base class helper and set it on renderer
    pers::NativeSurfaceHandle surface = createSurface();
    if (!surface.isValid()) {
        LOG_ERROR("PersTriangleApp",
            "Failed to create surface");
        return false;
    }
    
    _renderer->setSurface(surface);
    
    return true;
}

bool PersTriangleApp::createTriangle() {
    if (!_renderer) {
        LOG_ERROR("PersTriangleApp",
            "Renderer not initialized");
        return false;
    }
    
    return _renderer->createTriangleResources();
}

void PersTriangleApp::onUpdate(float deltaTime) {
    // Exit after timeout in CI environment
    if (_isCI) {
        _ciElapsedTime += deltaTime;
        if (_ciElapsedTime >= CI_MAX_SECONDS) {
            pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "PersTriangleApp", PERS_SOURCE_LOC,
                "CI mode: Exiting after %.1f seconds", CI_MAX_SECONDS);
            getWindow()->setShouldClose(true);
        }
    }
}

void PersTriangleApp::onRender() {
    if (_renderer) {
        _renderer->renderFrame();
    }
}

void PersTriangleApp::onResize(int width, int height) {
    // Forward to renderer
    if (_renderer) {
        _renderer->onResize(width, height);
    }
}

void PersTriangleApp::onKeyPress(int key, int scancode, int action, int mods) {
    // Additional key handling (ESC is handled by base class)
    if (action == 1) { // GLFW_PRESS = 1
        switch (key) {
            case 70: // GLFW_KEY_F = 70
                LOG_DEBUG("PersTriangleApp", "F key pressed");
                break;
        }
    }
}

void PersTriangleApp::onCleanup() {
    LOG_INFO("PersTriangleApp",
        "Cleaning up triangle resources");
    
    // Clean up renderer (must be before instance is destroyed)
    _renderer.reset();
}