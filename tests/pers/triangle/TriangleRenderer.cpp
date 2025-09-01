#include "TriangleRenderer.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/IBuffer.h"
#include "pers/graphics/IShaderModule.h"
#include "pers/graphics/IRenderPipeline.h"
#include "pers/graphics/IRenderPassEncoder.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoOrDie.h"
#include <chrono>
#include <array>

TriangleRenderer::~TriangleRenderer() {
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer", 
        "Destructor called", PERS_SOURCE_LOC);
    cleanup();
}

bool TriangleRenderer::initialize(std::shared_ptr<pers::IInstance> instance, const glm::ivec2& size) {
    _windowSize = size;
    _instance = instance;
    
    if (!_instance) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer", 
            "Invalid instance provided", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "Initializing with size: %dx%d", _windowSize.x, _windowSize.y);
    
    // Surface will be created and set by the app
    // Wait for surface to be set
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Waiting for surface to be set...", PERS_SOURCE_LOC);
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Renderer initialized successfully", PERS_SOURCE_LOC);
    return true;
}

void TriangleRenderer::setSurface(const pers::NativeSurfaceHandle& surface) {
    _surface = surface;
    
    if (!_surface.isValid()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Invalid surface provided", PERS_SOURCE_LOC);
        return;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Surface set, requesting physical device...", PERS_SOURCE_LOC);
    
    // Now that we have a surface, request a compatible adapter
    if (!requestPhysicalDevice()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to request physical device", PERS_SOURCE_LOC);
        return;
    }
    
    // Create logical device
    if (!createLogicalDevice()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create logical device", PERS_SOURCE_LOC);
        return;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Device initialization complete!", PERS_SOURCE_LOC);
}

bool TriangleRenderer::requestPhysicalDevice() {
    if (!_instance || !_surface.isValid()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Instance or surface not ready", PERS_SOURCE_LOC);
        return false;
    }
    
    // Request adapter compatible with our surface
    pers::PhysicalDeviceOptions options;
    options.powerPreference = pers::PowerPreference::HighPerformance;
    options.compatibleSurface = _surface;  // Request adapter compatible with this surface
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Requesting physical device with high performance preference...", PERS_SOURCE_LOC);
    
    _physicalDevice = _instance->requestPhysicalDevice(options);
    
    if (!_physicalDevice) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to get physical device", PERS_SOURCE_LOC);
        return false;
    }
    
    // Print device capabilities
    auto caps = _physicalDevice->getCapabilities();
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Physical device obtained:", PERS_SOURCE_LOC);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Device Name: %s", caps.deviceName.c_str());
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Driver Info: %s", caps.driverInfo.c_str());
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Max Texture 2D: %u", caps.maxTextureSize2D);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Max Texture 3D: %u", caps.maxTextureSize3D);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Supports Compute: %s", caps.supportsCompute ? "Yes" : "No");
    
    // Check surface support
    if (!_physicalDevice->supportsSurface(_surface)) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Physical device doesn't support the surface!", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Physical device supports the surface", PERS_SOURCE_LOC);
    
    return true;
}

bool TriangleRenderer::createLogicalDevice() {
    if (!_physicalDevice) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Physical device not ready", PERS_SOURCE_LOC);
        return false;
    }
    
    // Setup device descriptor
    pers::LogicalDeviceDesc deviceDesc;
    deviceDesc.enableValidation = true;  // Enable validation in debug
    deviceDesc.debugName = "TriangleRendererDevice";
    deviceDesc.timeout = std::chrono::seconds(10);  // 10 second timeout
    
    // For now, we don't request any special features or limits
    // Just use the adapter's defaults
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Creating logical device...", PERS_SOURCE_LOC);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Validation: %s", deviceDesc.enableValidation ? "Enabled" : "Disabled");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Timeout: %lld ms", deviceDesc.timeout.count());
    
    _device = _physicalDevice->createLogicalDevice(deviceDesc);
    
    if (!_device) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create logical device", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Logical device created successfully", PERS_SOURCE_LOC);
    
    // Get the queue
    _queue = _device->getQueue();
    
    if (!_queue) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to get queue from device", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Queue obtained successfully", PERS_SOURCE_LOC);
    
    return true;
}

