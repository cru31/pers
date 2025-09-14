#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <windows.h>
#elif defined(__linux__)
    #define GLFW_EXPOSE_NATIVE_X11
    #include <X11/Xlib.h>
#elif defined(__APPLE__)
    #define GLFW_EXPOSE_NATIVE_COCOA
    #import <Cocoa/Cocoa.h>
    #import <QuartzCore/CAMetalLayer.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

struct Demo {
    WGPUInstance instance = nullptr;
    WGPUSurface surface = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUSurfaceConfiguration config = {};
    WGPURenderPipeline renderPipeline = nullptr;
    bool isInitialized = false;
};

// Helper functions
WGPUStringView makeStringView(const char* str) {
    WGPUStringView view = {};
    view.data = str;
    view.length = strlen(str);
    return view;
}

void printStringView(const WGPUStringView& str) {
    if (str.data && str.length > 0) {
        std::cout.write(str.data, str.length);
    }
}

std::string loadShaderFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Callbacks
void handleRequestAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, 
                         WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestAdapterStatus_Success) {
        Demo* demo = static_cast<Demo*>(userdata1);
        demo->adapter = adapter;
        std::cout << "Adapter obtained successfully" << std::endl;
    } else {
        std::cerr << "Failed to get adapter: ";
        printStringView(message);
        std::cerr << std::endl;
    }
}

void handleRequestDevice(WGPURequestDeviceStatus status, WGPUDevice device,
                        WGPUStringView message, void* userdata1, void* userdata2) {
    if (status == WGPURequestDeviceStatus_Success) {
        Demo* demo = static_cast<Demo*>(userdata1);
        demo->device = device;
        std::cout << "Device obtained successfully" << std::endl;
    } else {
        std::cerr << "Failed to get device: ";
        printStringView(message);
        std::cerr << std::endl;
    }
}

void handleDeviceLost(WGPUDevice const* device, WGPUDeviceLostReason reason,
                     WGPUStringView message, void* userdata1, void* userdata2) {
    std::cerr << "Device lost: ";
    printStringView(message);
    std::cerr << std::endl;
}

void handleUncapturedError(WGPUDevice const* device, WGPUErrorType type,
                          WGPUStringView message, void* userdata1, void* userdata2) {
    std::cerr << "Uncaptured error: ";
    printStringView(message);
    std::cerr << std::endl;
}

// GLFW callbacks
void handleFramebufferSize(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) {
        return;
    }
    
    Demo* demo = static_cast<Demo*>(glfwGetWindowUserPointer(window));
    if (!demo || !demo->isInitialized) {
        return;
    }
    
    demo->config.width = width;
    demo->config.height = height;
    wgpuSurfaceConfigure(demo->surface, &demo->config);
}

void handleKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    std::cout << "=== WebGPU Triangle Example ===" << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "WebGPU Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    Demo demo;
    glfwSetWindowUserPointer(window, &demo);
    glfwSetFramebufferSizeCallback(window, handleFramebufferSize);
    glfwSetKeyCallback(window, handleKeyPress);
    
    // Create WebGPU instance
    WGPUInstanceDescriptor instanceDesc = {};
    demo.instance = wgpuCreateInstance(&instanceDesc);
    if (!demo.instance) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Create surface - platform specific
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    
    WGPUSurfaceSourceWindowsHWND surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    surfaceSource.hinstance = hinstance;
    surfaceSource.hwnd = hwnd;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    demo.surface = wgpuInstanceCreateSurface(demo.instance, &surfaceDesc);
#elif defined(__linux__)
    // Linux/X11 surface creation
    Display* display = glfwGetX11Display();
    Window x11Window = glfwGetX11Window(window);
    
    WGPUSurfaceSourceXlibWindow surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
    surfaceSource.display = display;
    surfaceSource.window = x11Window;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    demo.surface = wgpuInstanceCreateSurface(demo.instance, &surfaceDesc);
#elif defined(__APPLE__)
    // macOS surface creation
    // Create a CAMetalLayer and get the raw layer pointer
    id metalLayer = nullptr;
    NSWindow* nsWindow = glfwGetCocoaWindow(window);
    if (nsWindow) {
        [nsWindow.contentView setWantsLayer:YES];
        metalLayer = [CAMetalLayer layer];
        [nsWindow.contentView setLayer:metalLayer];
    }
    
    WGPUSurfaceSourceMetalLayer surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    surfaceSource.layer = metalLayer;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    demo.surface = wgpuInstanceCreateSurface(demo.instance, &surfaceDesc);
#else
    #error "Unsupported platform"
