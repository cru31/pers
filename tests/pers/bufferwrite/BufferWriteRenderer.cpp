#include "BufferWriteRenderer.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ISwapChain.h"
#include "pers/graphics/ISurfaceFramebuffer.h"
#include "pers/graphics/SurfaceFramebuffer.h"
#include "pers/graphics/RenderPassConfig.h"
#include "pers/graphics/IFramebuffer.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "ResourceLoader.h"
#include "pers/graphics/IShaderModule.h"
#include "pers/graphics/IRenderPipeline.h"
#include "pers/graphics/IRenderPassEncoder.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/utils/Logger.h"

BufferWriteRenderer::~BufferWriteRenderer() {
    LOG_INFO("BufferWriteRenderer",
        "Destructor called");
    cleanup();
}

bool BufferWriteRenderer::initialize(const std::shared_ptr<pers::IInstance>& instance, const BufferWriteRendererConfig& config) {
    _config = config;
    _instance = instance;
    
    if (!_instance) {
        LOG_ERROR("BufferWriteRenderer",
            "Invalid instance provided");
        return false;
    }
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "Initializing with size: %dx%d", _config.windowSize.x, _config.windowSize.y);
    
    // Surface will be created and set by the app
    // Wait for surface to be set
    LOG_INFO("BufferWriteRenderer",
        "Waiting for surface to be set...");
    
    LOG_INFO("BufferWriteRenderer",
        "Renderer initialized successfully");
    return true;
}

bool BufferWriteRenderer::initializeGraphics(const pers::NativeSurfaceHandle& surface) {
    // Step 1: Set surface
    setSurface(surface);
    if (!_surface.isValid()) {
        LOG_ERROR("BufferWriteRenderer",
            "Invalid surface provided");
        return false;
    }
    
    // Step 2: Initialize device
    LOG_INFO("BufferWriteRenderer",
        "Requesting physical device...");
    
    auto physicalDevice = requestPhysicalDevice(surface);
    if (!physicalDevice) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to request physical device");
        return false;
    }
    setPhysicalDevice(physicalDevice);
    
    auto logicalDevice = createLogicalDevice(physicalDevice);
    if (!logicalDevice) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create logical device");
        return false;
    }
    setLogicalDevice(logicalDevice);
    
    // Get and set queue
    auto queue = logicalDevice->getQueue();
    if (!queue) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to get queue from device");
        return false;
    }
    setQueue(queue);
    
    // Step 3: Create surface framebuffer and swap chain
    LOG_INFO("BufferWriteRenderer",
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
        .setDebugName("BufferWriteSwapChain")
        .build();
    
    // Create swap chain through surface framebuffer
    if (!_surfaceFramebuffer->create(surface, swapChainDesc)) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create swap chain");
        return false;
    }
    
    // Get resource factory from logical device
    const auto& resourceFactory = logicalDevice->getResourceFactory();
    if (!resourceFactory) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to get resource factory from device");
        return false;
    }
    
    // NOTE: NOT creating separate depth framebuffer - SurfaceFramebuffer handles it internally
    // This fixes the double depth buffer allocation bug (Review issue #1)
    
    LOG_INFO("BufferWriteRenderer",
        "Framebuffers created successfully");
    
    LOG_INFO("BufferWriteRenderer",
        "Graphics initialization complete!");
    return true;
}

// Setter functions
void BufferWriteRenderer::setSurface(const pers::NativeSurfaceHandle& surface) {
    _surface = surface;
}

void BufferWriteRenderer::setPhysicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice) {
    _physicalDevice = physicalDevice;
}

void BufferWriteRenderer::setLogicalDevice(const std::shared_ptr<pers::ILogicalDevice>& device) {
    _device = device;
}

void BufferWriteRenderer::setQueue(const std::shared_ptr<pers::IQueue>& queue) {
    _queue = queue;
}


