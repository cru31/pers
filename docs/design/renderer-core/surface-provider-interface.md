# Surface Provider and Event Handler Interface Design

## Overview

The Surface Provider and Event Handler interfaces handle window system integration, surface lifecycle management, and input event handling. These are completely separate from the graphics backend interfaces.

## Architecture

```
┌─────────────────┐          ┌──────────────────────────────────┐
│   Application   │          │         Pers Engine              │
│                 │          │                                  │
│  ┌───────────┐  │          │  ┌────────────────────────────┐ │
│  │   GLFW    │  │          │  │       Event Handlers       │ │
│  └───────────┘  │          │  │  - ISurfaceEventHandler    │ │
│        │        │          │  │  - IInputEventHandler      │ │
│        ▼        │          │  │  - IApplicationHandler     │ │
│  ┌───────────┐  │  passes  │  └────────────────────────────┘ │
│  │  Surface  │──┼──────────┼─►          ▲                    │
│  │  Provider │  │interface │            │                    │
│  └───────────┘  │          │            │                    │
│        │        │          │  ┌────────────────────────────┐ │
│        ▼        │  passes  │  │       Renderer             │ │
│  ┌───────────┐  │  surface │  │  (implements handlers)     │ │
│  │   Event   │──┼──────────┼─►│                            │ │
│  │ Forwarder │  │  events  │  └────────────────────────────┘ │
│  └───────────┘  │          │                                  │
└─────────────────┘          └──────────────────────────────────┘
```

## Core Interfaces

### ISurfaceProvider

```cpp
namespace pers {
namespace renderer {

// Surface provider interface - handles window system surface creation
// Completely separate from graphics backend
class ISurfaceProvider {
public:
    virtual ~ISurfaceProvider() = default;
    
    // Get opaque surface handle for graphics backend
    // Returns platform-specific surface (WGPUSurface, VkSurfaceKHR, etc.)
    // as void* to maintain abstraction
    virtual void* GetNativeSurfaceHandle() const = 0;
    
    // Surface properties
    virtual void GetSurfaceSize(uint32_t& width, uint32_t& height) const = 0;
    virtual float GetSurfaceScale() const = 0;
    
    // Surface state
    virtual bool IsSurfaceValid() const = 0;
    virtual bool IsSurfaceMinimized() const = 0;
    virtual bool IsSurfaceOccluded() const = 0;
    
    // Platform information
    virtual PlatformType GetPlatformType() const = 0;
    virtual std::string GetPlatformDescription() const = 0;
};

// Platform types
enum class PlatformType {
    Windows,
    MacOS,
    Linux_X11,
    Linux_Wayland,
    Android,
    iOS,
    Web,
    Headless
};

} // namespace renderer
} // namespace pers
```

### Event Handler Interfaces

