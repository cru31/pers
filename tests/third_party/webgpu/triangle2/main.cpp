#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <webgpu.h>
#include <wgpu.h>

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
    
    // [PERS V2 CALL STACK]
    // Window::OnResize(width, height)
    // └─> Application::OnWindowResize(width, height)
    //     └─> ISwapChain::Resize(width, height)
    //         └─> WebGPUSwapChain::Resize(width, height)
    //             └─> _config.width = width; _config.height = height;
    //             └─> wgpuSurfaceConfigure(_surface, &_config)
    // Note: SwapChain이 window resize를 직접 처리
    // State: SwapChain config 업데이트 및 surface 재구성
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
    // [PERS V2 CALL STACK]
    // Application::Initialize()
    // └─> CreateInstance(InstanceDesc{.backend = GraphicsBackend::WebGPU})
    //     └─> WebGPUBackend::CreateWebGPUInstance()
    //         └─> WebGPUInstance::WebGPUInstance(InstanceDesc desc)
    //             └─> WGPUInstanceDescriptor instanceDesc = {};
    //             └─> _instance = wgpuCreateInstance(&instanceDesc)
    // Returns: std::shared_ptr<IInstance>
    // Components: IInstance는 모든 graphics 시스템의 시작점
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

    // [PERS V2 CALL STACK]
    // Application::CreateWindow(WindowDesc{width: 800, height: 600})
    // └─> Window::GetNativeHandle() → void* hwnd
    // └─> IInstance::createSurface(hwnd)
    //     └─> WebGPUInstance::createSurface(void* windowHandle)
    //         └─> WGPUSurfaceDescriptor surfaceDesc = BuildSurfaceDesc(hwnd)
    //         └─> _surfaces.push_back(wgpuInstanceCreateSurface(_instance, &surfaceDesc))
    // Returns: std::shared_ptr<ISurface>
    // Note: Surface는 OS window와 GPU를 연결하는 인터페이스
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
    id metalLayer = glfwGetCocoaMetalLayer(window);
    
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
        
        // [PERS V2 CALL STACK]
        // Error cleanup - surface 생성 실패
        // ~WebGPUInstance() [shared_ptr destructor]
        // └─> if (_instance) wgpuInstanceRelease(_instance)
        // Note: V2는 RAII와 shared_ptr로 자동 cleanup
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

    // [PERS V2 CALL STACK]
    // Application::Initialize() [continued]
    // └─> IInstance::requestAdapter(AdapterOptions{.powerPreference = PowerPreference::HighPerformance})
    //     └─> WebGPUInstance::requestAdapter(const AdapterOptions& options)
    //         └─> WGPURequestAdapterOptions wgpuOptions = ConvertToWGPU(options)
    //         └─> wgpuOptions.compatibleSurface = surface ? surface->GetHandle() : nullptr
    //         └─> wgpuInstanceRequestAdapter(_instance, &wgpuOptions, callback)
    //         └─> while (!adapterReceived) {
    //                 wgpuInstanceProcessEvents(_instance);
    //                 std::this_thread::sleep_for(10ms);
    //             }
    // Returns: std::shared_ptr<IAdapter>
    // Note: V2는 비동기 API를 동기적으로 래핑하여 사용 편의성 제공
    wgpuInstanceRequestAdapter(demo.instance, &adapterOptions, adapterCallbackInfo);
    wgpuInstanceProcessEvents(demo.instance);

    if (!demo.adapter) {
        std::cerr << "Failed to get adapter" << std::endl;
        
        // [PERS V2 CALL STACK]
        // Error cleanup - adapter 요청 실패
        // ~WebGPUSurface() → wgpuSurfaceRelease(_surface)
        // ~WebGPUInstance() → wgpuInstanceRelease(_instance)
        // Note: 역순으로 cleanup (LIFO)
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Get adapter info
    // [PERS V2 CALL STACK]
    // IAdapter::getProperties()
    // └─> WebGPUAdapter::getProperties()
    //     └─> WGPUAdapterInfo info = {};
    //     └─> wgpuAdapterGetInfo(_adapter, &info)
    //     └─> return AdapterProperties{
    //            .vendorID = info.vendorID,
    //            .deviceID = info.deviceID,
    //            .name = std::string(info.device),
    //            .driverDescription = std::string(info.description),
    //            .adapterType = ConvertAdapterType(info.adapterType),
    //            .backendType = ConvertBackendType(info.backendType)
    //         }
    // Returns: AdapterProperties struct
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

    // [PERS V2 CALL STACK]
    // IAdapter::createDevice(DeviceDescriptor{.requiredFeatures = {}, .requiredLimits = {}})
    // └─> WebGPUAdapter::createDevice(const DeviceDescriptor& desc)
    //     └─> WGPUDeviceDescriptor wgpuDesc = ConvertToWGPU(desc)
    //     └─> SetupCallbacks(wgpuDesc) [device lost, uncaptured error]
    //     └─> wgpuAdapterRequestDevice(_adapter, &wgpuDesc, callback)
    //     └─> while (!deviceReceived) {
    //             wgpuInstanceProcessEvents(_instance);
    //         }
    //     └─> auto device = std::make_shared<WebGPUDevice>(wgpuDevice)
    //     └─> device->Initialize() [
    //             _queue = std::make_shared<WebGPUQueue>(wgpuDeviceGetQueue(_device)),
    //             _resourceFactory = std::make_shared<WebGPUResourceFactory>(_device)
    //         ]
    // Returns: std::shared_ptr<IDevice>
    // Creates: IQueue, IResourceFactory 자동 생성
    wgpuAdapterRequestDevice(demo.adapter, &deviceDesc, deviceCallbackInfo);
    wgpuInstanceProcessEvents(demo.instance);

    if (!demo.device) {
        std::cerr << "Failed to get device" << std::endl;
        
        // [PERS V2 CALL STACK]
        // Error cleanup - device 생성 실패
        // ~WebGPUAdapter() → wgpuAdapterRelease(_adapter)
        // ~WebGPUSurface() → wgpuSurfaceRelease(_surface)
        // ~WebGPUInstance() → wgpuInstanceRelease(_instance)
        wgpuAdapterRelease(demo.adapter);
        wgpuSurfaceRelease(demo.surface);
        wgpuInstanceRelease(demo.instance);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Get queue
    // [PERS V2 CALL STACK]
    // IDevice::getQueue()
    // └─> WebGPUDevice::getQueue()
    //     └─> return _queue; // 이미 device 생성시 초기화됨
    // Note: WebGPUDevice 생성자에서:
    //       _queue = std::make_shared<WebGPUQueue>(wgpuDeviceGetQueue(_device))
    // Returns: std::shared_ptr<IQueue>
    // Usage: Command buffer submission
    WGPUQueue queue = wgpuDeviceGetQueue(demo.device);

    // Load shader
    std::string shaderCode = loadShaderFile("shader.wgsl");
    if (shaderCode.empty()) {
        std::cerr << "Failed to load shader" << std::endl;
        
        // [PERS V2 CALL STACK] - Error cleanup
        // ~WebGPUQueue() → wgpuQueueRelease(_queue)
        // ~WebGPUDevice() → wgpuDeviceRelease(_device)
        // ~WebGPUAdapter() → wgpuAdapterRelease(_adapter)
        // ~WebGPUSurface() → wgpuSurfaceRelease(_surface)
        // ~WebGPUInstance() → wgpuInstanceRelease(_instance)
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

    // [PERS V2 CALL STACK]
    // ShaderLibrary::LoadShader("triangle.wgsl")
    // └─> IResourceFactory::createShaderModule(ShaderModuleDesc{.source = code, .stage = ShaderStage::Vertex|Fragment})
    //     └─> WebGPUResourceFactory::createShaderModule(const ShaderModuleDesc& desc)
    //         └─> WGPUShaderModuleDescriptor shaderDesc = {};
    //         └─> shaderDesc.label = desc.label.c_str();
    //         └─> WGPUShaderSourceWGSL wgslDesc = {};
    //         └─> wgslDesc.code = desc.source.c_str();
    //         └─> shaderDesc.nextInChain = &wgslDesc.chain;
    //         └─> WGPUShaderModule module = wgpuDeviceCreateShaderModule(_device, &shaderDesc)
    //         └─> return std::make_shared<WebGPUShaderModule>(module)
    // Returns: std::shared_ptr<IShaderModule>
    // Note: ShaderLibrary caches modules by hash(source) to avoid recompilation
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(demo.device, &shaderDesc);
    if (!shaderModule) {
        std::cerr << "Failed to create shader module" << std::endl;
        
        // [PERS V2 CALL STACK] - Shader creation failure cleanup
        // Resource cleanup in reverse order
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

    // [PERS V2 CALL STACK]
    // IResourceFactory::createPipelineLayout(PipelineLayoutDesc{.bindGroupLayouts = {}})
    // └─> WebGPUResourceFactory::createPipelineLayout(const PipelineLayoutDesc& desc)
    //     └─> WGPUPipelineLayoutDescriptor layoutDesc = {};
    //     └─> layoutDesc.label = desc.label.c_str();
    //     └─> std::vector<WGPUBindGroupLayout> wgpuLayouts;
    //     └─> for (auto& layout : desc.bindGroupLayouts) {
    //             wgpuLayouts.push_back(layout->GetHandle());
    //         }
    //     └─> layoutDesc.bindGroupLayoutCount = wgpuLayouts.size();
    //     └─> layoutDesc.bindGroupLayouts = wgpuLayouts.data();
    //     └─> WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &layoutDesc)
    //     └─> return std::make_shared<WebGPUPipelineLayout>(layout)
    // Returns: std::shared_ptr<IPipelineLayout>
    // Note: V2 uses 4-tier binding model (Frame/Pass/Material/Object)
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(demo.device, &pipelineLayoutDesc);

    // Get surface capabilities
    // [PERS V2 CALL STACK]
    // ISurface::GetCapabilities(adapter)
    // └─> WebGPUSurface::GetCapabilities(const std::shared_ptr<IAdapter>& adapter)
    //     └─> WGPUSurfaceCapabilities caps = {};
    //     └─> wgpuSurfaceGetCapabilities(_surface, adapter->GetHandle(), &caps)
    //     └─> return SurfaceCapabilities{
    //            .formats = ConvertFormats(caps.formats, caps.formatCount),
    //            .presentModes = ConvertPresentModes(caps.presentModes, caps.presentModeCount),
    //            .alphaModes = ConvertAlphaModes(caps.alphaModes, caps.alphaModeCount)
    //         }
    // Returns: SurfaceCapabilities
    // Used for: SwapChain configuration
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

    // [PERS V2 CALL STACK]
    // PipelineCache::GetOrCreateGraphicsPipeline(GraphicsPipelineDesc{...})
    // └─> uint64_t hash = HashPipelineDesc(desc)
    // └─> if (cache.contains(hash)) return cache[hash];
    // └─> IResourceFactory::createGraphicsPipeline(desc)
    //     └─> WebGPUResourceFactory::createGraphicsPipeline(const GraphicsPipelineDesc& desc)
    //         └─> WGPURenderPipelineDescriptor pipelineDesc = {};
    //         └─> SetupVertexState(pipelineDesc.vertex, desc.vertexShader, desc.vertexLayout)
    //         └─> SetupFragmentState(pipelineDesc.fragment, desc.fragmentShader, desc.colorTargets)
    //         └─> SetupPrimitiveState(pipelineDesc.primitive, desc.primitiveTopology)
    //         └─> SetupDepthStencilState(pipelineDesc.depthStencil, desc.depthStencil)
    //         └─> SetupMultisampleState(pipelineDesc.multisample, desc.multisample)
    //         └─> pipelineDesc.layout = desc.layout ? desc.layout->GetHandle() : nullptr
    //         └─> WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(_device, &pipelineDesc)
    //         └─> auto result = std::make_shared<WebGPUGraphicsPipeline>(pipeline, desc)
    //         └─> cache[hash] = result;
    //         └─> return result;
    // Returns: std::shared_ptr<IGraphicsPipeline>
    // Cached by: Pipeline state hash for reuse
    demo.renderPipeline = wgpuDeviceCreateRenderPipeline(demo.device, &pipelineDesc);
    if (!demo.renderPipeline) {
        std::cerr << "Failed to create render pipeline" << std::endl;
        
        // [PERS V2 CALL STACK] - Pipeline creation failure cleanup
        // ~WebGPUPipelineLayout() → wgpuPipelineLayoutRelease(_layout)
        // ~WebGPUShaderModule() → wgpuShaderModuleRelease(_module)
        // ~WebGPUQueue() → wgpuQueueRelease(_queue)
        // ~WebGPUDevice() → wgpuDeviceRelease(_device)
        // ~WebGPUAdapter() → wgpuAdapterRelease(_adapter)
        // ~WebGPUSurface() → wgpuSurfaceRelease(_surface)
        // ~WebGPUInstance() → wgpuInstanceRelease(_instance)
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

    // [PERS V2 CALL STACK]
    // IDevice::createSwapChain(surface, SwapChainDesc{.width = 800, .height = 600, .format = BGRA8Unorm})
    // └─> WebGPUDevice::createSwapChain(const std::shared_ptr<ISurface>& surface, const SwapChainDesc& desc)
    //     └─> WGPUSurfaceConfiguration config = {};
    //     └─> config.device = _device;
    //     └─> config.format = ConvertToWGPU(desc.format);
    //     └─> config.usage = WGPUTextureUsage_RenderAttachment;
    //     └─> config.presentMode = ConvertToWGPU(desc.presentMode);
    //     └─> config.alphaMode = ConvertToWGPU(desc.alphaMode);
    //     └─> config.width = desc.width;
    //     └─> config.height = desc.height;
    //     └─> wgpuSurfaceConfigure(surface->GetHandle(), &config)
    //     └─> return std::make_shared<WebGPUSwapChain>(surface, config)
    // Returns: std::shared_ptr<ISwapChain>
    // State: SwapChain ready for rendering
    wgpuSurfaceConfigure(demo.surface, &demo.config);
    demo.isInitialized = true;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get current texture
        // [PERS V2 CALL STACK]
        // Application::RenderFrame()
        // └─> ISwapChain::getCurrentTextureView()
        //     └─> WebGPUSwapChain::getCurrentTextureView()
        //         └─> WGPUSurfaceTexture surfaceTexture = {};
        //         └─> wgpuSurfaceGetCurrentTexture(_surface, &surfaceTexture)
        //         └─> if (surfaceTexture.status != Success) HandleError();
        //         └─> WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, nullptr)
        //         └─> _currentTexture = surfaceTexture.texture;
        //         └─> return std::make_shared<WebGPUTextureView>(view)
        // Returns: std::shared_ptr<ITextureView>
        // Note: SwapChain manages texture lifetime until present
        WGPUSurfaceTexture surfaceTexture = {};
        wgpuSurfaceGetCurrentTexture(demo.surface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            if (surfaceTexture.texture) {
                // [PERS V2 CALL STACK]
                // SwapChain texture acquisition failed
                // ~WebGPUTexture() → wgpuTextureRelease(_texture)
                wgpuTextureRelease(surfaceTexture.texture);
            }
            continue;
        }

        // Create texture view
        // [PERS V2 CALL STACK]
        // ISwapChain::getCurrentTextureView() [continued]
        // └─> wgpuTextureCreateView(surfaceTexture.texture, nullptr)
        // Note: V2에서는 SwapChain이 내부에서 처리하여 ITextureView로 반환
        WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

        // Create command encoder
        // [PERS V2 CALL STACK]
        // Application::RenderFrame() [continued]
        // └─> IDevice::createCommandEncoder(CommandEncoderDesc{.label = "Frame Encoder"})
        //     └─> WebGPUDevice::createCommandEncoder(const CommandEncoderDesc& desc)
        //         └─> WGPUCommandEncoderDescriptor encoderDesc = {};
        //         └─> encoderDesc.label = desc.label.c_str();
        //         └─> WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc)
        //         └─> return std::make_shared<WebGPUCommandEncoder>(encoder)
        // Returns: std::shared_ptr<ICommandEncoder>
        // State: Ready for command recording
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

        // [PERS V2 CALL STACK]
        // ICommandEncoder::beginRenderPass(RenderPassDesc{.colorAttachments = {{.view = backbuffer, .clearValue = {0.0f, 0.2f, 0.4f, 1.0f}}}})
        // └─> WebGPUCommandEncoder::beginRenderPass(const RenderPassDesc& desc)
        //     └─> WGPURenderPassDescriptor renderPassDesc = {};
        //     └─> std::vector<WGPURenderPassColorAttachment> colorAttachments;
        //     └─> for (auto& attachment : desc.colorAttachments) {
        //             WGPURenderPassColorAttachment wgpuAttachment = {};
        //             wgpuAttachment.view = attachment.view->GetHandle();
        //             wgpuAttachment.loadOp = ConvertLoadOp(attachment.loadOp);
        //             wgpuAttachment.storeOp = ConvertStoreOp(attachment.storeOp);
        //             wgpuAttachment.clearValue = ConvertColor(attachment.clearValue);
        //             colorAttachments.push_back(wgpuAttachment);
        //         }
        //     └─> renderPassDesc.colorAttachments = colorAttachments.data();
        //     └─> renderPassDesc.colorAttachmentCount = colorAttachments.size();
        //     └─> WGPURenderPassEncoder encoder = wgpuCommandEncoderBeginRenderPass(_encoder, &renderPassDesc)
        //     └─> return std::make_shared<WebGPURenderPassEncoder>(encoder)
        // Returns: std::shared_ptr<IRenderPassEncoder>
        // State: Recording render commands
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        // Draw triangle
        // [PERS V2 CALL STACK]
        // IRenderPassEncoder::cmdSetGraphicsPipeline(pipeline)
        // └─> WebGPURenderPassEncoder::cmdSetGraphicsPipeline(const std::shared_ptr<IGraphicsPipeline>& pipeline)
        //     └─> wgpuRenderPassEncoderSetPipeline(_encoder, pipeline->GetHandle())
        //     └─> _currentPipeline = pipeline;
        // State: Pipeline bound for drawing
        wgpuRenderPassEncoderSetPipeline(renderPass, demo.renderPipeline);
        // [PERS V2 CALL STACK]
        // IRenderPassEncoder::cmdDraw(3, 1, 0, 0)
        // └─> WebGPURenderPassEncoder::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        //     └─> ValidateDrawState() [check pipeline, vertex buffers bound]
        //     └─> wgpuRenderPassEncoderDraw(_encoder, vertexCount, instanceCount, firstVertex, firstInstance)
        // Parameters:
        //   - vertexCount: 3 (triangle)
        //   - instanceCount: 1 (single instance)
        //   - firstVertex: 0
        //   - firstInstance: 0
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
        // [PERS V2 CALL STACK]
        // IRenderPassEncoder::end()
        // └─> WebGPURenderPassEncoder::end()
        //     └─> wgpuRenderPassEncoderEnd(_encoder)
        //     └─> _ended = true;
        // State: Render pass commands finalized
        wgpuRenderPassEncoderEnd(renderPass);
        
        // [PERS V2 CALL STACK]
        // ~WebGPURenderPassEncoder()
        // └─> if (_encoder) wgpuRenderPassEncoderRelease(_encoder)
        wgpuRenderPassEncoderRelease(renderPass);

        // Finish command buffer
        // [PERS V2 CALL STACK]
        // ICommandEncoder::finish()
        // └─> WebGPUCommandEncoder::finish()
        //     └─> ValidateState() [ensure no open render/compute passes]
        //     └─> WGPUCommandBufferDescriptor cmdBufferDesc = {};
        //     └─> cmdBufferDesc.label = "Command Buffer";
        //     └─> WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(_encoder, &cmdBufferDesc)
        //     └─> return std::make_shared<WebGPUCommandBuffer>(cmdBuffer)
        // Returns: std::shared_ptr<ICommandBuffer>
        // State: Commands ready for submission
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = makeStringView("Command Buffer");
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        
        // [PERS V2 CALL STACK]
        // ~WebGPUCommandEncoder()
        // └─> if (_encoder) wgpuCommandEncoderRelease(_encoder)
        wgpuCommandEncoderRelease(encoder);

        // Submit command buffer
        // [PERS V2 CALL STACK]
        // IQueue::submit(commandBuffer)
        // └─> WebGPUQueue::submit(const std::shared_ptr<ICommandBuffer>& commandBuffer)
        //     └─> WGPUCommandBuffer buffers[] = { commandBuffer->GetHandle() };
        //     └─> wgpuQueueSubmit(_queue, 1, buffers)
        //     └─> _submittedCommandBuffers.push_back(commandBuffer) [keep alive until GPU done]
        // State: Commands submitted to GPU for execution
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        
        // [PERS V2 CALL STACK]
        // ~WebGPUCommandBuffer()
        // └─> if (_commandBuffer) wgpuCommandBufferRelease(_commandBuffer)
        wgpuCommandBufferRelease(commandBuffer);

        // Present
        // [PERS V2 CALL STACK]
        // ISwapChain::present()
        // └─> WebGPUSwapChain::present()
        //     └─> wgpuSurfacePresent(_surface)
        //     └─> _frameIndex = (_frameIndex + 1) % 3; [triple buffering]
        //     └─> ReleaseCurrentTexture() [cleanup current frame resources]
        // State: Frame presented to screen, next frame ready
        wgpuSurfacePresent(demo.surface);

        // Cleanup frame resources
        // [PERS V2 CALL STACK]
        // Frame resource cleanup (V2에서는 SwapChain이 자동 관리)
        // ~WebGPUTextureView() → wgpuTextureViewRelease(_view)
        // SwapChain::ReleaseCurrentTexture() → wgpuTextureRelease(_currentTexture)
        wgpuTextureViewRelease(textureView);
        wgpuTextureRelease(surfaceTexture.texture);
    }

    // Cleanup
    // [PERS V2 CALL STACK]
    // Application::Shutdown()
    // └─> GraphicsSystem::Shutdown()
    //     └─> PipelineCache::Clear()
    //         └─> ~WebGPUGraphicsPipeline() → wgpuRenderPipelineRelease(_pipeline)
    //     └─> ResourceFactory::ReleaseAll()
    //         └─> ~WebGPUPipelineLayout() → wgpuPipelineLayoutRelease(_layout)
    //         └─> ~WebGPUShaderModule() → wgpuShaderModuleRelease(_module)
    //     └─> Device::Shutdown()
    //         └─> ~WebGPUQueue() → wgpuQueueRelease(_queue)
    //         └─> ~WebGPUDevice() → wgpuDeviceRelease(_device)
    //     └─> ~WebGPUAdapter() → wgpuAdapterRelease(_adapter)
    //     └─> ~WebGPUSurface() → wgpuSurfaceRelease(_surface)
    //     └─> ~WebGPUInstance() → wgpuInstanceRelease(_instance)
    // Note: V2 uses RAII and shared_ptr for automatic cleanup in correct order
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

    std::cout << "=== Triangle2 example completed ===" << std::endl;
    return 0;
}