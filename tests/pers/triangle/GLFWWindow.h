#pragma once

#include "pers/core/IWindow.h"
#include <GLFW/glfw3.h>

/**
 * @brief GLFW implementation of IWindow interface
 */
class GLFWWindow : public IWindow {
public:
    GLFWWindow();
    ~GLFWWindow() override;
    
    // IWindow interface implementation
    bool create(int width, int height, const char* title) override;
    void destroy() override;
    bool isValid() const override;
    
    glm::ivec2 getFramebufferSize() const override;
    bool shouldClose() const override;
    void setShouldClose(bool shouldClose) override;
    
    void pollEvents() override;
    void setResizeCallback(ResizeCallback callback) override;
    void setKeyCallback(KeyCallback callback) override;
    
    pers::NativeWindowHandle getNativeHandle() const override;
    
    // GLFW-specific methods
    GLFWwindow* getGLFWWindow() const { return _window; }
    
private:
    // Static callbacks that forward to instance methods
    static void onFramebufferSize(GLFWwindow* window, int width, int height);
    static void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
    
private:
    GLFWwindow* _window = nullptr;
    ResizeCallback _resizeCallback;
    KeyCallback _keyCallback;
};