```cpp
namespace pers {
namespace renderer {

// Surface event handler - Engine implements this to handle surface changes
class ISurfaceEventHandler {
public:
    virtual ~ISurfaceEventHandler() = default;
    
    // Surface lifecycle events
    virtual void OnSurfaceResized(uint32_t width, uint32_t height) = 0;
    virtual void OnSurfaceScaleChanged(float scale) = 0;
    virtual void OnSurfaceMinimized() = 0;
    virtual void OnSurfaceRestored() = 0;
    virtual void OnSurfaceLost() = 0;
    virtual void OnSurfaceRecreated() = 0;
    
    // Frame events
    virtual void OnFrameRequested() = 0;  // For event-driven rendering
};

// Input event handler - Engine implements this to handle input
class IInputEventHandler {
public:
    virtual ~IInputEventHandler() = default;
    
    // Keyboard events
    virtual void OnKeyDown(KeyCode key, KeyModifiers mods, bool repeat) = 0;
    virtual void OnKeyUp(KeyCode key, KeyModifiers mods) = 0;
    virtual void OnCharInput(uint32_t codepoint) = 0;
    
    // Mouse events
    virtual void OnMouseMove(float x, float y) = 0;
    virtual void OnMouseDown(MouseButton button, float x, float y, KeyModifiers mods) = 0;
    virtual void OnMouseUp(MouseButton button, float x, float y, KeyModifiers mods) = 0;
    virtual void OnMouseScroll(float deltaX, float deltaY) = 0;
    virtual void OnMouseEnter() = 0;
    virtual void OnMouseLeave() = 0;
    
    // Touch events (for mobile/touch screens)
    virtual void OnTouchStart(uint32_t id, float x, float y) = 0;
    virtual void OnTouchMove(uint32_t id, float x, float y) = 0;
    virtual void OnTouchEnd(uint32_t id, float x, float y) = 0;
    virtual void OnTouchCancel(uint32_t id) = 0;
};

// Application lifecycle handler
class IApplicationHandler {
public:
    virtual ~IApplicationHandler() = default;
    
    // Application events
    virtual void OnApplicationPause() = 0;
    virtual void OnApplicationResume() = 0;
    virtual void OnApplicationTerminate() = 0;
    virtual void OnMemoryWarning() = 0;
    
    // File events
    virtual void OnFilesDropped(const std::vector<std::string>& paths) = 0;
};

// Input enums
enum class KeyCode {
    Unknown = 0,
    
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8,
    F9, F10, F11, F12, F13, F14, F15,
    
    // Navigation
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    
    // Editing
    Backspace, Tab, Enter, Delete,
    Insert, Escape, Space,
    
    // Modifiers
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,
    
    // Special
    CapsLock, ScrollLock, NumLock,
    PrintScreen, Pause, Menu,
    
    Count
};

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7,
    Count = 8
};

// Modifier flags (can be combined)
enum class KeyModifiers : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

// Enable bitwise operations for KeyModifiers
inline KeyModifiers operator|(KeyModifiers a, KeyModifiers b) {
    return static_cast<KeyModifiers>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline KeyModifiers operator&(KeyModifiers a, KeyModifiers b) {
    return static_cast<KeyModifiers>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasModifier(KeyModifiers mods, KeyModifiers check) {
    return (mods & check) != KeyModifiers::None;
}

} // namespace renderer
} // namespace pers
```

## Application-Side Implementations

### GLFW Surface Provider

