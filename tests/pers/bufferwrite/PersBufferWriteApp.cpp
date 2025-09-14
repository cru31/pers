#include "PersBufferWriteApp.h"
#include "BufferWriteRenderer.h"
#include "pers/core/IWindow.h"
#include "pers/utils/Logger.h"
#include <cstdlib>

PersBufferWriteApp::PersBufferWriteApp() {
    // Configure window before initialization
    _windowTitle = "PERS BufferWrite Demo";
    
    // Check if running in CI or test environment
    const char* ciEnv = std::getenv("CI");
    const char* testEnv = std::getenv("PERS_TEST_MODE");
    _isCI = (ciEnv != nullptr) || (testEnv != nullptr);
}

PersBufferWriteApp::~PersBufferWriteApp() = default;

bool PersBufferWriteApp::onInitialize() {
    LOG_INFO("PersBufferWriteApp",
        "=== PERS BufferWrite Application ===");
    
    // Initialize renderer
    if (!initializeRenderer()) {
        return false;
    }
    
    // Create bufferwrite resources
    if (!createBufferWrite()) {
        return false;
    }
    
    return true;
}

bool PersBufferWriteApp::initializeRenderer() {
    // Create and initialize the renderer
    _renderer = std::make_unique<BufferWriteRenderer>();
    
    // Get framebuffer size to pass to renderer
    glm::ivec2 size = getFramebufferSize();
    
    // Create configuration for renderer
    BufferWriteRendererConfig config;
    config.windowSize = size;
    
    // Initialize renderer with the instance we got from base class
    if (!_renderer->initialize(getInstance(), config)) {
        LOG_ERROR("PersBufferWriteApp",
            "Failed to initialize renderer");
        return false;
    }
    
    // Create surface using base class helper and initialize graphics
    pers::NativeSurfaceHandle surface = createSurface();
    if (!surface.isValid()) {
        LOG_ERROR("PersBufferWriteApp",
            "Failed to create surface");
        return false;
    }
    
    if (!_renderer->initializeGraphics(surface)) {
        LOG_ERROR("PersBufferWriteApp",
            "Failed to initialize graphics");
        return false;
    }
    
    return true;
}

bool PersBufferWriteApp::createBufferWrite() {
    if (!_renderer) {
        LOG_ERROR("PersBufferWriteApp",
            "Renderer not initialized");
        return false;
    }
    
    return _renderer->createGraphicsResources();
}

void PersBufferWriteApp::onUpdate(float deltaTime) {
    // Exit after timeout in CI environment
    if (_isCI) {
        _ciElapsedTime += deltaTime;
        if (_ciElapsedTime >= CI_MAX_SECONDS) {
            pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "PersBufferWriteApp", PERS_SOURCE_LOC,
                "CI mode: Exiting after %.1f seconds", CI_MAX_SECONDS);
            getWindow()->setShouldClose(true);
        }
    }
}

void PersBufferWriteApp::onRender() {
    if (_renderer) {
        _renderer->renderFrame();
    }
}

void PersBufferWriteApp::onResize(int width, int height) {
    // Forward to renderer
    if (_renderer) {
        _renderer->onResize(width, height);
    }
}

void PersBufferWriteApp::onKeyPress(int key, int scancode, int action, int mods) {
    // Additional key handling (ESC is handled by base class)
    if (action == 1) { // GLFW_PRESS = 1
        switch (key) {
            case 70: // GLFW_KEY_F = 70
                LOG_DEBUG("PersBufferWriteApp", "F key pressed");
                break;
        }
    }
}

void PersBufferWriteApp::onCleanup() {
    LOG_INFO("PersBufferWriteApp",
        "Cleaning up bufferwrite resources");
    
    // Clean up renderer (must be before instance is destroyed)
    _renderer.reset();
}