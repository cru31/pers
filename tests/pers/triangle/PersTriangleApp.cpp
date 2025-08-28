#include "PersTriangleApp.h"
#include "TriangleRenderer.h"
#include <iostream>

PersTriangleApp::PersTriangleApp() = default;

PersTriangleApp::~PersTriangleApp() {
    cleanup();
}

bool PersTriangleApp::initialize() {
    std::cout << "=== PERS RAL Triangle Example ===" << std::endl;
    
    // Initialize window
    if (!initializeWindow()) {
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

void PersTriangleApp::run() {
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        render();
    }
}

bool PersTriangleApp::initializeWindow() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(_width, _height, "PERS Triangle", nullptr, nullptr);
    if (!_window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Set user pointer and callbacks
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, onFramebufferSize);
    glfwSetKeyCallback(_window, onKeyPress);
    
    return true;
}

bool PersTriangleApp::initializeRenderer() {
    // Create and initialize the renderer
    _renderer = std::make_unique<TriangleRenderer>();
    
    if (!_renderer->initialize(_window)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    return true;
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
    // Clean up renderer first (before destroying window)
    _renderer.reset();
    
    // Then clean up window
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

// Static callbacks
void PersTriangleApp::onFramebufferSize(GLFWwindow* window, int width, int height) {
    PersTriangleApp* app = static_cast<PersTriangleApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleResize(width, height);
    }
}

void PersTriangleApp::onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
    PersTriangleApp* app = static_cast<PersTriangleApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleKeyPress(key, scancode, action, mods);
    }
}

// Instance event handlers
void PersTriangleApp::handleResize(int width, int height) {
    _width = width;
    _height = height;
    
    // Forward to renderer
    if (_renderer) {
        _renderer->onResize(width, height);
    }
}

void PersTriangleApp::handleKeyPress(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(_window, GLFW_TRUE);
                break;
            case GLFW_KEY_F:
                std::cout << "F key pressed" << std::endl;
                break;
        }
    }
}