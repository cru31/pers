#include "TriangleRenderer.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/ISurfaceFramebuffer.h"
#include "pers/graphics/SurfaceFramebuffer.h"
#include "pers/graphics/OffscreenFramebuffer.h"
#include "pers/graphics/RenderPassConfig.h"
#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/IShaderModule.h"
#include "pers/graphics/IRenderPipeline.h"
#include "pers/graphics/IRenderPassEncoder.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"
#include <chrono>
#include <array>

TriangleRenderer::~TriangleRenderer() {
    LOG_INFO("TriangleRenderer",
        "Destructor called");
    cleanup();
}

bool TriangleRenderer::initialize(const std::shared_ptr<pers::IInstance>& instance, const TriangleRendererConfig& config) {
    _config = config;
    _instance = instance;
    
    if (!_instance) {
        LOG_ERROR("TriangleRenderer",
            "Invalid instance provided");
        return false;
    }
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "Initializing with size: %dx%d", _config.windowSize.x, _config.windowSize.y);
    
    // Surface will be created and set by the app
    // Wait for surface to be set
    LOG_INFO("TriangleRenderer",
        "Waiting for surface to be set...");
    
    LOG_INFO("TriangleRenderer",
        "Renderer initialized successfully");
    return true;
}

bool TriangleRenderer::initializeGraphics(const pers::NativeSurfaceHandle& surface) {
    // Step 1: Set surface
    setSurface(surface);
    if (!_surface.isValid()) {
        LOG_ERROR("TriangleRenderer",
            "Invalid surface provided");
        return false;
    }
    
    // Step 2: Initialize device
    LOG_INFO("TriangleRenderer",
        "Requesting physical device...");
    
    auto physicalDevice = requestPhysicalDevice(surface);
    if (!physicalDevice) {
        LOG_ERROR("TriangleRenderer",
            "Failed to request physical device");
        return false;
    }
    setPhysicalDevice(physicalDevice);
    
    auto logicalDevice = createLogicalDevice(physicalDevice);
    if (!logicalDevice) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create logical device");
        return false;
    }
    setLogicalDevice(logicalDevice);
    
    // Get and set queue
    auto queue = logicalDevice->getQueue();
    if (!queue) {
        LOG_ERROR("TriangleRenderer",
            "Failed to get queue from device");
        return false;
    }
    setQueue(queue);
    
    // Step 3: Create surface framebuffer and swap chain
    LOG_INFO("TriangleRenderer",
        "Creating surface framebuffer and swap chain...");
    
    // Create surface framebuffer
    _surfaceFramebuffer = std::make_shared<pers::SurfaceFramebuffer>(logicalDevice);
    
    // Build swap chain description using builder with CONFIG values
    pers::SwapChainDescBuilder builder;
    pers::SwapChainDesc swapChainDesc = builder
        .setSize(_config.windowSize.x, _config.windowSize.y)
        .setFormat(_config.colorFormat)  // From config!
        .setPresentMode(_config.presentMode)  // From config!
        .setUsage(pers::TextureUsage::RenderAttachment)
        .setDebugName("TriangleSwapChain")
        .build();
    
    // Create swap chain through surface framebuffer
    if (!_surfaceFramebuffer->create(surface, swapChainDesc)) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create swap chain");
        return false;
    }
    
    // Get resource factory from logical device
    const auto& resourceFactory = logicalDevice->getResourceFactory();
    if (!resourceFactory) {
        LOG_ERROR("TriangleRenderer",
            "Failed to get resource factory from device");
        return false;
    }
    
    // NOTE: NOT creating separate depth framebuffer - SurfaceFramebuffer handles it internally
    // This fixes the double depth buffer allocation bug (Review issue #1)
    
    LOG_INFO("TriangleRenderer",
        "Framebuffers created successfully");
    
    LOG_INFO("TriangleRenderer",
        "Graphics initialization complete!");
    return true;
}

// Setter functions
void TriangleRenderer::setSurface(const pers::NativeSurfaceHandle& surface) {
    _surface = surface;
}

void TriangleRenderer::setPhysicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice) {
    _physicalDevice = physicalDevice;
}

void TriangleRenderer::setLogicalDevice(const std::shared_ptr<pers::ILogicalDevice>& device) {
    _device = device;
}

void TriangleRenderer::setQueue(const std::shared_ptr<pers::IQueue>& queue) {
    _queue = queue;
}