```cpp
// In application code (samples/platforms/glfw/)
class GLFWSurfaceProvider : public pers::renderer::ISurfaceProvider {
private:
    GLFWwindow* _window;
    void* _nativeSurfaceHandle = nullptr;
    
    // Graphics instance for surface creation
    std::weak_ptr<pers::renderer::IInstance> _graphicsInstance;
    pers::renderer::GraphicsBackendType _backendType;
    
public:
    explicit GLFWSurfaceProvider(GLFWwindow* window) 
        : _window(window) {
        
        if (!_window) {
            throw std::invalid_argument("Window cannot be null");
        }
    }
    
    ~GLFWSurfaceProvider() {
        // Clean up surface if needed
        if (_nativeSurfaceHandle) {
            DestroySurface();
        }
    }
    
    // Initialize surface for specific graphics backend
    void InitializeSurface(
        std::shared_ptr<pers::renderer::IInstance> instance,
        pers::renderer::GraphicsBackendType backendType) {
        
        _graphicsInstance = instance;
        _backendType = backendType;
        
        switch (backendType) {
            case pers::renderer::GraphicsBackendType::WebGPU:
                _nativeSurfaceHandle = CreateWebGPUSurface(instance);
                break;
                
            case pers::renderer::GraphicsBackendType::Vulkan:
                _nativeSurfaceHandle = CreateVulkanSurface(instance);
                break;
                
            default:
                throw std::runtime_error("Unsupported graphics backend");
        }
    }
    
    void* GetNativeSurfaceHandle() const override {
        return _nativeSurfaceHandle;
    }
    
    void GetSurfaceSize(uint32_t& width, uint32_t& height) const override {
        int w, h;
        glfwGetFramebufferSize(_window, &w, &h);
        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
    }
    
    float GetSurfaceScale() const override {
        float xscale, yscale;
        glfwGetWindowContentScale(_window, &xscale, &yscale);
        return xscale;  // Assuming uniform scaling
    }
    
    bool IsSurfaceValid() const override {
        return _nativeSurfaceHandle != nullptr && 
               !glfwWindowShouldClose(_window);
    }
    
    bool IsSurfaceMinimized() const override {
        return glfwGetWindowAttrib(_window, GLFW_ICONIFIED) != 0;
    }
    
    bool IsSurfaceOccluded() const override {
        // GLFW doesn't directly support occlusion detection
        return false;
    }
    
    PlatformType GetPlatformType() const override {
        #ifdef _WIN32
            return PlatformType::Windows;
        #elif defined(__APPLE__)
            return PlatformType::MacOS;
        #elif defined(__linux__)
            // Could detect Wayland vs X11
            return PlatformType::Linux_X11;
        #else
            return PlatformType::Unknown;
        #endif
    }
    
    std::string GetPlatformDescription() const override {
        return std::string("GLFW ") + glfwGetVersionString();
    }
    
private:
    void* CreateWebGPUSurface(std::shared_ptr<pers::renderer::IInstance> instance) {
        // Need to cast to WebGPU instance to get native handle
        // This is application-side code so it's OK to know about WebGPU types
        auto webgpuInstance = std::dynamic_pointer_cast<
            pers::renderer::backends::webgpu::WebGPUInstance>(instance);
        
        if (!webgpuInstance) {
            throw std::runtime_error("Invalid WebGPU instance");
        }
        
        WGPUInstance wgpuInstance = webgpuInstance->GetNativeHandle();
        
        #ifdef _WIN32
            WGPUSurfaceDescriptorFromWindowsHWND hwndDesc{};
            hwndDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
            hwndDesc.hwnd = glfwGetWin32Window(_window);
            hwndDesc.hinstance = GetModuleHandle(nullptr);
            
            WGPUSurfaceDescriptor surfaceDesc{};
            surfaceDesc.nextInChain = &hwndDesc.chain;
            
            return wgpuInstanceCreateSurface(wgpuInstance, &surfaceDesc);
            
        #elif defined(__APPLE__)
            // macOS: Create CAMetalLayer
            id metalLayer = CreateMetalLayer(_window);
            
            WGPUSurfaceDescriptorFromMetalLayer metalDesc{};
            metalDesc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
            metalDesc.layer = metalLayer;
            
            WGPUSurfaceDescriptor surfaceDesc{};
            surfaceDesc.nextInChain = &metalDesc.chain;
            
            return wgpuInstanceCreateSurface(wgpuInstance, &surfaceDesc);
            
        #elif defined(__linux__)
            #ifdef GLFW_EXPOSE_NATIVE_WAYLAND
                if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
                    WGPUSurfaceDescriptorFromWaylandSurface waylandDesc{};
                    waylandDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
                    waylandDesc.display = glfwGetWaylandDisplay();
                    waylandDesc.surface = glfwGetWaylandWindow(_window);
                    
                    WGPUSurfaceDescriptor surfaceDesc{};
                    surfaceDesc.nextInChain = &waylandDesc.chain;
                    
                    return wgpuInstanceCreateSurface(wgpuInstance, &surfaceDesc);
                }
            #endif
            
            // X11 fallback
            WGPUSurfaceDescriptorFromXlibWindow x11Desc{};
            x11Desc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
            x11Desc.display = glfwGetX11Display();
            x11Desc.window = glfwGetX11Window(_window);
            
            WGPUSurfaceDescriptor surfaceDesc{};
            surfaceDesc.nextInChain = &x11Desc.chain;
            
            return wgpuInstanceCreateSurface(wgpuInstance, &surfaceDesc);
        #endif
    }
    
    void* CreateVulkanSurface(std::shared_ptr<pers::renderer::IInstance> instance) {
        // Vulkan surface creation
        VkSurfaceKHR surface;
        // Use glfwCreateWindowSurface for Vulkan
        // ...
        return surface;
    }
    
    void DestroySurface() {
        // Clean up based on backend type
        // ...
        _nativeSurfaceHandle = nullptr;
    }
};
```

### GLFW Event Forwarder