std::shared_ptr<pers::IPhysicalDevice> BufferWriteRenderer::requestPhysicalDevice(const pers::NativeSurfaceHandle& surface) {
    if (!_instance || !surface.isValid()) {
        LOG_ERROR("BufferWriteRenderer",
            "Instance or surface not ready");
        return nullptr;
    }
    
    // Request adapter compatible with our surface
    pers::PhysicalDeviceOptions options;
    options.powerPreference = pers::PowerPreference::HighPerformance;
    options.compatibleSurface = surface;  // Request adapter compatible with this surface
    
    LOG_INFO("BufferWriteRenderer",
        "Requesting physical device with high performance preference...");
    
    auto physicalDevice = _instance->requestPhysicalDevice(options);
    
    if (!physicalDevice) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to get physical device");
        return nullptr;
    }
    
    // Print device capabilities
    auto caps = physicalDevice->getCapabilities();
    LOG_INFO("BufferWriteRenderer",
        "Physical device obtained:");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Device Name: %s", caps.deviceName.c_str());
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Driver Info: %s", caps.driverInfo.c_str());
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Max Texture 2D: %u", caps.maxTextureSize2D);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Max Texture 3D: %u", caps.maxTextureSize3D);
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Supports Compute: %s", caps.supportsCompute ? "Yes" : "No");
    
    // Check surface support
    if (!physicalDevice->supportsSurface(surface)) {
        LOG_ERROR("BufferWriteRenderer",
            "Physical device doesn't support the surface!");
        return nullptr;
    }
    
    LOG_INFO("BufferWriteRenderer",
        "Physical device supports the surface");
    
    return physicalDevice;
}

std::shared_ptr<pers::ILogicalDevice> BufferWriteRenderer::createLogicalDevice(const std::shared_ptr<pers::IPhysicalDevice>& physicalDevice) {
    if (!physicalDevice) {
        LOG_ERROR("BufferWriteRenderer",
            "Physical device not provided");
        return nullptr;
    }
    
    // Setup device descriptor
    pers::LogicalDeviceDesc deviceDesc;
    deviceDesc.enableValidation = _config.enableValidation;  // Use configured validation setting
    deviceDesc.debugName = "BufferWriteRendererDevice";
    deviceDesc.timeout = _config.deviceTimeout;  // Use configured timeout
    
    // For now, we don't request any special features or limits
    // Just use the adapter's defaults
    
    LOG_INFO("BufferWriteRenderer",
        "Creating logical device...");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Validation: %s", deviceDesc.enableValidation ? "Enabled" : "Disabled");
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "  - Timeout: %lld ms", deviceDesc.timeout.count());
    
    auto device = physicalDevice->createLogicalDevice(deviceDesc);
    
    if (!device) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create logical device");
        return nullptr;
    }
    
    LOG_INFO("BufferWriteRenderer",
        "Logical device created successfully");
    
    return device;
}


bool BufferWriteRenderer::createGraphicsResources() {
    LOG_INFO("BufferWriteRenderer",
        "Creating graphics resources...");
    
    if (!_device || !_queue) {
        LOG_ERROR("BufferWriteRenderer",
            "Device or queue not ready");
        return false;
    }
    
    // Get resource factory
    const auto& factory = _device->getResourceFactory();
    if (!factory) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to get resource factory");
        return false;
    }
    
    // 1. Load Stanford Bunny mesh
    MeshData bunnyMesh;
    if (!ResourceLoader::loadStanfordBunny(bunnyMesh)) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to load Stanford Bunny mesh");
        return false;
    }
    
    // Create GPU buffers from mesh data
    if (!ResourceLoader::createGPUBuffers(bunnyMesh, _device, _queue, _vertexBuffer, _indexBuffer)) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create GPU buffers for bunny mesh");
        return false;
    }
    
    // Store index count for drawing
    _indexCount = static_cast<uint32_t>(bunnyMesh.indices.size());
    
    LOG_INFO("BufferWriteRenderer",
        "Loaded Stanford Bunny with " + std::to_string(_indexCount) + " indices");
    
    // 2. Create shaders
    // Vertex shader with camera view and rotation
    const char* vertexShaderCode = R"(
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
}