std::shared_ptr<pers::IPhysicalDevice> TriangleRenderer::requestPhysicalDevice(const pers::NativeSurfaceHandle& surface) {
    if (!_instance || !surface.isValid()) {
        LOG_ERROR("TriangleRenderer",
            "Instance or surface not ready");
        return nullptr;
    }
    
    // Request adapter compatible with our surface
    pers::PhysicalDeviceOptions options;
    options.powerPreference = pers::PowerPreference::HighPerformance;
    options.compatibleSurface = surface;  // Request adapter compatible with this surface
    
    LOG_INFO("TriangleRenderer",
        "Requesting physical device with high performance preference...");
    
    auto physicalDevice = _instance->requestPhysicalDevice(options);
    
    if (!physicalDevice) {
        LOG_ERROR("TriangleRenderer",
            "Failed to get physical device");
        return nullptr;
    }
    
    // Print device capabilities
    auto caps = physicalDevice->getCapabilities();
    LOG_INFO("TriangleRenderer",
        "Physical device obtained:");
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
    if (!physicalDevice->supportsSurface(surface)) {
        LOG_ERROR("TriangleRenderer",
            "Physical device doesn't support the surface!");
        return nullptr;
    }
    
    LOG_INFO("TriangleRenderer",
        "Physical device supports the surface");
    
    return physicalDevice;
}

std::shared_ptr<pers::ILogicalDevice> TriangleRenderer::createLogicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice) {
    if (!physicalDevice) {
        LOG_ERROR("TriangleRenderer",
            "Physical device not provided");
        return nullptr;
    }
    
    // Setup device descriptor
    pers::LogicalDeviceDesc deviceDesc;
    deviceDesc.enableValidation = _config.enableValidation;  // Use configured validation setting
    deviceDesc.debugName = "TriangleRendererDevice";
    deviceDesc.timeout = _config.deviceTimeout;  // Use configured timeout
    
    // For now, we don't request any special features or limits
    // Just use the adapter's defaults
    
    LOG_INFO("TriangleRenderer",
        "Creating logical device...");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Validation: %s", deviceDesc.enableValidation ? "Enabled" : "Disabled");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "  - Timeout: %lld ms", deviceDesc.timeout.count());
    
    auto device = physicalDevice->createLogicalDevice(deviceDesc);
    
    if (!device) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create logical device");
        return nullptr;
    }
    
    LOG_INFO("TriangleRenderer",
        "Logical device created successfully");
    
    return device;
}


bool TriangleRenderer::createTriangleResources() {
    LOG_INFO("TriangleRenderer",
        "Creating triangle resources...");
    
    if (!_device) {
        LOG_ERROR("TriangleRenderer",
            "Device not ready");
        return false;
    }
    
    // Get resource factory
    const auto& factory = _device->getResourceFactory();
    if (!factory) {
        LOG_ERROR("TriangleRenderer",
            "Failed to get resource factory");
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
    vertexBufferDesc.debugName = "TriangleVertexBuffer";
    
    // Use createInitializableDeviceBuffer for synchronous data upload
    _vertexBuffer = factory->createInitializableDeviceBuffer(
        vertexBufferDesc,
        vertices.data(),
        vertices.size() * sizeof(float)
    );
    
    if (!_vertexBuffer) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create vertex buffer with initial data");
        return false;
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
        LOG_ERROR("TriangleRenderer",
            "Failed to create vertex shader");
        return false;
    }
    
    pers::ShaderModuleDesc fragmentShaderDesc;
    fragmentShaderDesc.code = fragmentShaderCode;
    fragmentShaderDesc.stage = pers::ShaderStage::Fragment;
    fragmentShaderDesc.entryPoint = "main";
    fragmentShaderDesc.debugName = "TriangleFragmentShader";
    
    auto fragmentShader = factory->createShaderModule(fragmentShaderDesc);
    if (!fragmentShader) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create fragment shader");
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
    colorTarget.format = _config.colorFormat;  // Use configured color format
    colorTarget.writeMask = pers::ColorWriteMask::All;
    pipelineDesc.colorTargets.push_back(colorTarget);
    
    // Multisample state (no MSAA for now)
    pipelineDesc.multisample.count = 1;
    
    // Depth-stencil state (must match the framebuffer's depth format)
    pipelineDesc.depthStencil.format = _config.depthFormat;
    pipelineDesc.depthStencil.depthWriteEnabled = true;
    pipelineDesc.depthStencil.depthCompare = pers::CompareFunction::Less;
    
    _renderPipeline = factory->createRenderPipeline(pipelineDesc);
    if (!_renderPipeline) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create render pipeline");
        return false;
    }
    
    // 4. Create render pass configuration (once, reused every frame)
    _renderPassConfig = std::make_unique<pers::RenderPassConfig>();
    
    // Configure color attachment
    pers::RenderPassConfig::ColorConfig colorConfig;
    colorConfig.loadOp = pers::LoadOp::Clear;
    colorConfig.storeOp = pers::StoreOp::Store;
    colorConfig.clearColor = { _config.clearColor.r, _config.clearColor.g, _config.clearColor.b, _config.clearColor.a };  // Use configured clear color
    _renderPassConfig->addColorAttachment(colorConfig);
    
    // Configure depth attachment
    pers::RenderPassConfig::DepthStencilConfig depthConfig;
    depthConfig.depthLoadOp = pers::LoadOp::Clear;
    depthConfig.depthStoreOp = pers::StoreOp::Store;
    depthConfig.depthClearValue = _config.clearDepth;
    depthConfig.depthReadOnly = false;
    _renderPassConfig->setDepthStencilConfig(depthConfig);
    
    _renderPassConfig->setLabel("TriangleRenderPass");
    
    LOG_INFO("TriangleRenderer",
        "Triangle resources created successfully");
    return true;
}

