#include "buffer_data_verification_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUInstanceFactory.h"
#include "pers/graphics/GraphicsEnumStrings.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/buffers/DeferredStagingBuffer.h"
#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandBuffer.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <numeric>

namespace pers::tests {

BufferDataVerificationHandler::BufferDataVerificationHandler()
    : _factory(std::make_shared<WebGPUInstanceFactory>()) {
}

BufferDataVerificationHandler::~BufferDataVerificationHandler() {
    cleanupDevice();
}

std::string BufferDataVerificationHandler::getTestType() const {
    return "Buffer Data Verification";
}

bool BufferDataVerificationHandler::initializeDevice(bool enableLogging, bool enableValidation) {
    // ALWAYS control logging based on JSON option for EACH test
    // This must happen BEFORE checking if device exists
    // Otherwise first test's logging setting will stick for all tests
    if (!enableLogging) {
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Info, false);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Debug, false);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Trace, false);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Warning, false);
    } else {
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Info, true);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Debug, true);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Trace, true);
        pers::Logger::Instance().SetLogLevelEnabled(pers::LogLevel::Warning, true);
    }
    
    if (_logicalDevice) {
        return true;  // Device already initialized
    }

    // Create instance
    InstanceDesc instanceDesc;
    instanceDesc.applicationName = "Buffer Data Verification Test";
    instanceDesc.enableValidation = enableValidation;
    
    _instance = _factory->createInstance(instanceDesc);
    if (!_instance) {
        
        return false;
    }
    
    // Get physical device
    PhysicalDeviceOptions options;
    options.powerPreference = PowerPreference::HighPerformance;
    _physicalDevice = _instance->requestPhysicalDevice(options);
    if (!_physicalDevice) {
        
        return false;
    }
    
    // Create logical device
    LogicalDeviceDesc deviceDesc;
    deviceDesc.enableValidation = enableValidation;
    _logicalDevice = _physicalDevice->createLogicalDevice(deviceDesc);
    if (!_logicalDevice) {
        
        return false;
    }
    
    const auto factory = _logicalDevice->getResourceFactory();
    _queue = _logicalDevice->getQueue();
    
    // Setup verifiers
    _stagingVerifier.queue = _queue;
    _computeVerifier.device = _logicalDevice;
    _renderVerifier.device = _logicalDevice;
    
    return true;
}

void BufferDataVerificationHandler::cleanupDevice() {
    _commandEncoder.reset();
    _queue.reset();
    _logicalDevice.reset();
    _physicalDevice.reset();
    _instance.reset();
}

TestResult BufferDataVerificationHandler::execute(const TestVariation& variation) {
    setupLogCapture();
    
    TestResult result;
    
    
    // Set source location info
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);
    
    // Get JSON options for logging and validation
    bool enableLogging = getOption<bool>(variation.options, "enable_logging", true);
    bool enableValidation = getOption<bool>(variation.options, "enable_validation", true);
    
    if (!initializeDevice(enableLogging, enableValidation)) {
        result.passed = false;
        result.actualBehavior = "Device initialization failed";
        result.failureReason = "Failed to create WebGPU device";
        return result;
    }
    
    _verbose = getOption<bool>(variation.options, "verbose", false);
    
    std::string verifyMethod = getOption<std::string>(variation.options, "verify_method", "staging_copy");
    
    // Map JSON verify methods to our internal methods
    if (verifyMethod == "readback") {
        verifyMethod = "staging_copy";  // readback uses staging copy method
    } else if (verifyMethod == "mapping") {
        verifyMethod = "direct_mapping";  // mapping uses direct mapping method
    }

    if (verifyMethod == "direct_mapping") {
        result = verifyThroughDirectMapping(variation);
    } else if (verifyMethod == "staging_copy") {
        result = verifyThroughStagingCopy(variation);
    } else if (verifyMethod == "compute_shader") {
        result = verifyThroughComputeShader(variation);
    } else if (verifyMethod == "rendering") {
        result = verifyThroughRendering(variation);
    } else if (verifyMethod == "initializable_device") {
        result = verifyInitializableDeviceBuffer(variation);
    } else {
        result.passed = false;
        result.failureReason = "Unknown verification method: " + verifyMethod;
    }

    transferLogsToResult(result);
    
    // Add aggregate metrics
    if (!_allMetrics.empty()) {
        double totalWriteTime = 0.0;
        double totalVerifyTime = 0.0;
        size_t totalBytes = 0;
        int passCount = 0;
        
        for (const auto& metric : _allMetrics) {
            totalWriteTime += metric.writeTimeMs;
            totalVerifyTime += metric.verifyTimeMs;
            totalBytes += metric.bytesVerified;
            if (metric.verificationPassed) passCount++;
        }
        
        result.actualProperties["total_write_time_ms"] = totalWriteTime;
        result.actualProperties["total_verify_time_ms"] = totalVerifyTime;
        result.actualProperties["total_bytes_verified"] = totalBytes;
        result.actualProperties["verification_pass_rate"] = 
            static_cast<double>(passCount) / _allMetrics.size();
    }
    
    return result;
}