@vertex
fn main(@location(0) position: vec3<f32>,
        @location(1) normal: vec3<f32>,
        @builtin(instance_index) instance_index: u32) -> VertexOutput {
    // Model rotation: 30 degrees per frame around Y axis
    let angle = f32(instance_index) * 0.00523599; // 0.3 degrees in radians
    let c = cos(angle);
    let s = sin(angle);

    // Rotate position around Y axis
    var rotated_pos: vec3<f32>;
    rotated_pos.x = position.x * c - position.z * s;
    rotated_pos.y = position.y;
    rotated_pos.z = position.x * s + position.z * c;

    // Rotate normal as well
    var rotated_normal: vec3<f32>;
    rotated_normal.x = normal.x * c - normal.z * s;
    rotated_normal.y = normal.y;
    rotated_normal.z = normal.x * s + normal.z * c;

    // Camera view matrix - looking from above at an angle
    // Camera position: (2, 3, 4) looking at origin
    let eye = vec3<f32>(0.67, 1.0, 1.33);
    let center = vec3<f32>(0.0, 0.0, 0.0);
    let up = vec3<f32>(0.0, 1.0, 0.0);

    // Calculate view matrix (simplified lookAt)
    let f = normalize(center - eye);
    let s_vec = normalize(cross(f, up));
    let u = cross(s_vec, f);

    // Apply view transformation
    var view_pos: vec3<f32>;
    view_pos.x = dot(rotated_pos - eye, s_vec);
    view_pos.y = dot(rotated_pos - eye, u);
    view_pos.z = dot(rotated_pos - eye, f);

    // Simple perspective projection
    let fov = 1.0; // ~60 degrees
    let aspect = 800.0 / 600.0;
    let near = 0.1;
    let far = 100.0;

    var proj_pos: vec4<f32>;
    proj_pos.x = view_pos.x / (aspect * fov);
    proj_pos.y = view_pos.y / fov;
    proj_pos.z = (view_pos.z * (far + near) - 2.0 * far * near) / (far - near);
    proj_pos.w = view_pos.z;

    var output: VertexOutput;
    output.position = proj_pos;
    output.color = vec4<f32>(rotated_normal, 1.0);
    return output;
}
)";

    // Fragment shader that uses normal-based color
    const char* fragmentShaderCode = R"(
@fragment
fn main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color;  // Use normal as color
}
)";
    pers::ShaderModuleDesc vertexShaderDesc;
    vertexShaderDesc.code = vertexShaderCode;
    vertexShaderDesc.stage = pers::ShaderStage::Vertex;
    vertexShaderDesc.entryPoint = "main";
    vertexShaderDesc.debugName = "BufferWriteVertexShader";
    
    auto vertexShader = factory->createShaderModule(vertexShaderDesc);
    if (!vertexShader) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create vertex shader");
        return false;
    }
    
    pers::ShaderModuleDesc fragmentShaderDesc;
    fragmentShaderDesc.code = fragmentShaderCode;
    fragmentShaderDesc.stage = pers::ShaderStage::Fragment;
    fragmentShaderDesc.entryPoint = "main";
    fragmentShaderDesc.debugName = "BufferWriteFragmentShader";
    
    auto fragmentShader = factory->createShaderModule(fragmentShaderDesc);
    if (!fragmentShader) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create fragment shader");
        return false;
    }
    
    // 3. Create render pipeline
    pers::RenderPipelineDesc pipelineDesc;
    pipelineDesc.vertex = vertexShader;
    pipelineDesc.fragment = fragmentShader;
    pipelineDesc.debugName = "BufferWritePipeline";
    
    // Vertex layout
    pers::VertexBufferLayout vertexLayout;
    vertexLayout.arrayStride = bunnyMesh.vertexStride;  // Use mesh's vertex stride
    vertexLayout.stepMode = pers::VertexStepMode::Vertex;
    
    // Position attribute
    pers::VertexAttribute positionAttribute;
    positionAttribute.format = pers::VertexFormat::Float32x3;
    positionAttribute.offset = 0;
    positionAttribute.shaderLocation = 0;
    vertexLayout.attributes.push_back(positionAttribute);
    
    // Add normal attribute if mesh has normals
    if (bunnyMesh.hasNormals) {
        pers::VertexAttribute normalAttribute;
        normalAttribute.format = pers::VertexFormat::Float32x3;
        normalAttribute.offset = bunnyMesh.normalOffset;
        normalAttribute.shaderLocation = 1;
        vertexLayout.attributes.push_back(normalAttribute);
    }
    
    pipelineDesc.vertexLayouts.push_back(vertexLayout);
    
    // Primitive state
    pipelineDesc.primitive.topology = pers::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.frontFace = pers::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = pers::CullMode::None;  // No culling for simple bufferwrite
    
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
        LOG_ERROR("BufferWriteRenderer",
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
    
    _renderPassConfig->setLabel("BufferWriteRenderPass");
    
    LOG_INFO("BufferWriteRenderer",
        "BufferWrite resources created successfully");
    return true;
}