```cpp
// In application code (samples/platforms/glfw/)
class GLFWEventForwarder {
private:
    GLFWwindow* _window;
    std::weak_ptr<pers::renderer::ISurfaceEventHandler> _surfaceHandler;
    std::weak_ptr<pers::renderer::IInputEventHandler> _inputHandler;
    std::weak_ptr<pers::renderer::IApplicationHandler> _appHandler;
    
public:
    GLFWEventForwarder(GLFWwindow* window) : _window(window) {
        // Set up GLFW callbacks
        glfwSetWindowUserPointer(window, this);
        
        // Window callbacks
        glfwSetFramebufferSizeCallback(window, OnFramebufferSize);
        glfwSetWindowContentScaleCallback(window, OnContentScale);
        glfwSetWindowIconifyCallback(window, OnIconify);
        glfwSetWindowMaximizeCallback(window, OnMaximize);
        glfwSetWindowFocusCallback(window, OnFocus);
        glfwSetWindowRefreshCallback(window, OnRefresh);
        
        // Input callbacks
        glfwSetKeyCallback(window, OnKey);
        glfwSetCharCallback(window, OnChar);
        glfwSetCursorPosCallback(window, OnCursorPos);
        glfwSetMouseButtonCallback(window, OnMouseButton);
        glfwSetScrollCallback(window, OnScroll);
        glfwSetCursorEnterCallback(window, OnCursorEnter);
        
        // File drop callback
        glfwSetDropCallback(window, OnDrop);
    }
    
    void SetSurfaceEventHandler(std::shared_ptr<ISurfaceEventHandler> handler) {
        _surfaceHandler = handler;
    }
    
    void SetInputEventHandler(std::shared_ptr<IInputEventHandler> handler) {
        _inputHandler = handler;
    }
    
    void SetApplicationHandler(std::shared_ptr<IApplicationHandler> handler) {
        _appHandler = handler;
    }
    
private:
    // Window callbacks
    static void OnFramebufferSize(GLFWwindow* window, int width, int height) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_surfaceHandler.lock()) {
            handler->OnSurfaceResized(
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            );
        }
    }
    
    static void OnContentScale(GLFWwindow* window, float xscale, float yscale) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_surfaceHandler.lock()) {
            handler->OnSurfaceScaleChanged(xscale);
        }
    }
    
    static void OnIconify(GLFWwindow* window, int iconified) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_surfaceHandler.lock()) {
            if (iconified) {
                handler->OnSurfaceMinimized();
            } else {
                handler->OnSurfaceRestored();
            }
        }
    }
    
    static void OnRefresh(GLFWwindow* window) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_surfaceHandler.lock()) {
            handler->OnFrameRequested();
        }
    }
    
    // Input callbacks
    static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            KeyCode keyCode = ConvertGLFWKey(key);
            KeyModifiers modifiers = ConvertGLFWMods(mods);
            
            if (action == GLFW_PRESS) {
                handler->OnKeyDown(keyCode, modifiers, false);
            } else if (action == GLFW_RELEASE) {
                handler->OnKeyUp(keyCode, modifiers);
            } else if (action == GLFW_REPEAT) {
                handler->OnKeyDown(keyCode, modifiers, true);
            }
        }
    }
    
    static void OnChar(GLFWwindow* window, unsigned int codepoint) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            handler->OnCharInput(codepoint);
        }
    }
    
    static void OnCursorPos(GLFWwindow* window, double xpos, double ypos) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            handler->OnMouseMove(
                static_cast<float>(xpos),
                static_cast<float>(ypos)
            );
        }
    }
    
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            
            MouseButton mouseButton = ConvertGLFWButton(button);
            KeyModifiers modifiers = ConvertGLFWMods(mods);
            
            if (action == GLFW_PRESS) {
                handler->OnMouseDown(
                    mouseButton,
                    static_cast<float>(xpos),
                    static_cast<float>(ypos),
                    modifiers
                );
            } else if (action == GLFW_RELEASE) {
                handler->OnMouseUp(
                    mouseButton,
                    static_cast<float>(xpos),
                    static_cast<float>(ypos),
                    modifiers
                );
            }
        }
    }
    
    static void OnScroll(GLFWwindow* window, double xoffset, double yoffset) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            handler->OnMouseScroll(
                static_cast<float>(xoffset),
                static_cast<float>(yoffset)
            );
        }
    }
    
    static void OnCursorEnter(GLFWwindow* window, int entered) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_inputHandler.lock()) {
            if (entered) {
                handler->OnMouseEnter();
            } else {
                handler->OnMouseLeave();
            }
        }
    }
    
    static void OnDrop(GLFWwindow* window, int count, const char** paths) {
        auto* forwarder = static_cast<GLFWEventForwarder*>(
            glfwGetWindowUserPointer(window));
        
        if (auto handler = forwarder->_appHandler.lock()) {
            std::vector<std::string> filePaths;
            for (int i = 0; i < count; ++i) {
                filePaths.push_back(paths[i]);
            }
            handler->OnFilesDropped(filePaths);
        }
    }
    
    // Key/Button conversion helpers
    static KeyCode ConvertGLFWKey(int key) {
        // Convert GLFW key codes to engine key codes
        switch (key) {
            case GLFW_KEY_A: return KeyCode::A;
            case GLFW_KEY_B: return KeyCode::B;
            // ... etc
            default: return KeyCode::Unknown;
        }
    }
    
    static MouseButton ConvertGLFWButton(int button) {
        if (button >= 0 && button < static_cast<int>(MouseButton::Count)) {
            return static_cast<MouseButton>(button);
        }
        return MouseButton::Left;
    }
    
    static KeyModifiers ConvertGLFWMods(int mods) {
        KeyModifiers result = KeyModifiers::None;
        
        if (mods & GLFW_MOD_SHIFT) result = result | KeyModifiers::Shift;
        if (mods & GLFW_MOD_CONTROL) result = result | KeyModifiers::Control;
        if (mods & GLFW_MOD_ALT) result = result | KeyModifiers::Alt;
        if (mods & GLFW_MOD_SUPER) result = result | KeyModifiers::Super;
        if (mods & GLFW_MOD_CAPS_LOCK) result = result | KeyModifiers::CapsLock;
        if (mods & GLFW_MOD_NUM_LOCK) result = result | KeyModifiers::NumLock;
        
        return result;
    }
};
```