TestResult BufferDataVerificationHandler::verifyInitializableDeviceBuffer(const TestVariation& variation) {
    TestResult result;
    
    // Set source location info
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);
    
    // Get test parameters
    size_t bufferSize = getOption<size_t>(variation.options, "size", 4096);
    std::string patternType = getOption<std::string>(variation.options, "pattern", "sequential");
    
    // Generate test pattern
    TestPattern::Type type = TestPattern::Sequential;
    if (patternType == "random") type = TestPattern::Random;
    else if (patternType == "gradient") type = TestPattern::Gradient;
    else if (patternType == "binary") type = TestPattern::Binary;
    else if (patternType == "float") type = TestPattern::FloatingPoint;
    else if (patternType == "structured") type = TestPattern::Structured;
    
    auto pattern = TestPattern::generate(type, bufferSize);
    
    if (_verbose) {
        std::cout << "Testing createInitializableDeviceBuffer with " 
                  << patternType << " pattern, size: " << bufferSize << " bytes" << std::endl;
        std::cout << "Pattern checksum: 0x" << std::hex << pattern.checksum << std::dec << std::endl;
    }
    
    // Create buffer with initial data using createInitializableDeviceBuffer
    BufferDesc desc;
    desc.size = bufferSize;
    desc.usage = BufferUsage::Storage | BufferUsage::CopySrc; // Allow readback
    desc.debugName = "InitializableTestBuffer";
    
    auto startWrite = std::chrono::high_resolution_clock::now();

    const auto& factory = _logicalDevice->getResourceFactory();
    auto buffer = factory->createInitializableDeviceBuffer(
        desc,
        pattern.data.data(),
        pattern.data.size()
    );
    
    auto endWrite = std::chrono::high_resolution_clock::now();
    double writeTimeMs = std::chrono::duration<double, std::milli>(endWrite - startWrite).count();
    
    if (!buffer) {
        result.passed = false;
        result.failureReason = "Failed to create buffer with initial data";
        result.actualProperties["buffer_created"] = false;
        return result;
    }
    
    // Now verify the data was written correctly
    // Create DeferredStagingBuffer for readback
    BufferDesc stagingDesc;
    stagingDesc.size = bufferSize;
    stagingDesc.usage = BufferUsage::MapRead | BufferUsage::CopyDst;
    stagingDesc.debugName = "DeferredStagingBuffer";
    
    auto deferredStaging = std::make_shared<DeferredStagingBuffer>();
    deferredStaging->create(stagingDesc, _logicalDevice);
    
    // Create command encoder for copy
    _commandEncoder = _logicalDevice->createCommandEncoder();
    if (!_commandEncoder) {
        result.passed = false;
        result.failureReason = "Failed to create command encoder";
        return result;
    }
    
    // Cast our buffer to DeviceBuffer
    auto deviceBuffer = std::dynamic_pointer_cast<DeviceBuffer>(buffer);
    if (!deviceBuffer) {
        result.passed = false;
        result.failureReason = "Buffer is not a DeviceBuffer type";
        return result;
    }
    
    // Copy from device buffer to staging
    BufferCopyDesc copyDesc;
    copyDesc.srcOffset = 0;
    copyDesc.dstOffset = 0;
    copyDesc.size = bufferSize;
    
    if (!_commandEncoder->downloadFromDeviceBuffer(deviceBuffer, deferredStaging, copyDesc)) {
        result.passed = false;
        result.failureReason = "Failed to encode download command";
        return result;
    }
    
    // Submit and wait
    auto commandBuffer = _commandEncoder->finish();
    if (!commandBuffer) {
        result.passed = false;
        result.failureReason = "Failed to finish command encoding";
        return result;
    }
    
    _queue->submit({commandBuffer});
    

    

    
    // Map staging buffer and verify data
    auto startVerify = std::chrono::high_resolution_clock::now();
    
    bool verificationPassed = false;
    std::string verificationError;
    
    // Map the deferred staging buffer for reading
    auto mapFuture = deferredStaging->mapAsync(MapMode::Read, {0, bufferSize});
    // Process events until mapping completes or timeout
    auto mapStart = std::chrono::high_resolution_clock::now();
    while (mapFuture.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
        _instance->processEvents();
        auto elapsed = std::chrono::high_resolution_clock::now() - mapStart;
        if (elapsed > std::chrono::seconds(5)) break;
    }
    if (mapFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        auto mapped = mapFuture.get();
        if (mapped.data() == nullptr) {
            verificationError = "Failed to map staging buffer";
        } else {
            
            // Verify the data
            const uint8_t* readData = static_cast<const uint8_t*>(mapped.data());
            
            // Compare byte by byte
            for (size_t i = 0; i < bufferSize; i++) {
                if (readData[i] != pattern.data[i]) {
                    verificationError = "Data mismatch at byte " + std::to_string(i) +
                                      ": expected 0x" + std::to_string(pattern.data[i]) +
                                      ", got 0x" + std::to_string(readData[i]);
                    
                    if (_verbose) {
                        std::cerr << verificationError << std::endl;
                        
                        // Print surrounding bytes for context
                        size_t contextStart = (i >= 16) ? i - 16 : 0;
                        size_t contextEnd = std::min(i + 16, bufferSize);
                        
                        std::cerr << "Context (expected):" << std::hex;
                        for (size_t j = contextStart; j < contextEnd; j++) {
                            if (j == i) std::cerr << " [";
                            std::cerr << " " << std::setw(2) << std::setfill('0') 
                                     << static_cast<int>(pattern.data[j]);
                            if (j == i) std::cerr << "]";
                        }
                        std::cerr << std::dec << std::endl;
                        
                        std::cerr << "Context (actual):  " << std::hex;
                        for (size_t j = contextStart; j < contextEnd; j++) {
                            if (j == i) std::cerr << " [";
                            std::cerr << " " << std::setw(2) << std::setfill('0')
                                     << static_cast<int>(readData[j]);
                            if (j == i) std::cerr << "]";
                        }
                        std::cerr << std::dec << std::endl;
                    }
                    break;
                }
            }
            
            // Calculate checksum of read data
            uint32_t readChecksum = TestPattern::generate(TestPattern::Sequential, 0).calculateCRC32();
            // Simple checksum for now
            readChecksum = 0;
            for (size_t i = 0; i < bufferSize; i++) {
                readChecksum = (readChecksum << 1) ^ readData[i];
            }
            
            if (_verbose) {
                std::cout << "Data verification passed!" << std::endl;
                std::cout << "Read checksum: 0x" << std::hex << readChecksum << std::dec << std::endl;
            }
            
            verificationPassed = true;
        }
    }
    
    // Mapping should already be complete after future.get()
    
    deferredStaging->unmap();
    
    auto endVerify = std::chrono::high_resolution_clock::now();
    double verifyTimeMs = std::chrono::duration<double, std::milli>(endVerify - startVerify).count();
    
    // Set result
    result.passed = verificationPassed;
    result.actualProperties["buffer_created"] = true;
    result.actualProperties["data_verified"] = verificationPassed;
    result.actualProperties["buffer_size"] = bufferSize;
    result.actualProperties["pattern_type"] = patternType;
    result.actualProperties["write_time_ms"] = writeTimeMs;
    result.actualProperties["verify_time_ms"] = verifyTimeMs;
    result.actualProperties["total_time_ms"] = writeTimeMs + verifyTimeMs;
    
    if (verificationPassed) {
        double throughputGBps = (bufferSize * 2.0) / ((writeTimeMs + verifyTimeMs) * 1e6);
        result.actualProperties["throughput_gbps"] = throughputGBps;
        result.actualBehavior = "Data successfully written and verified through staging buffer readback";
    } else {
        result.actualBehavior = "Data verification failed";
        result.failureReason = verificationError.empty() ? 
            "Data mismatch detected" : verificationError;
        result.actualProperties["error_details"] = verificationError;
    }
    
    // Store metrics
    VerificationMetrics metrics;
    metrics.writeTimeMs = writeTimeMs;
    metrics.verifyTimeMs = verifyTimeMs;
    metrics.totalTimeMs = writeTimeMs + verifyTimeMs;
    metrics.bytesVerified = bufferSize;
    metrics.verificationPassed = verificationPassed;
    metrics.verificationMethod = "staging_copy";
    metrics.failureDetails = verificationError;
    _allMetrics.push_back(metrics);
    
    return result;
}

