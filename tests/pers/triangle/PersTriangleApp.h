#pragma once

#include <GLFW/glfw3.h>
#include <memory>

class TriangleRenderer;

class PersTriangleApp {
public:
    PersTriangleApp();
    ~PersTriangleApp();
    
    bool initialize();
    void run();
    
private:
    // Initialization methods
    bool initializeWindow();
    bool initializeRenderer();
    bool createTriangle();
    
    // Runtime methods
    void render();
    void cleanup();
    
    // Static callbacks that forward to instance methods
    static void onFramebufferSize(GLFWwindow* window, int width, int height);
    static void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    // Instance methods for handling events
    void handleResize(int width, int height);
    void handleKeyPress(int key, int scancode, int action, int mods);
    
private:
    GLFWwindow* _window = nullptr;
    int _width = 800;
    int _height = 600;
    
    // Renderer
    std::unique_ptr<TriangleRenderer> _renderer;
};