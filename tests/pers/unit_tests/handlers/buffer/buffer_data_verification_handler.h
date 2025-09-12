#pragma once

#include "../../test_handler_base.h"
#include "pers/graphics/backends/IGraphicsInstanceFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/buffers/BufferTypes.h"
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/IMappableBuffer.h"
#include <chrono>
#include <vector>
#include <random>
#include <memory>

namespace pers::tests {

// Forward declaration
class WebGPUTestHelper;

/**
 * Buffer Data Verification Handler
 * 
 * This handler specifically tests that data written to buffers
 * can be correctly read back and verified through multiple methods:
 * 1. Direct mapping (for mappable buffers)
 * 2. Copy to staging buffer and read back
 * 3. GPU compute shader verification
 * 4. Rendering verification (use buffer as vertex data and check output)
 */
class BufferDataVerificationHandler : public TestHandlerBase {
public:
    BufferDataVerificationHandler();
    ~BufferDataVerificationHandler() override;
    
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
    
private:
    // Verification methods
    TestResult verifyThroughDirectMapping(const TestVariation& variation);
    TestResult verifyThroughStagingCopy(const TestVariation& variation);
    TestResult verifyThroughComputeShader(const TestVariation& variation);
    TestResult verifyThroughRendering(const TestVariation& variation);
    TestResult verifyInitializableDeviceBuffer(const TestVariation& variation);
    
    // Helper methods
    bool initializeDevice(bool enableLogging = true, bool enableValidation = true);
    void cleanupDevice();
    
    // Data generation methods
    struct TestPattern {
        std::vector<uint8_t> data;
        uint32_t checksum;
        std::string patternName;
        
        // Pattern types
        enum Type {
            Sequential,     // 0, 1, 2, 3, ...
            Random,        // Random with seed
            Gradient,      // Smooth gradient values
            Binary,        // Alternating 0xFF and 0x00
            FloatingPoint, // Float test patterns
            Structured     // Complex struct with specific layout
        };
        
        static TestPattern generate(Type type, size_t size, uint32_t seed = 42);
        bool verify(const uint8_t* actual, size_t size) const;
        uint32_t calculateCRC32() const;
    };
    
    // Method 1: Direct Mapping Verification
    struct DirectMappingVerification {
        bool execute(
            std::shared_ptr<IMappableBuffer> buffer,
            const TestPattern& pattern
        );
    };
    
    // Method 2: Staging Buffer Copy Verification  
    struct StagingCopyVerification {
        std::shared_ptr<ICommandEncoder> encoder;
        std::shared_ptr<IQueue> queue;
        
        bool execute(
            std::shared_ptr<IBuffer> sourceBuffer,
            const TestPattern& pattern, 
            const std::shared_ptr<ILogicalDevice>& device 
        );
        
    private:
        std::shared_ptr<IMappableBuffer> createStagingBuffer(size_t size, 
            const std::shared_ptr<ILogicalDevice>& device );

        bool downloadDeviceBufferToStagingSync(
            std::shared_ptr<DeviceBuffer> deviceBuffer,
            std::shared_ptr<DeferredStagingBuffer> stagingBuffer,
            const std::shared_ptr<ILogicalDevice>& device 
        );
        bool waitForGPU();
    };
    
    // Method 3: Compute Shader Verification
    struct ComputeShaderVerification {
        std::shared_ptr<ILogicalDevice> device;
        std::shared_ptr<IResourceFactory> resourceFactory;
        
        bool execute(
            std::shared_ptr<IBuffer> buffer,
            const TestPattern& pattern
        );
        
    private:
        std::string generateVerificationShader(size_t dataSize);
        std::shared_ptr<IBuffer> createResultBuffer();
        bool runVerificationCompute(
            std::shared_ptr<IBuffer> dataBuffer,
            std::shared_ptr<IBuffer> expectedBuffer,
            std::shared_ptr<IBuffer> resultBuffer
        );
        bool checkResults(std::shared_ptr<IBuffer> resultBuffer);
    };
    
    // Method 4: Rendering Verification
    struct RenderingVerification {
        std::shared_ptr<ILogicalDevice> device;
        std::shared_ptr<IResourceFactory> resourceFactory;
        
        bool execute(
            std::shared_ptr<IBuffer> vertexBuffer,
            const TestPattern& pattern
        );
        
    private:
        bool renderWithVertexBuffer(std::shared_ptr<IBuffer> vertexBuffer);
        bool verifyRenderedOutput(const TestPattern& expectedPattern);
        std::vector<uint32_t> readbackFramebuffer();
    };
    
    // Performance metrics
    struct VerificationMetrics {
        double writeTimeMs = 0.0;
        double verifyTimeMs = 0.0;
        double totalTimeMs = 0.0;
        size_t bytesVerified = 0;
        bool verificationPassed = false;
        std::string verificationMethod;
        std::string failureDetails;
    };
    
    VerificationMetrics measureVerification(
        const std::function<bool()>& verifyFunc,
        const std::string& method,
        size_t dataSize
    );
    
    // Test different buffer sizes
    std::vector<size_t> getTestSizes() {
        return {
            256,           // Tiny
            1024,          // 1KB
            4096,          // 4KB  
            65536,         // 64KB
            1048576,       // 1MB
            16777216,      // 16MB
            67108864       // 64MB
        };
    }
    
    // Member variables
    std::shared_ptr<IGraphicsInstanceFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _logicalDevice;
    std::shared_ptr<ICommandEncoder> _commandEncoder;
    std::shared_ptr<IQueue> _queue;
    
    // Verification components
    DirectMappingVerification _directVerifier;
    StagingCopyVerification _stagingVerifier;
    ComputeShaderVerification _computeVerifier;
    RenderingVerification _renderVerifier;
    
    // Test state
    std::vector<VerificationMetrics> _allMetrics;
    bool _verbose = false;
};

} // namespace pers::tests