## Engine-Side Implementation

```cpp
namespace pers {
namespace renderer {

// Renderer implements all event handlers
class Renderer : public ISurfaceEventHandler,
                 public IInputEventHandler,
                 public IApplicationHandler {
private:
    // Surface provider (separate from graphics)
    std::shared_ptr<ISurfaceProvider> _surfaceProvider;
    
    // Graphics backend (separate from surface)
    std::shared_ptr<IGraphicsBackendFactory> _graphicsFactory;
    std::shared_ptr<ILogicalDevice> _device;
    std::shared_ptr<ISwapChain> _swapChain;
    
    // Renderer state
    uint32_t _surfaceWidth = 0;
    uint32_t _surfaceHeight = 0;
    float _surfaceScale = 1.0f;
    bool _isMinimized = false;
    bool _needsResize = false;
    
public:
    Renderer(const RendererCreateInfo& createInfo)
        : _surfaceProvider(createInfo.surfaceProvider)
        , _graphicsFactory(createInfo.graphicsFactory) {
        
        Initialize();
    }
    
    // ISurfaceEventHandler implementation
    void OnSurfaceResized(uint32_t width, uint32_t height) override {
        if (width == 0 || height == 0) {
            _isMinimized = true;
            return;
        }
        
        _surfaceWidth = width;
        _surfaceHeight = height;
        _needsResize = true;
        _isMinimized = false;
        
        Logger::Info("Surface resized to {}x{}", width, height);
    }
    
    void OnSurfaceScaleChanged(float scale) override {
        _surfaceScale = scale;
        _needsResize = true;
        
        Logger::Info("Surface scale changed to {}", scale);
    }
    
    void OnSurfaceMinimized() override {
        _isMinimized = true;
        Logger::Info("Surface minimized");
    }
    
    void OnSurfaceRestored() override {
        _isMinimized = false;
        _needsResize = true;
        Logger::Info("Surface restored");
    }
    
    void OnSurfaceLost() override {
        // Handle surface loss (e.g., GPU reset)
        _swapChain.reset();
        Logger::Warning("Surface lost - need to recreate swap chain");
    }
    
    void OnSurfaceRecreated() override {
        // Recreate swap chain with new surface
        RecreateSwapChain();
        Logger::Info("Surface recreated");
    }
    
    void OnFrameRequested() override {
        // Render a frame (for event-driven rendering)
        if (!_isMinimized) {
            RenderFrame();
        }
    }
    
    // IInputEventHandler implementation
    void OnKeyDown(KeyCode key, KeyModifiers mods, bool repeat) override {
        // Forward to input system or game logic
        if (_inputSystem) {
            _inputSystem->HandleKeyDown(key, mods, repeat);
        }
    }
    
    void OnKeyUp(KeyCode key, KeyModifiers mods) override {
        if (_inputSystem) {
            _inputSystem->HandleKeyUp(key, mods);
        }
    }
    
    void OnCharInput(uint32_t codepoint) override {
        if (_inputSystem) {
            _inputSystem->HandleCharInput(codepoint);
        }
    }
    
    void OnMouseMove(float x, float y) override {
        if (_inputSystem) {
            _inputSystem->HandleMouseMove(x, y);
        }
    }
    
    void OnMouseDown(MouseButton button, float x, float y, KeyModifiers mods) override {
        if (_inputSystem) {
            _inputSystem->HandleMouseDown(button, x, y, mods);
        }
    }
    
    void OnMouseUp(MouseButton button, float x, float y, KeyModifiers mods) override {
        if (_inputSystem) {
            _inputSystem->HandleMouseUp(button, x, y, mods);
        }
    }
    
    void OnMouseScroll(float deltaX, float deltaY) override {
        if (_inputSystem) {
            _inputSystem->HandleMouseScroll(deltaX, deltaY);
        }
    }
    
    void OnMouseEnter() override {
        if (_inputSystem) {
            _inputSystem->HandleMouseEnter();
        }
    }
    
    void OnMouseLeave() override {
        if (_inputSystem) {
            _inputSystem->HandleMouseLeave();
        }
    }
    
    // Touch events
    void OnTouchStart(uint32_t id, float x, float y) override {
        if (_inputSystem) {
            _inputSystem->HandleTouchStart(id, x, y);
        }
    }
    
    void OnTouchMove(uint32_t id, float x, float y) override {
        if (_inputSystem) {
            _inputSystem->HandleTouchMove(id, x, y);
        }
    }
    
    void OnTouchEnd(uint32_t id, float x, float y) override {
        if (_inputSystem) {
            _inputSystem->HandleTouchEnd(id, x, y);
        }
    }
    
    void OnTouchCancel(uint32_t id) override {
        if (_inputSystem) {
            _inputSystem->HandleTouchCancel(id);
        }
    }
    
    // IApplicationHandler implementation
    void OnApplicationPause() override {
        _isPaused = true;
        Logger::Info("Application paused");
    }
    
    void OnApplicationResume() override {
        _isPaused = false;
        Logger::Info("Application resumed");
    }
    
    void OnApplicationTerminate() override {
        _shouldExit = true;
        Logger::Info("Application terminating");
    }
    
    void OnMemoryWarning() override {
        // Release non-essential resources
        Logger::Warning("Memory warning - releasing cached resources");
        if (_resourceManager) {
            _resourceManager->ReleaseUnusedResources();
        }
    }
    
    void OnFilesDropped(const std::vector<std::string>& paths) override {
        Logger::Info("Files dropped: {} files", paths.size());
        for (const auto& path : paths) {
            Logger::Info("  - {}", path);
        }
        
        // Forward to asset system or game logic
        if (_assetSystem) {
            _assetSystem->HandleDroppedFiles(paths);
        }
    }
    
private:
    void Initialize() {
        // Get initial surface size
        _surfaceProvider->GetSurfaceSize(_surfaceWidth, _surfaceHeight);
        _surfaceScale = _surfaceProvider->GetSurfaceScale();
        
        // Initialize graphics backend (uses separate factory)
        // ... create device, etc.
        
        // Create swap chain using surface from provider
        CreateSwapChain();
    }
    
    void CreateSwapChain() {
        if (!_surfaceProvider || !_device) return;
        
        void* surfaceHandle = _surfaceProvider->GetNativeSurfaceHandle();
        if (!surfaceHandle) {
            Logger::Error("No surface handle available");
            return;
        }
        
        SwapChainDescriptor desc{};
        desc.width = _surfaceWidth;
        desc.height = _surfaceHeight;
        desc.format = TextureFormat::BGRA8UnormSrgb;
        desc.usage = TextureUsage::RenderAttachment;
        desc.presentMode = PresentMode::Fifo;
        
        _swapChain = _device->CreateSwapChain(surfaceHandle, desc);
    }
    
    void RecreateSwapChain() {
        // Wait for device to be idle
        _device->WaitIdle();
        
        // Release old swap chain
        _swapChain.reset();
        
        // Create new swap chain
        CreateSwapChain();
        
        _needsResize = false;
    }
    
    void RenderFrame() {
        if (_needsResize) {
            RecreateSwapChain();
        }
        
        if (!_swapChain || _isMinimized) {
            return;
        }
        
        // Render implementation...
    }
};

// Renderer creation info
struct RendererCreateInfo {
    // Required: Surface provider for window system integration
    std::shared_ptr<ISurfaceProvider> surfaceProvider;
    
    // Required: Graphics backend factory
    std::shared_ptr<IGraphicsBackendFactory> graphicsFactory;
    
    // Optional: Initial configuration
    bool enableValidation = false;
    bool enableVSync = true;
    uint32_t frameBufferCount = 3;  // Triple buffering
};

} // namespace renderer
} // namespace pers
```