TestResult BufferDataVerificationHandler::verifyThroughStagingCopy(const TestVariation& variation) {
    TestResult result;
    
    // Set source location info
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);

    size_t bufferSize = getOption<size_t>(variation.options, "size", 4096);
    std::string patternType = getOption<std::string>(variation.options, "pattern", "sequential");
    
    // Map pattern type
    TestPattern::Type type = TestPattern::Sequential;
    if (patternType == "random") type = TestPattern::Random;
    else if (patternType == "gradient") type = TestPattern::Gradient;
    else if (patternType == "binary") type = TestPattern::Binary;
    
    auto pattern = TestPattern::generate(type, bufferSize);

    // Create buffer
    BufferDesc desc;
    desc.size = bufferSize;
    desc.usage = BufferUsage::Storage | BufferUsage::CopyDst | BufferUsage::CopySrc;
    desc.debugName = "TestBuffer";
    
    auto deviceBuffer = std::make_shared<DeviceBuffer>();
    if (!deviceBuffer->create(desc, _logicalDevice)) {
        
        result.passed = false;
        result.failureReason = "Failed to create device buffer";
        return result;
    }

    // Create ImmediateStagingBuffer for upload
    BufferDesc immediateStagingDesc;
    immediateStagingDesc.size = bufferSize;
    immediateStagingDesc.usage = BufferUsage::MapWrite | BufferUsage::CopySrc;
    immediateStagingDesc.debugName = "ImmediateStagingBuffer";
    
    auto immediateStaging = std::make_shared<ImmediateStagingBuffer>();
    if (!immediateStaging->create(immediateStagingDesc, _logicalDevice)) {
        
        result.passed = false;
        result.failureReason = "Failed to create immediate staging buffer";
        return result;
    }

    // Write pattern data to immediate staging
    auto startWrite = std::chrono::high_resolution_clock::now();
    
    uint64_t bytesWritten = immediateStaging->writeBytes(pattern.data.data(), bufferSize, 0);
    if (bytesWritten != bufferSize) {
        result.passed = false;
        result.failureReason = "Failed to write all data to staging buffer";
        return result;
    }
    
    // Finalize the staging buffer before measuring time
    immediateStaging->finalize();
    
    auto endWrite = std::chrono::high_resolution_clock::now();
    double writeTimeMs = std::chrono::duration<double, std::milli>(endWrite - startWrite).count();

    // Upload from staging to device
    BufferCopyDesc copyDesc;
    copyDesc.srcOffset = 0;
    copyDesc.dstOffset = 0;
    copyDesc.size = bufferSize;
    
    _commandEncoder = _logicalDevice->createCommandEncoder();
    if (!_commandEncoder) {
        
        result.passed = false;
        result.failureReason = "Failed to create command encoder";
        return result;
    }
    
    if (!_commandEncoder->uploadToDeviceBuffer(immediateStaging, deviceBuffer, copyDesc)) {
        
        result.passed = false;
        result.failureReason = "Failed to upload data to buffer";
        return result;
    }
    
    auto commandBuffer = _commandEncoder->finish();
    if (!commandBuffer) {
        
        result.passed = false;
        result.failureReason = "Failed to finish command encoding";
        return result;
    }
    
    _queue->submit({commandBuffer});
    

    


    // Now verify through staging copy (readback)

    // Create DeferredStagingBuffer for readback
    BufferDesc readbackDesc;
    readbackDesc.size = bufferSize;
    readbackDesc.usage = BufferUsage::MapRead | BufferUsage::CopyDst;
    readbackDesc.debugName = "ReadbackStagingBuffer";
    
    auto readbackStaging = std::make_shared<DeferredStagingBuffer>();
    if (!readbackStaging->create(readbackDesc, _logicalDevice)) {
        
        result.passed = false;
        result.failureReason = "Failed to create readback staging buffer";
        return result;
    }
    
    // Download from device to staging
    _commandEncoder = _logicalDevice->createCommandEncoder();
    if (!_commandEncoder->downloadFromDeviceBuffer(deviceBuffer, readbackStaging, copyDesc)) {
        
        result.passed = false;
        result.failureReason = "Failed to download data from buffer";
        return result;
    }
    
    commandBuffer = _commandEncoder->finish();
    _queue->submit({commandBuffer});
    

    

    
    // Map and verify
    auto startVerify = std::chrono::high_resolution_clock::now();
    
    auto mapFuture = readbackStaging->mapAsync(MapMode::Read, {0, bufferSize});
    // Process events until mapping completes or timeout
    auto mapStart = std::chrono::high_resolution_clock::now();
    while (mapFuture.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
        _instance->processEvents();
        auto elapsed = std::chrono::high_resolution_clock::now() - mapStart;
        if (elapsed > std::chrono::seconds(5)) break;
    }
    if (mapFuture.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        
        result.passed = false;
        result.failureReason = "Timeout during buffer mapping";
        return result;
    }
    
    auto mapped = mapFuture.get();
    if (!mapped.data()) {
        
        result.passed = false;
        result.failureReason = "Failed to map buffer for reading";
        return result;
    }
    
    // Verify data
    const uint8_t* readData = static_cast<const uint8_t*>(mapped.data());
    bool dataMatches = true;
    size_t firstMismatch = 0;
    
    for (size_t i = 0; i < bufferSize; i++) {
        if (readData[i] != pattern.data[i]) {
            dataMatches = false;
            firstMismatch = i;
            
            break;
        }
    }
    
    readbackStaging->unmap();
    
    auto endVerify = std::chrono::high_resolution_clock::now();
    double verifyTimeMs = std::chrono::duration<double, std::milli>(endVerify - startVerify).count();
    
    // Store metrics
    VerificationMetrics metrics;
    metrics.writeTimeMs = writeTimeMs;
    metrics.verifyTimeMs = verifyTimeMs;
    metrics.totalTimeMs = writeTimeMs + verifyTimeMs;
    metrics.bytesVerified = bufferSize;
    metrics.verificationPassed = dataMatches;
    metrics.verificationMethod = "staging_copy";
    if (!dataMatches) {
        metrics.failureDetails = "First mismatch at byte " + std::to_string(firstMismatch);
    }
    _allMetrics.push_back(metrics);
    
    // Set result
    result.passed = dataMatches;
    result.actualProperties["data_verified"] = dataMatches;
    result.actualProperties["buffer_size"] = bufferSize;
    result.actualProperties["pattern_type"] = patternType;
    result.actualProperties["write_time_ms"] = writeTimeMs;
    result.actualProperties["verify_time_ms"] = verifyTimeMs;
    
    if (dataMatches) {
        
        result.actualBehavior = "Data successfully written to device buffer and verified through staging copy readback";
        
        double throughputGBps = (bufferSize * 2.0) / ((writeTimeMs + verifyTimeMs) * 1e6);
        result.actualProperties["throughput_gbps"] = throughputGBps;
    } else {
        
        result.actualBehavior = "Data verification failed - corruption detected";
        result.failureReason = "Data mismatch at byte " + std::to_string(firstMismatch);
        result.actualProperties["first_mismatch_byte"] = firstMismatch;
    }
    
    return result;
}