void BufferWriteRenderer::renderFrame() {
    if (!_surfaceFramebuffer || !_renderPipeline || !_vertexBuffer || !_queue) {
        return;  // Not ready to render
    }
    
    // 1. Acquire next image from surface framebuffer
    if (!_surfaceFramebuffer->acquireNextImage()) {
        LOG_WARNING("BufferWriteRenderer",
            "Failed to acquire next image");
        return;
    }
    
    // 2. Create command encoder
    auto commandEncoder = _device->createCommandEncoder();
    if (!commandEncoder) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to create command encoder");
        return;
    }
    
    // 3. Begin render pass using pre-configured settings
    if (!_renderPassConfig) {
        LOG_ERROR("BufferWriteRenderer",
            "Render pass configuration not initialized");
        return;
    }
    
    // Make descriptor from config + current framebuffer
    pers::RenderPassDesc renderPassDesc = _renderPassConfig->makeDescriptor(_surfaceFramebuffer);
    
    auto renderPass = commandEncoder->beginRenderPass(renderPassDesc);
    if (!renderPass) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to begin render pass");
        return;
    }
    
    // 4. Set pipeline
    renderPass->setPipeline(_renderPipeline);
    
    // 5. Set vertex buffer
    renderPass->setVertexBuffer(0, _vertexBuffer, 0);
    
    // 6. Draw
    if (_indexBuffer && _indexCount > 0) {
        // Draw indexed bunny
        renderPass->setIndexBuffer(_indexBuffer, pers::IndexFormat::Uint32, 0);
        renderPass->drawIndexed(_indexCount, 1, 0, 0, _frameCounter);
    } else {
        // Fallback to triangle
        renderPass->draw(3, 1, 0, 0);
    }
    
    // 7. End render pass
    renderPass->end();
    
    // 8. Finish and submit commands
    auto commandBuffer = commandEncoder->finish();
    if (!commandBuffer) {
        LOG_ERROR("BufferWriteRenderer",
            "Failed to finish command encoder");
        return;
    }
    
    _queue->submit(commandBuffer);
    
    // 9. Present the frame
    _surfaceFramebuffer->present();
    
    // Frame counter for debugging
    _frameCounter++;
    if (_frameCounter % _config.frameLogInterval == 0) {
        pers::Logger::Instance().LogFormat(pers::LogLevel::Debug, "BufferWriteRenderer", PERS_SOURCE_LOC,
            "Frame: %d", _frameCounter);
    }
}

void BufferWriteRenderer::onResize(int width, int height) {
    glm::ivec2 newSize(width, height);
    if (_config.windowSize == newSize) {
        return; // No change
    }
    
    _config.windowSize = newSize;
    
    pers::Logger::Instance().LogFormat(pers::LogLevel::Info, "BufferWriteRenderer", PERS_SOURCE_LOC,
        "Resized to: %dx%d", _config.windowSize.x, _config.windowSize.y);
    
    // Resize surface framebuffer with new size
    if (_surfaceFramebuffer) {
        _surfaceFramebuffer->resize(width, height);
    }
}

void BufferWriteRenderer::cleanup() {
    // Clean up in EXACT reverse order of creation
    
    // 8. _renderPassConfig (created last in createGraphicsResources)
    _renderPassConfig.reset();
    
    // 7. _renderPipeline (created in createGraphicsResources)
    _renderPipeline.reset();
    
    // 6. _vertexBuffer and _indexBuffer (created in createGraphicsResources)
    _vertexBuffer.reset();
    _indexBuffer.reset();
    
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
    
    LOG_INFO("BufferWriteRenderer",
        "Cleanup completed");
}