## Application Integration Example

```cpp
class Application {
private:
    // Window system
    GLFWwindow* _window = nullptr;
    std::unique_ptr<GLFWSurfaceProvider> _surfaceProvider;
    std::unique_ptr<GLFWEventForwarder> _eventForwarder;
    
    // Engine
    std::unique_ptr<pers::renderer::IRenderer> _renderer;
    
public:
    bool Initialize() {
        // 1. Initialize GLFW
        if (!glfwInit()) {
            return false;
        }
        
        // 2. Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        _window = glfwCreateWindow(1280, 720, "Application", nullptr, nullptr);
        
        // 3. Create surface provider
        _surfaceProvider = std::make_unique<GLFWSurfaceProvider>(_window);
        
        // 4. Select graphics backend
        auto graphicsFactory = pers::renderer::CreateWebGPUBackendFactory();
        
        // 5. Create renderer with separate interfaces
        pers::renderer::RendererCreateInfo createInfo{};
        createInfo.surfaceProvider = _surfaceProvider;
        createInfo.graphicsFactory = graphicsFactory;
        createInfo.enableValidation = true;
        
        _renderer = pers::renderer::CreateRenderer(createInfo);
        
        // 6. Initialize surface after renderer creates graphics instance
        auto instance = _renderer->GetGraphicsInstance();
        _surfaceProvider->InitializeSurface(
            instance,
            graphicsFactory->GetBackendType()
        );
        
        // 7. Set up event forwarding
        _eventForwarder = std::make_unique<GLFWEventForwarder>(_window);
        _eventForwarder->SetSurfaceEventHandler(
            std::dynamic_pointer_cast<ISurfaceEventHandler>(_renderer)
        );
        _eventForwarder->SetInputEventHandler(
            std::dynamic_pointer_cast<IInputEventHandler>(_renderer)
        );
        _eventForwarder->SetApplicationHandler(
            std::dynamic_pointer_cast<IApplicationHandler>(_renderer)
        );
        
        return true;
    }
    
    void Run() {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
            _renderer->Update();
            _renderer->Render();
        }
    }
    
    void Shutdown() {
        _renderer.reset();
        _eventForwarder.reset();
        _surfaceProvider.reset();
        
        if (_window) {
            glfwDestroyWindow(_window);
            _window = nullptr;
        }
        
        glfwTerminate();
    }
};
```

## Key Design Principles

1. **Complete Separation**: Surface/input handling is completely separate from graphics backend
2. **Event-Driven Architecture**: All window system events are forwarded through interfaces
3. **Platform Abstraction**: ISurfaceProvider abstracts platform-specific surface creation
4. **Flexible Event Handling**: Engine can implement only the handlers it needs
5. **No Cross-Dependencies**: Surface provider doesn't know about graphics implementation details

## Benefits

- Window system can be changed without affecting graphics code
- Graphics backend can be changed without affecting window system code
- Easy to add new platforms (SDL, native Win32, Android, iOS)
- Testable with mock implementations
- Clear separation of concerns between rendering and platform code