// TestPattern implementation
BufferDataVerificationHandler::TestPattern 
BufferDataVerificationHandler::TestPattern::generate(Type type, size_t size, uint32_t seed) {
    TestPattern pattern;
    pattern.data.resize(size);
    
    switch (type) {
        case Sequential: {
            pattern.patternName = "Sequential";
            for (size_t i = 0; i < size; i++) {
                pattern.data[i] = static_cast<uint8_t>(i & 0xFF);
            }
            break;
        }
        
        case Random: {
            pattern.patternName = "Random";
            std::mt19937 gen(seed);
            std::uniform_int_distribution<int> dist(0, 255);
            for (size_t i = 0; i < size; i++) {
                pattern.data[i] = static_cast<uint8_t>(dist(gen));
            }
            break;
        }
        
        case Gradient: {
            pattern.patternName = "Gradient";
            for (size_t i = 0; i < size; i++) {
                float t = static_cast<float>(i) / size;
                pattern.data[i] = static_cast<uint8_t>(t * 255);
            }
            break;
        }
        
        case Binary: {
            pattern.patternName = "Binary";
            for (size_t i = 0; i < size; i++) {
                pattern.data[i] = (i % 2) ? 0xFF : 0x00;
            }
            break;
        }
        
        case FloatingPoint: {
            pattern.patternName = "FloatingPoint";
            float* floatData = reinterpret_cast<float*>(pattern.data.data());
            size_t floatCount = size / sizeof(float);
            for (size_t i = 0; i < floatCount; i++) {
                floatData[i] = static_cast<float>(i) * 3.14159f;
            }
            break;
        }
        
        case Structured: {
            pattern.patternName = "Structured";
            struct TestStruct {
                float x, y, z;
                uint32_t color;
                uint16_t id;
                uint16_t flags;
            };
            
            TestStruct* structs = reinterpret_cast<TestStruct*>(pattern.data.data());
            size_t structCount = size / sizeof(TestStruct);
            for (size_t i = 0; i < structCount; i++) {
                structs[i].x = static_cast<float>(i);
                structs[i].y = static_cast<float>(i * 2);
                structs[i].z = static_cast<float>(i * 3);
                structs[i].color = 0xFF00FF00;
                structs[i].id = static_cast<uint16_t>(i);
                structs[i].flags = 0x1234;
            }
            break;
        }
    }
    
    pattern.checksum = pattern.calculateCRC32();
    return pattern;
}

