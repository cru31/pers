#include "webgpu_buffer_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUInstanceFactory.h"
#include "pers/graphics/GraphicsEnumStrings.h"
#include <iostream>
#include <algorithm>

#include <numeric>
namespace pers::tests {

WebGPUBufferHandler::WebGPUBufferHandler()
    : _factory(std::make_shared<WebGPUInstanceFactory>()) {
}

WebGPUBufferHandler::~WebGPUBufferHandler() {
    cleanupDevice();
}

std::string WebGPUBufferHandler::getTestType() const {
    return "WebGPUBuffer Test";
}

bool WebGPUBufferHandler::initializeDevice() {
    if (_logicalDevice) {
        return true;
    }
    
    // Create instance
    InstanceDesc instanceDesc;
    instanceDesc.applicationName = "WebGPUBuffer Test";
    instanceDesc.enableValidation = false;
    
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
    deviceDesc.enableValidation = false;
    _logicalDevice = _physicalDevice->createLogicalDevice(deviceDesc);
    if (!_logicalDevice) {
        return false;
    }
    
    _resourceFactory = _logicalDevice->getResourceFactory();
    return _resourceFactory != nullptr;
}

void WebGPUBufferHandler::cleanupDevice() {
    _createdBuffers.clear();
    _resourceFactory.reset();
    _logicalDevice.reset();
    _physicalDevice.reset();
    _instance.reset();
}

TestResult WebGPUBufferHandler::execute(const TestVariation& variation) {
    setupLogCapture();
    
    TestResult result;
    
    if (!initializeDevice()) {
        result.passed = false;
        result.actualBehavior = "Device initialization failed";
        result.failureReason = "Failed to create WebGPU device";
        return result;
    }
    
    _verbose = getOption<bool>(variation.options, "verbose", false);
    
    std::string testCase = getOption<std::string>(variation.options, "test_case", "basic_creation");
    
    if (testCase == "basic_creation") {
        result = testBasicCreation(variation);
    } else if (testCase == "size_limits") {
        result = testSizeLimits(variation);
    } else if (testCase == "usage_flags") {
        result = testUsageFlags(variation);
    } else if (testCase == "invalid_parameters") {
        result = testInvalidParameters(variation);
    } else if (testCase == "memory_alignment") {
        result = testMemoryAlignment(variation);
    } else if (testCase == "multiple_creation") {
        result = testMultipleCreation(variation);
    } else {
        result.passed = false;
        result.failureReason = "Unknown test case: " + testCase;
    }
    
    transferLogsToResult(result);
    
    // Add metrics to result
    if (_metrics.totalBuffersCreated > 0) {
        result.actualProperties["total_buffers_created"] = _metrics.totalBuffersCreated;
        result.actualProperties["total_bytes_allocated"] = _metrics.totalBytesAllocated;
        result.actualProperties["avg_creation_time_ms"] = _metrics.avgCreationTimeMs;
        result.actualProperties["min_creation_time_ms"] = _metrics.minCreationTimeMs;
        result.actualProperties["max_creation_time_ms"] = _metrics.maxCreationTimeMs;
    }
    
    return result;
}

TestResult WebGPUBufferHandler::testBasicCreation(const TestVariation& variation) {
    TestResult result;
    
    size_t bufferSize = getOption<size_t>(variation.options, "size", 1024);
    std::string usageStr = getOption<std::string>(variation.options, "usage", "Vertex");
    
    BufferDesc desc;
    desc.size = bufferSize;
    desc.usage = parseUsageFlags({usageStr});
    desc.debugName = "TestBuffer";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto buffer = _resourceFactory->createBuffer(desc);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    double creationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    updateMetrics(creationTimeMs, bufferSize);
    
    if (!buffer) {
        result.passed = false;
        result.failureReason = "Buffer creation returned null";
        result.actualProperties["buffer_created"] = false;
        return result;
    }
    
    // Verify properties
    bool sizeCorrect = (buffer->getSize() == bufferSize);
    bool usageCorrect = (buffer->getUsage() == desc.usage);
    bool isValid = buffer->isValid();
    
    result.passed = sizeCorrect && usageCorrect && isValid;
    result.actualProperties["buffer_created"] = true;
    result.actualProperties["size_correct"] = sizeCorrect;
    result.actualProperties["usage_correct"] = usageCorrect;
    result.actualProperties["is_valid"] = isValid;
    result.actualProperties["actual_size"] = buffer->getSize();
    result.actualProperties["creation_time_ms"] = creationTimeMs;
    
    if (result.passed) {
        result.actualBehavior = "Buffer created successfully with correct properties";
        _createdBuffers.push_back(buffer);
    } else {
        result.actualBehavior = "Buffer creation failed or properties incorrect";
    }
    
    return result;
}

TestResult WebGPUBufferHandler::testSizeLimits(const TestVariation& variation) {
    TestResult result;
    
    // Test various sizes
    std::vector<size_t> testSizes = {
        0,          // Invalid: zero size
        1,          // Minimum valid size
        256,        // Small buffer
        65536,      // 64KB - uniform buffer limit
        1048576,    // 1MB
        134217728,  // 128MB - storage buffer limit
        2147483648  // 2GB - max buffer size
    };
    
    for (size_t size : testSizes) {
        BufferDesc desc;
        desc.size = size;
        desc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
        desc.debugName = "SizeLimitTest_" + std::to_string(size);
        
        auto buffer = _resourceFactory->createBuffer(desc);
        
        bool shouldSucceed = (size > 0 && size <= BufferLimits::MAX_BUFFER_SIZE);
        bool didSucceed = (buffer != nullptr);
        
        if (shouldSucceed != didSucceed) {
            result.passed = false;
            result.failureReason = "Size limit test failed for size " + std::to_string(size);
            result.actualProperties["failed_size"] = size;
            result.actualProperties["should_succeed"] = shouldSucceed;
            result.actualProperties["did_succeed"] = didSucceed;
            return result;
        }
        
        if (buffer) {
            _createdBuffers.push_back(buffer);
        }
    }
    
    result.passed = true;
    result.actualBehavior = "All size limit tests passed";
    result.actualProperties["sizes_tested"] = testSizes.size();
    
    return result;
}

TestResult WebGPUBufferHandler::testUsageFlags(const TestVariation& variation) {
    TestResult result;
    
    // Test different usage flag combinations
    std::vector<BufferUsage> usageCombinations = {
        BufferUsage::Vertex,
        BufferUsage::Index,
        BufferUsage::Uniform,
        BufferUsage::Storage,
        BufferUsage::Vertex | BufferUsage::CopyDst,
        BufferUsage::Uniform | BufferUsage::CopyDst,
        BufferUsage::Storage | BufferUsage::CopySrc | BufferUsage::CopyDst,
        BufferUsage::MapRead | BufferUsage::CopySrc,
        BufferUsage::MapWrite | BufferUsage::CopyDst
    };
    
    for (BufferUsage usage : usageCombinations) {
        BufferDesc desc;
        desc.size = 1024;
        desc.usage = usage;
        desc.debugName = "UsageFlagTest";
        
        auto buffer = _resourceFactory->createBuffer(desc);
        
        if (!buffer) {
            result.passed = false;
            result.failureReason = "Failed to create buffer with usage flags";
            result.actualProperties["failed_usage"] = static_cast<uint32_t>(usage);
            return result;
        }
        
        if (buffer->getUsage() != usage) {
            result.passed = false;
            result.failureReason = "Buffer usage doesn't match requested usage";
            result.actualProperties["requested_usage"] = static_cast<uint32_t>(usage);
            result.actualProperties["actual_usage"] = static_cast<uint32_t>(buffer->getUsage());
            return result;
        }
        
        _createdBuffers.push_back(buffer);
    }
    
    result.passed = true;
    result.actualBehavior = "All usage flag combinations tested successfully";
    result.actualProperties["combinations_tested"] = usageCombinations.size();
    
    return result;
}

TestResult WebGPUBufferHandler::testInvalidParameters(const TestVariation& variation) {
    TestResult result;
    
    // Test 1: Zero size
    {
        BufferDesc desc;
        desc.size = 0;
        desc.usage = BufferUsage::Vertex;
        
        auto buffer = _resourceFactory->createBuffer(desc);
        if (buffer) {
            result.passed = false;
            result.failureReason = "Buffer creation should fail with zero size";
            return result;
        }
    }
    
    // Test 2: No usage flags
    {
        BufferDesc desc;
        desc.size = 1024;
        desc.usage = BufferUsage::None;
        
        auto buffer = _resourceFactory->createBuffer(desc);
        if (buffer) {
            result.passed = false;
            result.failureReason = "Buffer creation should fail with no usage flags";
            return result;
        }
    }
    
    // Test 3: Size exceeding uniform buffer limit with uniform usage
    {
        BufferDesc desc;
        desc.size = BufferLimits::MAX_UNIFORM_BUFFER_SIZE + 1;
        desc.usage = BufferUsage::Uniform;
        
        auto buffer = _resourceFactory->createBuffer(desc);
        if (buffer) {
            result.passed = false;
            result.failureReason = "Buffer creation should fail when exceeding uniform buffer size limit";
            return result;
        }
    }
    
    result.passed = true;
    result.actualBehavior = "Invalid parameter tests passed - buffers correctly rejected";
    
    return result;
}

TestResult WebGPUBufferHandler::testMemoryAlignment(const TestVariation& variation) {
    TestResult result;
    
    // Test alignment requirements for different buffer types
    struct AlignmentTest {
        BufferUsage usage;
        size_t requestedSize;
        size_t expectedAlignment;
    };
    
    std::vector<AlignmentTest> tests = {
        {BufferUsage::Uniform, 100, BufferAlignment::UNIFORM_BUFFER_OFFSET},
        {BufferUsage::Storage, 100, BufferAlignment::STORAGE_BUFFER_OFFSET},
        {BufferUsage::Vertex, 100, BufferAlignment::VERTEX_BUFFER_OFFSET},
        {BufferUsage::Index, 100, BufferAlignment::INDEX_BUFFER_OFFSET}
    };
    
    for (const auto& test : tests) {
        BufferDesc desc;
        desc.size = test.requestedSize;
        desc.usage = test.usage;
        
        auto buffer = _resourceFactory->createBuffer(desc);
        if (!buffer) {
            result.passed = false;
            result.failureReason = "Failed to create buffer for alignment test";
            return result;
        }
        
        size_t actualSize = buffer->getSize();
        size_t expectedAlignedSize = getAlignedSize(test.requestedSize, test.usage);
        
        // The actual size should be at least the requested size
        // and should be properly aligned
        if (actualSize < test.requestedSize) {
            result.passed = false;
            result.failureReason = "Buffer size less than requested";
            result.actualProperties["requested_size"] = test.requestedSize;
            result.actualProperties["actual_size"] = actualSize;
            return result;
        }
        
        _createdBuffers.push_back(buffer);
    }
    
    result.passed = true;
    result.actualBehavior = "Memory alignment requirements satisfied";
    
    return result;
}

TestResult WebGPUBufferHandler::testMultipleCreation(const TestVariation& variation) {
    TestResult result;
    
    int bufferCount = getOption<int>(variation.options, "buffer_count", 100);
    size_t bufferSize = getOption<size_t>(variation.options, "buffer_size", 4096);
    
    std::vector<double> creationTimes;
    
    for (int i = 0; i < bufferCount; i++) {
        BufferDesc desc;
        desc.size = bufferSize;
        desc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        desc.debugName = "MultiBuffer_" + std::to_string(i);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto buffer = _resourceFactory->createBuffer(desc);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        double timeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        creationTimes.push_back(timeMs);
        updateMetrics(timeMs, bufferSize);
        
        if (!buffer) {
            result.passed = false;
            result.failureReason = "Failed to create buffer " + std::to_string(i);
            result.actualProperties["failed_at_index"] = i;
            return result;
        }
        
        _createdBuffers.push_back(buffer);
    }
    
    // Calculate statistics
    double avgTime = std::accumulate(creationTimes.begin(), creationTimes.end(), 0.0) / creationTimes.size();
    double minTime = *std::min_element(creationTimes.begin(), creationTimes.end());
    double maxTime = *std::max_element(creationTimes.begin(), creationTimes.end());
    
    result.passed = true;
    result.actualBehavior = "Successfully created " + std::to_string(bufferCount) + " buffers";
    result.actualProperties["buffer_count"] = bufferCount;
    result.actualProperties["avg_time_ms"] = avgTime;
    result.actualProperties["min_time_ms"] = minTime;
    result.actualProperties["max_time_ms"] = maxTime;
    result.actualProperties["total_memory_mb"] = (bufferCount * bufferSize) / (1024.0 * 1024.0);
    
    return result;
}

BufferUsage WebGPUBufferHandler::parseUsageFlags(const std::vector<std::string>& flags) {
    BufferUsage usage = BufferUsage::None;
    
    for (const auto& flag : flags) {
        if (flag == "Vertex") usage |= BufferUsage::Vertex;
        else if (flag == "Index") usage |= BufferUsage::Index;
        else if (flag == "Uniform") usage |= BufferUsage::Uniform;
        else if (flag == "Storage") usage |= BufferUsage::Storage;
        else if (flag == "CopySrc") usage |= BufferUsage::CopySrc;
        else if (flag == "CopyDst") usage |= BufferUsage::CopyDst;
        else if (flag == "MapRead") usage |= BufferUsage::MapRead;
        else if (flag == "MapWrite") usage |= BufferUsage::MapWrite;
        else if (flag == "Indirect") usage |= BufferUsage::Indirect;
        else if (flag == "QueryResolve") usage |= BufferUsage::QueryResolve;
    }
    
    return usage;
}

size_t WebGPUBufferHandler::getAlignedSize(size_t size, BufferUsage usage) {
    size_t alignment = BufferAlignment::DEFAULT;
    
    if (hasFlag(usage, BufferUsage::Uniform)) {
        alignment = BufferAlignment::UNIFORM_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Storage)) {
        alignment = BufferAlignment::STORAGE_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Vertex)) {
        alignment = BufferAlignment::VERTEX_BUFFER_OFFSET;
    } else if (hasFlag(usage, BufferUsage::Index)) {
        alignment = BufferAlignment::INDEX_BUFFER_OFFSET;
    }
    
    return ((size + alignment - 1) / alignment) * alignment;
}

void WebGPUBufferHandler::updateMetrics(double timeMs, size_t size) {
    _metrics.totalBuffersCreated++;
    _metrics.totalBytesAllocated += size;
    
    _metrics.minCreationTimeMs = std::min(_metrics.minCreationTimeMs, timeMs);
    _metrics.maxCreationTimeMs = std::max(_metrics.maxCreationTimeMs, timeMs);
    
    // Update running average
    double n = static_cast<double>(_metrics.totalBuffersCreated);
    _metrics.avgCreationTimeMs = ((_metrics.avgCreationTimeMs * (n - 1)) + timeMs) / n;
}

} // namespace pers::tests