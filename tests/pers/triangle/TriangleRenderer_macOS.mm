#ifdef __APPLE__

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#include <iostream>

void* createMetalLayer(GLFWwindow* window) {
    if (!window) {
        std::cerr << "[createMetalLayer] Invalid window handle" << std::endl;
        return nullptr;
    }
    
    @autoreleasepool {
        // Get the NSWindow from GLFW
        NSWindow* nsWindow = glfwGetCocoaWindow(window);
        if (!nsWindow) {
            std::cerr << "[createMetalLayer] Failed to get NSWindow from GLFW" << std::endl;
            return nullptr;
        }
        
        // Get the content view
        NSView* contentView = [nsWindow contentView];
        if (!contentView) {
            std::cerr << "[createMetalLayer] Failed to get content view" << std::endl;
            return nullptr;
        }
        
        // Create a CAMetalLayer
        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        if (!metalLayer) {
            std::cerr << "[createMetalLayer] Failed to create CAMetalLayer" << std::endl;
            return nullptr;
        }
        
        // Configure the layer
        [contentView setWantsLayer:YES];
        [contentView setLayer:metalLayer];
        
        // The layer should fill the view
        metalLayer.frame = contentView.bounds;
        metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
        
        // Set pixel format (BGRA8Unorm is standard)
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Get the default Metal device
        metalLayer.device = MTLCreateSystemDefaultDevice();
        if (!metalLayer.device) {
            std::cerr << "[createMetalLayer] Failed to get Metal device" << std::endl;
            return nullptr;
        }
        
        std::cout << "[createMetalLayer] Metal layer created successfully" << std::endl;
        
        // Return the layer as a void pointer (the bridge cast retains the object)
        return (__bridge_retained void*)metalLayer;
    }
}

#endif // __APPLE__