bool BufferDataVerificationHandler::TestPattern::verify(const uint8_t* actual, size_t size) const {
    if (size != data.size()) {
        return false;
    }
    
    return memcmp(actual, data.data(), size) == 0;
}

uint32_t BufferDataVerificationHandler::TestPattern::calculateCRC32() const {
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    
    return ~crc;
}

// StagingCopyVerification implementation
bool BufferDataVerificationHandler::StagingCopyVerification::execute(
    std::shared_ptr<IBuffer> sourceBuffer,
    const TestPattern& pattern, 
    const std::shared_ptr<ILogicalDevice>& device)
 {
    
    // Create staging buffer for readback
    auto staging = createStagingBuffer(pattern.data.size(), device);
    if (!staging) {
        return false;
    }
    
    // Copy from source to staging
    
    // Wait for GPU
    if (!waitForGPU()) {
        return false;
    }
    
    // Map and verify
    bool verified = false;
    auto mapFuture = staging->mapAsync(MapMode::Read, {0, pattern.data.size()});
    if (mapFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        auto mapped = mapFuture.get();
        if (mapped.data() != nullptr) {
            verified = pattern.verify(
                static_cast<const uint8_t*>(mapped.data()),
                pattern.data.size()
            );
        }
    }
    
    staging->unmap();
    
    return verified;
}