#endif
    
    if (!demo.surface) {
        std::cerr << "Failed to create surface" << std::endl;
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Request adapter
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.compatibleSurface = demo.surface;
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    
    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = handleRequestAdapter;
    adapterCallbackInfo.userdata1 = &demo;
    
    wgpuInstanceRequestAdapter(demo.instance, &adapterOptions, adapterCallbackInfo);
    wgpuInstanceProcessEvents(demo.instance);
    
    if (!demo.adapter) {
        std::cerr << "Failed to get adapter" << std::endl;
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Get adapter info
    WGPUAdapterInfo adapterInfo = {};
    wgpuAdapterGetInfo(demo.adapter, &adapterInfo);
    std::cout << "Adapter: ";
    printStringView(adapterInfo.description);
    std::cout << std::endl;
    
    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = makeStringView("Triangle Device");
    
    WGPUDeviceLostCallbackInfo lostInfo = {};
    lostInfo.callback = handleDeviceLost;
    deviceDesc.deviceLostCallbackInfo = lostInfo;
    
    WGPUUncapturedErrorCallbackInfo errorInfo = {};
    errorInfo.callback = handleUncapturedError;
    deviceDesc.uncapturedErrorCallbackInfo = errorInfo;
    
    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = handleRequestDevice;
    deviceCallbackInfo.userdata1 = &demo;
    
    wgpuAdapterRequestDevice(demo.adapter, &deviceDesc, deviceCallbackInfo);
    wgpuInstanceProcessEvents(demo.instance);
    
    if (!demo.device) {
        std::cerr << "Failed to get device" << std::endl;
        wgpuAdapterRelease(demo.adapter);
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Get queue
    WGPUQueue queue = wgpuDeviceGetQueue(demo.device);
    
    // Load shader
    std::string shaderCode = loadShaderFile("shader.wgsl");
    if (shaderCode.empty()) {
        std::cerr << "Failed to load shader" << std::endl;
        wgpuQueueRelease(queue);
        wgpuDeviceRelease(demo.device);
        wgpuAdapterRelease(demo.adapter);
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Create shader module
    WGPUShaderSourceWGSL wgslSource = {};
    wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSource.code = makeStringView(shaderCode.c_str());
    
    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.label = makeStringView("Triangle Shader");
    shaderDesc.nextInChain = &wgslSource.chain;
    
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(demo.device, &shaderDesc);
    if (!shaderModule) {
        std::cerr << "Failed to create shader module" << std::endl;
        wgpuQueueRelease(queue);
        wgpuDeviceRelease(demo.device);
        wgpuAdapterRelease(demo.adapter);
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Create pipeline layout
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.label = makeStringView("Pipeline Layout");
    pipelineLayoutDesc.bindGroupLayoutCount = 0;
    
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(demo.device, &pipelineLayoutDesc);
    
    // Get surface capabilities
    WGPUSurfaceCapabilities surfaceCapabilities = {};
    wgpuSurfaceGetCapabilities(demo.surface, demo.adapter, &surfaceCapabilities);
    
    // Create render pipeline
    WGPUVertexState vertexState = {};
    vertexState.module = shaderModule;
    vertexState.entryPoint = makeStringView("vs_main");
    
    WGPUColorTargetState colorTarget = {};
    colorTarget.format = surfaceCapabilities.formats[0];
    colorTarget.writeMask = WGPUColorWriteMask_All;
    
    WGPUFragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = makeStringView("fs_main");
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    
    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    
    WGPUMultisampleState multisampleState = {};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;
    
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = makeStringView("Render Pipeline");
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.vertex = vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = primitiveState;
    pipelineDesc.multisample = multisampleState;
    
    demo.renderPipeline = wgpuDeviceCreateRenderPipeline(demo.device, &pipelineDesc);
    if (!demo.renderPipeline) {
        std::cerr << "Failed to create render pipeline" << std::endl;
        wgpuPipelineLayoutRelease(pipelineLayout);
        wgpuShaderModuleRelease(shaderModule);
        wgpuQueueRelease(queue);
        wgpuDeviceRelease(demo.device);
        wgpuAdapterRelease(demo.adapter);
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Configure surface
    demo.config.device = demo.device;
    demo.config.usage = WGPUTextureUsage_RenderAttachment;
    demo.config.format = surfaceCapabilities.formats[0];
    demo.config.presentMode = WGPUPresentMode_Fifo;
    demo.config.alphaMode = surfaceCapabilities.alphaModes[0];
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    demo.config.width = width;
    demo.config.height = height;
    
    wgpuSurfaceConfigure(demo.surface, &demo.config);
    demo.isInitialized = true;
    
    // Main loop
    int frameCount = 0;
    const int maxFramesInCI = 300; // Run for 300 frames (about 5 seconds at 60fps) in CI
    const bool isCI = std::getenv("CI") != nullptr;
    const bool isTestMode = std::getenv("PERS_TEST_MODE") != nullptr;
    const bool shouldAutoExit = isCI || isTestMode;
    
    while (!glfwWindowShouldClose(window)) {
        // Auto-exit in CI or test mode after rendering enough frames
        if (shouldAutoExit && frameCount++ >= maxFramesInCI) {
            std::cout << "Auto-exit mode: Rendered " << frameCount << " frames successfully, exiting..." << std::endl;
            break;
        }
        glfwPollEvents();
        
        // Get current texture
        WGPUSurfaceTexture surfaceTexture = {};
        wgpuSurfaceGetCurrentTexture(demo.surface, &surfaceTexture);
        
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            if (surfaceTexture.texture) {
                wgpuTextureRelease(surfaceTexture.texture);
            }
            continue;
        }
        
        // Create texture view
        WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
        
        // Create command encoder
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = makeStringView("Command Encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(demo.device, &encoderDesc);
        
        // Begin render pass
        WGPUColor clearColor = {0.0f, 0.2f, 0.4f, 1.0f};
        
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = textureView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = clearColor;
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        
        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.label = makeStringView("Render Pass");
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        
        // Draw triangle
        wgpuRenderPassEncoderSetPipeline(renderPass, demo.renderPipeline);
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);
        
        // Finish command buffer
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = makeStringView("Command Buffer");
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuCommandEncoderRelease(encoder);
        
        // Submit command buffer
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        
        // Present
        wgpuSurfacePresent(demo.surface);
        
        // Cleanup frame resources
        wgpuTextureViewRelease(textureView);
        wgpuTextureRelease(surfaceTexture.texture);
    }
    
    // Cleanup
    wgpuRenderPipelineRelease(demo.renderPipeline);
    wgpuPipelineLayoutRelease(pipelineLayout);
    wgpuShaderModuleRelease(shaderModule);
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(demo.device);
    wgpuAdapterRelease(demo.adapter);
    wgpuSurfaceRelease(demo.surface);
    wgpuInstanceRelease(demo.instance);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "=== Triangle example completed ===" << std::endl;
    return 0;
}