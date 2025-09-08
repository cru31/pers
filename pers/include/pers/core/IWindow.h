#pragma once

#include <glm/vec2.hpp>
#include <functional>
#include <memory>
#include "pers/core/platform/NativeWindowHandle.h"

// Window event callbacks
using ResizeCallback = std::function<void(int width, int height)>;
using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
using RefreshCallback = std::function<void()>;

/**
 * @brief Window interface for abstracting windowing system
 * 
 * This interface abstracts platform-specific windowing operations,
 * allowing the application to work with different windowing systems
 * (GLFW, SDL, native OS windows, etc.)
 */
class IWindow {
public:
    virtual ~IWindow() = default;
    
    // Window lifecycle
    virtual bool create(int width, int height, const char* title) = 0;
    virtual void destroy() = 0;
    virtual bool isValid() const = 0;
    
    // Window properties
    virtual glm::ivec2 getFramebufferSize() const = 0;
    virtual bool shouldClose() const = 0;
    virtual void setShouldClose(bool shouldClose) = 0;
    
    // Event handling
    virtual void pollEvents() = 0;
    virtual void setResizeCallback(ResizeCallback callback) = 0;
    virtual void setKeyCallback(KeyCallback callback) = 0;
    virtual void setRefreshCallback(RefreshCallback callback) = 0;
    
    // Platform-specific handle extraction
    // Returns platform-specific window handle for surface creation
    virtual pers::NativeWindowHandle getNativeHandle() const = 0;
};