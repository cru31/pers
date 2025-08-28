#ifdef __APPLE__

#include "WebGPUInstance.h"
#include <iostream>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <webgpu.h>

namespace pers {

void* WebGPUInstance::createSurfaceMacOS(void* windowHandle) {
    if (!_instance) {
        std::cerr << "[WebGPUInstance] Instance not initialized" << std::endl;
        return nullptr;
    }
    
    if (!windowHandle) {
        std::cerr << "[WebGPUInstance] Invalid window handle" << std::endl;
        return nullptr;
    }
    
    GLFWwindow* window = static_cast<GLFWwindow*>(windowHandle);
    
    @autoreleasepool {
        std::cout << "[WebGPUInstance] Creating macOS Metal surface..." << std::endl;
        
        // Get the NSWindow from GLFW
        NSWindow* nsWindow = glfwGetCocoaWindow(window);
        if (!nsWindow) {
            std::cerr << "[WebGPUInstance] Failed to get NSWindow from GLFW" << std::endl;
            return nullptr;
        }
        
        // Get the content view
        NSView* contentView = [nsWindow contentView];
        if (!contentView) {
            std::cerr << "[WebGPUInstance] Failed to get content view" << std::endl;
            return nullptr;
        }
        
        // Create a CAMetalLayer
        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        if (!metalLayer) {
            std::cerr << "[WebGPUInstance] Failed to create CAMetalLayer" << std::endl;
            return nullptr;
        }
        
        // Configure the layer
        [contentView setWantsLayer:YES];
        [contentView setLayer:metalLayer];
        
        // The layer should fill the view
        metalLayer.frame = contentView.bounds;
        metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Create the WebGPU surface from the Metal layer
        WGPUSurfaceSourceMetalLayer surfaceSource = {};
        surfaceSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        surfaceSource.chain.next = nullptr;
        surfaceSource.layer = (__bridge void*)metalLayer;
        
        WGPUSurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
        surfaceDesc.label = "macOS Metal Surface";
        
        WGPUSurface surface = wgpuInstanceCreateSurface(_instance, &surfaceDesc);
        if (!surface) {
            std::cerr << "[WebGPUInstance] Failed to create WebGPU surface from Metal layer" << std::endl;
            return nullptr;
        }
        
        std::cout << "[WebGPUInstance] Metal surface created successfully" << std::endl;
        return surface;
    }
}

} // namespace pers

#endif // __APPLE__