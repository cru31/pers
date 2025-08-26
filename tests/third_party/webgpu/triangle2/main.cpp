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
    std::cout << "=== WebGPU Triangle2 Example ===" << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "WebGPU Triangle2", nullptr, nullptr);
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
    // [PERS ENGINE CALL STACK]
    // Application::OnInitialize()
    // └─> Renderer::Initialize(RendererConfig{backend: WebGPU, validation: true})
    //     └─> WebGPURenderer::CreateInstance()
    //         └─> wgpuCreateInstance(&instanceDesc)
    // Parameters:
    //   - instanceDesc: 엔진에서는 RendererConfig로부터 생성
    //     - validation layers 설정
    //     - backend 선택 (WebGPU/Vulkan/D3D12)
    //     - 로깅 콜백 설정
    WGPUInstanceDescriptor instanceDesc = {};
    demo.instance = wgpuCreateInstance(&instanceDesc);
    if (!demo.instance) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Create surface for Windows
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    
    WGPUSurfaceSourceWindowsHWND surfaceSource = {};
    surfaceSource.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    surfaceSource.hinstance = hinstance;
    surfaceSource.hwnd = hwnd;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceSource);
    
    // [PERS ENGINE CALL STACK]
    // Application::OnInitialize()
    // └─> Window::CreateSurface() [Platform layer에서 처리]
    //     └─> WebGPURenderer::CreateSurface(windowHandle)
    //         └─> wgpuInstanceCreateSurface(instance, &surfaceDesc)
    // Parameters:
    //   - surfaceDesc: Window system integration (HWND, NSWindow, X11, etc.)
    //   - Pers에서는 플랫폼 추상화 레이어가 처리
    demo.surface = wgpuInstanceCreateSurface(demo.instance, &surfaceDesc);