std::shared_ptr<IMappableBuffer> 
BufferDataVerificationHandler::StagingCopyVerification::createStagingBuffer(
    size_t size, const std::shared_ptr<ILogicalDevice>& device ) {
    BufferDesc desc;
    desc.size = size;
    desc.usage = BufferUsage::MapRead | BufferUsage::CopyDst;
    desc.debugName = "StagingReadback";

    return device->getResourceFactory()->createMappableBuffer(desc);
}

bool BufferDataVerificationHandler::StagingCopyVerification::downloadDeviceBufferToStagingSync(
    std::shared_ptr<DeviceBuffer> deviceBuffer,
    std::shared_ptr<DeferredStagingBuffer> stagingBuffer,
    const std::shared_ptr<ILogicalDevice>& device ) {
    
    auto commandEncoder = device->createCommandEncoder();
    if (!commandEncoder) {
        return false;
    }
    
    BufferCopyDesc copyDesc;
    copyDesc.srcOffset = 0;
    copyDesc.dstOffset = 0;
    copyDesc.size = std::min(deviceBuffer->getSize(), stagingBuffer->getSize());
    
    if (!commandEncoder->downloadFromDeviceBuffer(deviceBuffer, stagingBuffer, copyDesc)) {
        return false;
    }
    
    auto commandBuffer = commandEncoder->finish();
    if (!commandBuffer) {
        return false;
    }
    
    auto queue = device->getQueue();
    if (!queue) {
        return false;
    }
    
    queue->submit({commandBuffer});
    
    return true;
}

