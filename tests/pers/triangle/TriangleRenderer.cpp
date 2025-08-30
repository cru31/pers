#include "TriangleRenderer.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/utils/Logger.h"
#include <chrono>

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

bool TriangleRenderer::createTriangleResources() {
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Creating triangle resources...", PERS_SOURCE_LOC);
    
    // TODO: Create triangle resources
    // 1. Create vertex buffer with triangle data
    // 2. Load/compile shaders
    // 3. Create render pipeline
    // 4. Create bind groups if needed
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "TriangleRenderer",
        "Triangle resources created", PERS_SOURCE_LOC);
    return true;
}

void TriangleRenderer::renderFrame() {
    // TODO: Actual rendering
    // 1. Get current texture from swap chain
    // 2. Create command encoder
    // 3. Begin render pass
    // 4. Set pipeline
    // 5. Set vertex buffer
    // 6. Draw
    // 7. End render pass
    // 8. Submit commands
    // 9. Present swap chain
    
    // For now, just count frames
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