bool TriangleRenderer::createSwapChain() {
    if (!_device || !_surface.isValid()) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Device or surface not ready", PERS_SOURCE_LOC);
        return false;
    }
    
    // Create swap chain descriptor
    pers::SwapChainDescBuilder builder;
    pers::SwapChainDesc swapChainDesc = builder
        .setSize(_windowSize.x, _windowSize.y)
        .setFormat(pers::TextureFormat::BGRA8Unorm)
        .setPresentMode(pers::PresentMode::Fifo)  // VSync
        .setUsage(pers::TextureUsage::RenderAttachment)
        .setDebugName("TriangleSwapChain")
        .build();
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "Creating swap chain: %dx%d", _windowSize.x, _windowSize.y);
    
    _swapChain = _device->createSwapChain(_surface, swapChainDesc);
    
    if (!_swapChain) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create swap chain", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Swap chain created successfully", PERS_SOURCE_LOC);
    
    return true;
}

bool TriangleRenderer::createTriangleResources() {
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Creating triangle resources...", PERS_SOURCE_LOC);
    
    if (!_device) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Device not ready", PERS_SOURCE_LOC);
        return false;
    }
    
    // Get resource factory
    auto factory = _device->getResourceFactory();
    if (!factory) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to get resource factory", PERS_SOURCE_LOC);
        return false;
    }
    
    // Create swap chain first
    if (!createSwapChain()) {
        return false;
    }
    
    // 1. Create vertex buffer with triangle data
    // Simple triangle vertices (position only, NDC coordinates)
    const std::array<float, 9> vertices = {
        // x,    y,    z
         0.0f,  0.5f, 0.0f,  // Top
        -0.5f, -0.5f, 0.0f,  // Bottom left
         0.5f, -0.5f, 0.0f   // Bottom right
    };
    
    pers::BufferDesc vertexBufferDesc;
    vertexBufferDesc.size = vertices.size() * sizeof(float);
    vertexBufferDesc.usage = pers::BufferUsage::Vertex | pers::BufferUsage::CopyDst;
    vertexBufferDesc.mappedAtCreation = true;
    vertexBufferDesc.debugName = "TriangleVertexBuffer";
    
    _vertexBuffer = factory->createBuffer(vertexBufferDesc);
    if (!_vertexBuffer) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create vertex buffer", PERS_SOURCE_LOC);
        return false;
    }
    
    // Copy vertex data if buffer was mapped at creation
    void* mappedData = _vertexBuffer->map();
    if (mappedData) {
        memcpy(mappedData, vertices.data(), vertices.size() * sizeof(float));
        _vertexBuffer->unmap();
    }
    
    // 2. Create shaders
    // Simple vertex shader
    const char* vertexShaderCode = R"(
@vertex
fn main(@location(0) position: vec3<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(position, 1.0);
}
)";
    
    // Simple fragment shader (red triangle)
    const char* fragmentShaderCode = R"(
@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);  // Red color
}
)";
    
    pers::ShaderModuleDesc vertexShaderDesc;
    vertexShaderDesc.code = vertexShaderCode;
    vertexShaderDesc.stage = pers::ShaderStage::Vertex;
    vertexShaderDesc.entryPoint = "main";
    vertexShaderDesc.debugName = "TriangleVertexShader";
    
    auto vertexShader = factory->createShaderModule(vertexShaderDesc);
    if (!vertexShader) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create vertex shader", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::ShaderModuleDesc fragmentShaderDesc;
    fragmentShaderDesc.code = fragmentShaderCode;
    fragmentShaderDesc.stage = pers::ShaderStage::Fragment;
    fragmentShaderDesc.entryPoint = "main";
    fragmentShaderDesc.debugName = "TriangleFragmentShader";
    
    auto fragmentShader = factory->createShaderModule(fragmentShaderDesc);
    if (!fragmentShader) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create fragment shader", PERS_SOURCE_LOC);
        return false;
    }
    
    // 3. Create render pipeline
    pers::RenderPipelineDesc pipelineDesc;
    pipelineDesc.vertex = vertexShader;
    pipelineDesc.fragment = fragmentShader;
    pipelineDesc.debugName = "TrianglePipeline";
    
    // Vertex layout
    pers::VertexBufferLayout vertexLayout;
    vertexLayout.arrayStride = 3 * sizeof(float);  // 3 floats per vertex
    vertexLayout.stepMode = pers::VertexStepMode::Vertex;
    
    pers::VertexAttribute positionAttribute;
    positionAttribute.format = pers::VertexFormat::Float32x3;
    positionAttribute.offset = 0;
    positionAttribute.shaderLocation = 0;
    vertexLayout.attributes.push_back(positionAttribute);
    
    pipelineDesc.vertexLayouts.push_back(vertexLayout);
    
    // Primitive state
    pipelineDesc.primitive.topology = pers::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.frontFace = pers::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = pers::CullMode::None;  // No culling for simple triangle
    
    // Color target (matches swap chain format)
    pers::ColorTargetState colorTarget;
    colorTarget.format = pers::TextureFormat::BGRA8Unorm;  // Standard swap chain format
    colorTarget.writeMask = pers::ColorWriteMask::All;
    pipelineDesc.colorTargets.push_back(colorTarget);
    
    // Multisample state (no MSAA for now)
    pipelineDesc.multisample.count = 1;
    
    _renderPipeline = factory->createRenderPipeline(pipelineDesc);
    if (!_renderPipeline) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create render pipeline", PERS_SOURCE_LOC);
        return false;
    }
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Triangle resources created successfully", PERS_SOURCE_LOC);
    return true;
}

