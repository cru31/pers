#pragma once

#include "pers/core/IWindowFactory.h"

class GLFWWindowFactory : public IWindowFactory {
public:
    GLFWWindowFactory();
    ~GLFWWindowFactory() override;
    
    std::unique_ptr<IWindow> createWindow(int width, int height, const std::string& title) const override;
    const char* getFactoryName() const override { return "GLFW"; }
    
private:
    // Static GLFW initialization management
    static bool initializeGLFW();
    static void terminateGLFW();
    
    static int _glfwRefCount;
};