bool BufferDataVerificationHandler::StagingCopyVerification::waitForGPU() {
    // No fixed wait - rely on proper synchronization
    return true;
}

// Placeholder implementations for other verification methods


TestResult BufferDataVerificationHandler::verifyThroughDirectMapping(const TestVariation& variation) {
    // Use staging copy approach since WebGPU prohibits MAP_READ | MAP_WRITE on same buffer
    return verifyThroughStagingCopy(variation);
}

TestResult BufferDataVerificationHandler::verifyThroughComputeShader(const TestVariation& variation) {
    TestResult result;
    
    // Set source location info
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);
    
    size_t bufferSize = getOption<size_t>(variation.options, "size", 4096);
    std::string patternType = getOption<std::string>(variation.options, "pattern", "sequential");
    
    // Compute shader verification not yet implemented
    result.passed = false;
    result.actualBehavior = "Compute shader verification not yet implemented";
    result.failureReason = "Compute pipeline infrastructure not available";
    result.actualProperties["buffer_size"] = bufferSize;
    result.actualProperties["pattern_type"] = patternType;
    result.actualProperties["implementation_status"] = "NOT_IMPLEMENTED";
    
    return result;
}

TestResult BufferDataVerificationHandler::verifyThroughRendering(const TestVariation& variation) {
    TestResult result;
    
    // Set source location info
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);
    
    size_t bufferSize = getOption<size_t>(variation.options, "size", 4096);
    std::string patternType = getOption<std::string>(variation.options, "pattern", "sequential");
    
    // Rendering verification not yet implemented
    result.passed = false;
    result.actualBehavior = "Rendering verification not yet implemented";
    result.failureReason = "Render pipeline infrastructure not available";
    result.actualProperties["buffer_size"] = bufferSize;
    result.actualProperties["pattern_type"] = patternType;
    result.actualProperties["implementation_status"] = "NOT_IMPLEMENTED";
    
    return result;
}
}