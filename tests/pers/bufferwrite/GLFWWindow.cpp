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
    // Force X11 only - Wayland detection causes compilation issues
    #define GLFW_EXPOSE_NATIVE_X11
    // Don't define GLFW_EXPOSE_NATIVE_WAYLAND to avoid compilation errors
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
    glfwSetWindowRefreshCallback(_window, onWindowRefresh);
    
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

void GLFWWindow::setRefreshCallback(RefreshCallback callback) {
    _refreshCallback = callback;
}

pers::NativeWindowHandle GLFWWindow::getNativeHandle() const {
    if (!_window) {
        std::cerr << "[GLFWWindow] No window to get native handle from" << std::endl;
        return pers::NativeWindowHandle();  // Return empty handle
    }
    
#ifdef _WIN32
    // Windows: Get HWND
    void* hwnd = glfwGetWin32Window(_window);
    if (!hwnd) {
        std::cerr << "[GLFWWindow] Failed to get Win32 window handle" << std::endl;
        return pers::NativeWindowHandle();
    }
    return pers::NativeWindowHandle::Create(hwnd);
    
#elif defined(__APPLE__)
    // macOS: Create CAMetalLayer and attach to window
    void* metalLayer = createMetalLayer(_window);
    if (!metalLayer) {
        std::cerr << "[GLFWWindow] Failed to create Metal layer" << std::endl;
        return pers::NativeWindowHandle();
    }
    return pers::NativeWindowHandle::Create(metalLayer);
    
#elif defined(__linux__)
    // Linux: Force X11 only (Wayland detection causes compilation issues)
    Display* x11Display = glfwGetX11Display();
    Window x11Window = glfwGetX11Window(_window);
    
    if (!x11Display || !x11Window) {
        std::cerr << "[GLFWWindow] Failed to get X11 handles" << std::endl;
        std::cerr << "[GLFWWindow] Make sure GLFW was built with X11 support" << std::endl;
        return pers::NativeWindowHandle();
    }
    
    std::cout << "[GLFWWindow] Using X11 backend (forced)" << std::endl;
    return pers::NativeWindowHandle::CreateX11(x11Display, reinterpret_cast<void*>(x11Window));
    
#else
    #error "Unsupported platform"
#endif
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

void GLFWWindow::onWindowRefresh(GLFWwindow* window) {
    GLFWWindow* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->_refreshCallback) {
        self->_refreshCallback();
    }
}
