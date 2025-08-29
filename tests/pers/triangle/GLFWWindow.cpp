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
    // Linux: Check which windowing system GLFW is using
    
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 4
    // GLFW 3.4+ has glfwGetPlatform()
    int platform = glfwGetPlatform();
    
    switch (platform) {
        case GLFW_PLATFORM_X11: {
            Display* x11Display = glfwGetX11Display();
            Window x11Window = glfwGetX11Window(_window);
            
            if (!x11Display || !x11Window) {
                std::cerr << "[GLFWWindow] Failed to get X11 handles" << std::endl;
                return pers::NativeWindowHandle();
            }
            
            std::cout << "[GLFWWindow] Using X11 backend (via glfwGetPlatform)" << std::endl;
            return pers::NativeWindowHandle::CreateX11(x11Display, reinterpret_cast<void*>(x11Window));
        }
        
        case GLFW_PLATFORM_WAYLAND: {
            struct wl_display* wlDisplay = glfwGetWaylandDisplay();
            struct wl_surface* wlSurface = glfwGetWaylandWindow(_window);
            
            if (!wlDisplay || !wlSurface) {
                std::cerr << "[GLFWWindow] Failed to get Wayland handles" << std::endl;
                return pers::NativeWindowHandle();
            }
            
            std::cout << "[GLFWWindow] Using Wayland backend (via glfwGetPlatform)" << std::endl;
            return pers::NativeWindowHandle::CreateWayland(wlDisplay, wlSurface);
        }
        
        default:
            std::cerr << "[GLFWWindow] Unsupported platform: " << platform << std::endl;
            return pers::NativeWindowHandle();
    }
#else
    // GLFW 3.3 and older - fallback to checking both
    Display* x11Display = glfwGetX11Display();
    struct wl_display* wlDisplay = glfwGetWaylandDisplay();
    
    // Check for unexpected situation where both are available
    if (x11Display && wlDisplay) {
        std::cerr << "[GLFWWindow] WARNING: Both X11 and Wayland displays detected, defaulting to X11" << std::endl;
        Window x11Window = glfwGetX11Window(_window);
        if (!x11Window) {
            std::cerr << "[GLFWWindow] Failed to get X11 window handle" << std::endl;
            return pers::NativeWindowHandle();
        }
        return pers::NativeWindowHandle::CreateX11(x11Display, reinterpret_cast<void*>(x11Window));
    }
    
    // Normal case: only one is available
    if (x11Display) {
        Window x11Window = glfwGetX11Window(_window);
        if (!x11Window) {
            std::cerr << "[GLFWWindow] Failed to get X11 window handle" << std::endl;
            return pers::NativeWindowHandle();
        }
        std::cout << "[GLFWWindow] Using X11 backend" << std::endl;
        return pers::NativeWindowHandle::CreateX11(x11Display, reinterpret_cast<void*>(x11Window));
    }
    
    if (wlDisplay) {
        struct wl_surface* wlSurface = glfwGetWaylandWindow(_window);
        if (!wlSurface) {
            std::cerr << "[GLFWWindow] Failed to get Wayland surface" << std::endl;
            return pers::NativeWindowHandle();
        }
        std::cout << "[GLFWWindow] Using Wayland backend" << std::endl;
        return pers::NativeWindowHandle::CreateWayland(wlDisplay, wlSurface);
    }
    
    std::cerr << "[GLFWWindow] Could not detect windowing system (neither X11 nor Wayland)" << std::endl;
    return pers::NativeWindowHandle();
#endif
    
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
