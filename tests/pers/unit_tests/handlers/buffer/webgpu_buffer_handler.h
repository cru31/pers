#pragma once

#include "../../test_handler_base.h"
#include "pers/graphics/backends/IGraphicsInstanceFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/buffers/BufferTypes.h"
#include "pers/graphics/buffers/IBuffer.h"
#include <chrono>
#include <vector>

namespace pers::tests {

/**
 * Test handler for WebGPUBuffer (basic device-only buffer)
 * Tests creation, destruction, size limits, and usage flags
 */
class WebGPUBufferHandler : public TestHandlerBase {
public:
    WebGPUBufferHandler();
    ~WebGPUBufferHandler() override;
    
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
    
private:
    // Test methods
    TestResult testBasicCreation(const TestVariation& variation);
    TestResult testSizeLimits(const TestVariation& variation);
    TestResult testUsageFlags(const TestVariation& variation);
    TestResult testInvalidParameters(const TestVariation& variation);
    TestResult testMemoryAlignment(const TestVariation& variation);
    TestResult testMultipleCreation(const TestVariation& variation);
    
    // Helper methods
    bool initializeDevice();
    void cleanupDevice();
    BufferUsage parseUsageFlags(const std::vector<std::string>& flags);
    size_t getAlignedSize(size_t size, BufferUsage usage);
    
    // Performance measurement
    struct CreationMetrics {
        double avgCreationTimeMs = 0.0;
        double minCreationTimeMs = 1e9;
        double maxCreationTimeMs = 0.0;
        size_t totalBuffersCreated = 0;
        size_t totalBytesAllocated = 0;
    };
    
    void updateMetrics(double timeMs, size_t size);
    
    // Member variables
    std::shared_ptr<IGraphicsInstanceFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _logicalDevice;
    std::shared_ptr<IResourceFactory> _resourceFactory;
    
    // Test state
    std::vector<std::shared_ptr<IBuffer>> _createdBuffers;
    CreationMetrics _metrics;
    bool _verbose = false;
};

} // namespace pers::tests