void TriangleRenderer::renderFrame() {
    if (!_swapChain || !_renderPipeline || !_vertexBuffer || !_queue) {
        return;  // Not ready to render
    }
    
    // 1. Get current texture from swap chain
    auto textureView = _swapChain->getCurrentTextureView();
    if (!textureView) {
        pers::Logger::Instance().Log(pers::LogLevel::Warning, "TriangleRenderer",
            "Failed to get current texture view", PERS_SOURCE_LOC);
        return;
    }
    
    // 2. Create command encoder
    auto commandEncoder = _device->createCommandEncoder();
    if (!commandEncoder) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to create command encoder", PERS_SOURCE_LOC);
        return;
    }
    
    // 3. Begin render pass
    pers::RenderPassDesc renderPassDesc;
    renderPassDesc.label = "TriangleRenderPass";
    
    pers::RenderPassColorAttachment colorAttachment;
    colorAttachment.view = textureView;
    colorAttachment.loadOp = pers::LoadOp::Clear;
    colorAttachment.storeOp = pers::StoreOp::Store;
    colorAttachment.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };  // Clear to black
    renderPassDesc.colorAttachments.push_back(colorAttachment);
    
    auto renderPass = commandEncoder->beginRenderPass(renderPassDesc);
    if (!renderPass) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to begin render pass", PERS_SOURCE_LOC);
        return;
    }
    
    // 4. Set pipeline
    renderPass->setPipeline(_renderPipeline);
    
    // 5. Set vertex buffer
    renderPass->setVertexBuffer(0, _vertexBuffer, 0);
    
    // 6. Draw
    renderPass->draw(3, 1, 0, 0);  // 3 vertices, 1 instance
    
    // 7. End render pass
    renderPass->end();
    
    // 8. Finish and submit commands
    auto commandBuffer = commandEncoder->finish();
    if (!commandBuffer) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "TriangleRenderer",
            "Failed to finish command encoder", PERS_SOURCE_LOC);
        return;
    }
    
    _queue->submit(commandBuffer);
    
    // 9. Present swap chain
    _swapChain->present();
    
    // Frame counter for debugging
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {
        pers::Logger::Instance().LogFormat(pers::LogLevel::Debug, "TriangleRenderer", PERS_SOURCE_LOC,
            "Frame: %d", frameCount);
    }
}

void TriangleRenderer::onResize(int width, int height) {
    glm::ivec2 newSize(width, height);
    if (_windowSize == newSize) {
        return; // No change
    }
    
    _windowSize = newSize;
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "Resized to: %dx%d", _windowSize.x, _windowSize.y);
    
    // TODO: Recreate swap chain with new size
}

void TriangleRenderer::cleanup() {
    // Clean up in reverse order of creation
    
    // TODO: Release pipeline, buffers, swap chain
    
    // Release queue
    _queue.reset();
    
    // Release logical device
    _device.reset();
    
    // Release physical device
    _physicalDevice.reset();
    
    // Release surface (WebGPU will handle this)
    _surface = pers::NativeSurfaceHandle();
    
    // Release instance
    _instance.reset();
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Cleanup completed", PERS_SOURCE_LOC);
}