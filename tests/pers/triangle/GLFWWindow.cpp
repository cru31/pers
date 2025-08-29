#include "GLFWWindow.h"
#include <iostream>

// Platform-specific includes for native window handles
#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
    #define GLFW_EXPOSE_NATIVE_COCOA
    #include <GLFW/glfw3native.h>
    // Forward declare the function to create Metal layer
    void* createMetalLayer(GLFWwindow* window);
#elif defined(__linux__)
    #define GLFW_EXPOSE_NATIVE_X11
    #include <GLFW/glfw3native.h>
#endif

GLFWWindow::GLFWWindow() {
    // GLFW initialization is now handled by GLFWWindowFactory
}

GLFWWindow::~GLFWWindow() {
    std::cout << "[GLFWWindow] Destructor called" << std::endl;
    destroy();
}

bool GLFWWindow::create(int width, int height, const char* title) {
    if (_window) {
        std::cerr << "[GLFWWindow] Window already created" << std::endl;
        return false;
    }
    
    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        std::cerr << "[GLFWWindow] Failed to create window" << std::endl;
        return false;
    }
    
    // Set user pointer and callbacks
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, onFramebufferSize);
    glfwSetKeyCallback(_window, onKeyPress);
    
    std::cout << "[GLFWWindow] Window created: " << width << "x" << height << std::endl;
    return true;
}

void GLFWWindow::destroy() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
}

bool GLFWWindow::isValid() const {
    return _window != nullptr;
}

glm::ivec2 GLFWWindow::getFramebufferSize() const {
    if (!_window) {
        return glm::ivec2(0, 0);
    }
    
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return glm::ivec2(width, height);
}

bool GLFWWindow::shouldClose() const {
    return _window && glfwWindowShouldClose(_window);
}

void GLFWWindow::setShouldClose(bool shouldClose) {
    if (_window) {
        glfwSetWindowShouldClose(_window, shouldClose ? GLFW_TRUE : GLFW_FALSE);
    }
}

void GLFWWindow::pollEvents() {
    glfwPollEvents();
}

void GLFWWindow::setResizeCallback(ResizeCallback callback) {
    _resizeCallback = callback;
}

void GLFWWindow::setKeyCallback(KeyCallback callback) {
    _keyCallback = callback;
}

void* GLFWWindow::getNativeHandle() const {
    if (!_window) {
        std::cerr << "[GLFWWindow] No window to get native handle from" << std::endl;
        return nullptr;
    }
    
    void* nativeHandle = nullptr;
    
#ifdef _WIN32
    // Windows: Get HWND
    nativeHandle = glfwGetWin32Window(_window);
    if (!nativeHandle) {
        std::cerr << "[GLFWWindow] Failed to get Win32 window handle" << std::endl;
    }
#elif defined(__APPLE__)
    // macOS: Create CAMetalLayer and attach to window
    nativeHandle = createMetalLayer(_window);
    if (!nativeHandle) {
        std::cerr << "[GLFWWindow] Failed to create Metal layer" << std::endl;
    }
#elif defined(__linux__)
    // Linux: Get X11 window
    // TODO: Implement Linux native handle extraction
    std::cerr << "[GLFWWindow] Linux native handle extraction not yet implemented" << std::endl;
#endif
    
    return nativeHandle;
}

// Static callbacks
void GLFWWindow::onFramebufferSize(GLFWwindow* window, int width, int height) {
    GLFWWindow* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->_resizeCallback) {
        self->_resizeCallback(width, height);
    }
}

void GLFWWindow::onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GLFWWindow* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->_keyCallback) {
        self->_keyCallback(key, scancode, action, mods);
    }
}