#else
    #error "Only Windows platform is currently supported"
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
    
    // [PERS ENGINE CALL STACK]
    // Renderer::Initialize()
    // └─> WebGPURenderer::RequestAdapter()
    //     └─> wgpuInstanceRequestAdapter(instance, &adapterOptions, callback)
    // Parameters:
    //   - adapterOptions.powerPreference: RendererConfig.powerMode
    //   - adapterOptions.compatibleSurface: SwapChain의 surface
    //   - callback: 내부 핸들러가 처리 후 Promise/Future로 반환
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
    
    // [PERS ENGINE CALL STACK]
    // Renderer::Initialize()
    // └─> WebGPURenderer::CreateDevice()
    //     └─> wgpuAdapterRequestDevice(adapter, &deviceDesc, callback)
    // Parameters:
    //   - deviceDesc.requiredFeatures: 엔진이 필요로 하는 기능들
    //   - deviceDesc.requiredLimits: 최소 리소스 제한
    //   - deviceLostCallback: 엔진의 에러 복구 시스템과 연결
    //   - uncapturedErrorCallback: Logger 시스템으로 전달
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
    // [PERS ENGINE CALL STACK]
    // Renderer::Initialize()
    // └─> WebGPURenderer::_queue = wgpuDeviceGetQueue(_device)
    // Note: Queue는 Renderer 내부에서 관리, 외부 노출 없음
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
    // [PERS ENGINE CALL STACK]
    // Material::Create("shaders/basic.wgsl")
    // └─> ShaderManager::LoadShader(path)
    //     └─> WebGPURenderer::CreateShaderModule(shaderCode)
    //         └─> wgpuDeviceCreateShaderModule(device, &shaderDesc)
    // Parameters:
    //   - shaderCode: 파일에서 로드하거나 #include 처리 후 전달
    //   - label: 디버깅용 이름 (파일명 기반)
    //   - Pers는 ShaderCache로 중복 컴파일 방지
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
    // [PERS ENGINE CALL STACK]
    // Pipeline::Create(PipelineDesc{shader, vertexLayout, ...})
    // └─> WebGPURenderer::CreatePipelineLayout(bindGroupLayouts[])
    //     └─> wgpuDeviceCreatePipelineLayout(device, &layoutDesc)
    // Parameters:
    //   - bindGroupLayouts: Pers의 4-set 계층 (Frame/Pass/Material/Object)
    //   - Pers는 표준 레이아웃을 캐싱하여 재사용
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.label = makeStringView("Pipeline Layout");
    pipelineLayoutDesc.bindGroupLayoutCount = 0;
    
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(demo.device, &pipelineLayoutDesc);
    
    // Get surface capabilities
    // [PERS ENGINE CALL STACK]
    // SwapChain::Initialize()
    // └─> wgpuSurfaceGetCapabilities(surface, adapter, &caps)
    // Note: SwapChain이 내부적으로 처리, 표면 포맷 자동 선택
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
    
    // [PERS ENGINE CALL STACK]
    // Pipeline::Create(PipelineDesc{...})
    // └─> PipelineCache::GetOrCreate(desc) [중복 생성 방지]
    //     └─> WebGPURenderer::CreateRenderPipeline(pipelineDesc)
    //         └─> wgpuDeviceCreateRenderPipeline(device, &pipelineDesc)
    // Parameters:
    //   - vertexState: VertexLayout에서 자동 생성
    //   - fragmentState: Material의 shader 설정
    //   - primitiveState: Mesh의 topology
    //   - Pers는 PSO(Pipeline State Object) 캐싱으로 성능 최적화
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
    
    // [PERS ENGINE CALL STACK]
    // SwapChain::Configure(width, height, format)
    // └─> wgpuSurfaceConfigure(surface, &config)
    // Note: Window resize 시에도 호출됨
    wgpuSurfaceConfigure(demo.surface, &demo.config);
    demo.isInitialized = true;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Get current texture
        // [PERS ENGINE CALL STACK]
        // Renderer::BeginFrame()
        // └─> SwapChain::AcquireNextTexture()
        //     └─> wgpuSurfaceGetCurrentTexture(surface, &texture)
        // Returns: 현재 프레임의 백버퍼 텍스처
        // Note: Pers는 이를 RenderTarget으로 래핑
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
        // [PERS ENGINE CALL STACK]
        // Renderer::BeginFrame()
        // └─> CommandPool::GetOrCreate() [재사용 풀]
        //     └─> WebGPUCommandRecorder::WebGPUCommandRecorder()
        //         └─> wgpuDeviceCreateCommandEncoder(device, &desc)
        // Returns: std::shared_ptr<ICommandRecorder>
        // Note: Pers는 CommandRecorder를 프레임마다 재사용
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
        
        // [PERS ENGINE CALL STACK]
        // ICommandRecorder::BeginRenderPass(nullptr, ClearValue{0.0f, 0.2f, 0.4f, 1.0f})
        // └─> WebGPUCommandRecorder::BeginRenderPass(target, clear)
        //     └─> wgpuCommandEncoderBeginRenderPass(encoder, &passDesc)
        // Parameters:
        //   - nullptr: 현재 백버퍼를 의미 (Pers의 핵심 철학)
        //   - clear: ClearValue 구조체로 전달
        //   - passDesc.colorAttachments[0].view: 백버퍼 texture view
        // State: _state = RecorderState::InRenderPass
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        
        // Draw triangle
        // [PERS ENGINE CALL STACK]
        // ICommandRecorder::SetPipeline(pipeline)
        // └─> WebGPUCommandRecorder::SetPipeline(pipeline)
        //     └─> wgpuRenderPassEncoderSetPipeline(pass, pipeline->GetHandle())
        wgpuRenderPassEncoderSetPipeline(renderPass, demo.renderPipeline);
        
        // [PERS ENGINE CALL STACK]
        // ICommandRecorder::Draw(3, 1)
        // └─> WebGPUCommandRecorder::Draw(vertexCount, instanceCount)
        //     └─> wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0)
        // Note: Pers에서는 주로 DrawIndexed 사용 (Mesh 기반)
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
        
        // [PERS ENGINE CALL STACK]
        // ICommandRecorder::EndRenderPass()
        // └─> WebGPUCommandRecorder::EndRenderPass()
        //     └─> wgpuRenderPassEncoderEnd(pass)
        // State: _state = RecorderState::Recording
        // Note: Pers는 EndRenderPass 호출 없이 EndFrame 시 에러 발생
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);
        
        // Finish command buffer
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        cmdBufferDesc.label = makeStringView("Command Buffer");
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuCommandEncoderRelease(encoder);
        
        // Submit command buffer
        // [PERS ENGINE CALL STACK]
        // Renderer::EndFrame(commandRecorder)
        // └─> WebGPUCommandRecorder::Finish() [validation check]
        //     └─> wgpuCommandEncoderFinish(encoder, &desc)
        // └─> WebGPURenderer::Submit(commandBuffer)
        //     └─> wgpuQueueSubmit(queue, 1, &commandBuffer)
        // Validation:
        //   - if (IsInRenderPass()) throw "RenderPass not ended!"
        //   - Frame경계 검증
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        
        // Present
        // [PERS ENGINE CALL STACK]
        // Renderer::EndFrame(commandRecorder) [계속]
        // └─> SwapChain::Present()
        //     └─> wgpuSurfacePresent(surface)
        // Note: Triple buffering - 다음 프레임 준비
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
    
    std::cout << "=== Triangle2 example completed ===" << std::endl;
    return 0;
}