void TriangleRenderer::renderFrame() {
    if (!_surfaceFramebuffer || !_renderPipeline || !_vertexBuffer || !_queue) {
        return;  // Not ready to render
    }
    
    // 1. Acquire next image from surface framebuffer
    if (!_surfaceFramebuffer->acquireNextImage()) {
        LOG_WARNING("TriangleRenderer",
            "Failed to acquire next image");
        return;
    }
    
    // 2. Create command encoder
    auto commandEncoder = _device->createCommandEncoder();
    if (!commandEncoder) {
        LOG_ERROR("TriangleRenderer",
            "Failed to create command encoder");
        return;
    }
    
    // 3. Begin render pass using pre-configured settings
    if (!_renderPassConfig) {
        LOG_ERROR("TriangleRenderer",
            "Render pass configuration not initialized");
        return;
    }
    
    // Make descriptor from config + current framebuffer
    pers::RenderPassDesc renderPassDesc = _renderPassConfig->makeDescriptor(_surfaceFramebuffer);
    
    auto renderPass = commandEncoder->beginRenderPass(renderPassDesc);
    if (!renderPass) {
        LOG_ERROR("TriangleRenderer",
            "Failed to begin render pass");
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
        LOG_ERROR("TriangleRenderer",
            "Failed to finish command encoder");
        return;
    }
    
    _queue->submit(commandBuffer);
    
    // 9. Present the frame
    _surfaceFramebuffer->present();
    
    // Frame counter for debugging
    _frameCounter++;
    if (_frameCounter % _config.frameLogInterval == 0) {
        pers::Logger::Instance().LogFormat(pers::LogLevel::Debug, "TriangleRenderer", PERS_SOURCE_LOC,
            "Frame: %d", _frameCounter);
    }
}

void TriangleRenderer::onResize(int width, int height) {
    glm::ivec2 newSize(width, height);
    if (_config.windowSize == newSize) {
        return; // No change
    }
    
    _config.windowSize = newSize;
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "TriangleRenderer", PERS_SOURCE_LOC,
        "Resized to: %dx%d", _config.windowSize.x, _config.windowSize.y);
    
    // Resize surface framebuffer with new size
    if (_surfaceFramebuffer) {
        _surfaceFramebuffer->resize(width, height);
    }
}

void TriangleRenderer::cleanup() {
    // Clean up in EXACT reverse order of creation
    
    // 8. _renderPassConfig (created last in createTriangleResources)
    _renderPassConfig.reset();
    
    // 7. _renderPipeline (created in createTriangleResources)
    _renderPipeline.reset();
    
    // 6. _vertexBuffer (created in createTriangleResources)
    _vertexBuffer.reset();
    
    // 5. _surfaceFramebuffer (created in initializeGraphics)
    _surfaceFramebuffer.reset();

    // 4. _queue (set in initializeGraphics)
    _queue.reset();
    
    // 3. _device (logical device, created in initializeGraphics)
    _device.reset();
    
    // 2. _physicalDevice (set in initializeGraphics)
    _physicalDevice.reset();
    
    // 1. _surface (set first in initializeGraphics)
    _surface = pers::NativeSurfaceHandle();
    
    // Note: _instance is NOT reset here as it was provided externally
    // The application that created the instance should destroy it
    
    LOG_INFO("TriangleRenderer",
        "Cleanup completed");
}