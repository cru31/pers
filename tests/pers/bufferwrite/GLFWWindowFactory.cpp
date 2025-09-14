#include "GLFWWindowFactory.h"
#include "GLFWWindow.h"
#include <GLFW/glfw3.h>
#include <iostream>

// Static member initialization
int GLFWWindowFactory::_glfwRefCount = 0;

GLFWWindowFactory::GLFWWindowFactory() {
    initializeGLFW();
}

GLFWWindowFactory::~GLFWWindowFactory() {
    terminateGLFW();
}

std::unique_ptr<IWindow> GLFWWindowFactory::createWindow(int width, int height, const std::string& title) const {
    auto window = std::make_unique<GLFWWindow>();
    if (!window->create(width, height, title.c_str())) {
        std::cerr << "[GLFWWindowFactory] Failed to create window" << std::endl;
        return nullptr;
    }
    return window;
}

bool GLFWWindowFactory::initializeGLFW() {
    if (_glfwRefCount == 0) {
        if (!glfwInit()) {
            std::cerr << "[GLFWWindowFactory] Failed to initialize GLFW" << std::endl;
            return false;
        }
        std::cout << "[GLFWWindowFactory] GLFW initialized" << std::endl;
    }
    _glfwRefCount++;
    return true;
}

void GLFWWindowFactory::terminateGLFW() {
    _glfwRefCount--;
    if (_glfwRefCount == 0) {
        glfwTerminate();
        std::cout << "[GLFWWindowFactory] GLFW terminated" << std::endl;
    }
}