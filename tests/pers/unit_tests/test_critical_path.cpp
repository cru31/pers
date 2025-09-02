#include "test_framework.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/graphics/backends/webgpu/WebGPUPhysicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/backends/webgpu/WebGPUCommandEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPUSwapChain.h"
#include "pers/utils/Logger.h"

namespace pers::tests {

class CriticalPathTests : public TestSuite {
public:
    CriticalPathTests() : TestSuite("Critical Path") {
        _factory = std::make_shared<WebGPUBackendFactory>();
    }
    
    void registerTests() override {
        // Test 001: WebGPU Instance Creation
        addTest("001", "WebGPU Instance Creation", [this]() {
            TestCase test;
            test.input = "InstanceDesc{enableValidation=true}";
            test.expectedResult = "Valid instance created";
            test.expectedCallstack = "WebGPUBackendFactory::createInstance() -> WebGPUInstance::WebGPUInstance() -> wgpuCreateInstance()";
            
            InstanceDesc desc;
            desc.applicationName = "Unit Test";
            desc.enableValidation = true;
            desc.engineName = "Pers Graphics Engine";
            
            _instance = _factory->createInstance(desc);
            
            if (_instance) {
                test.actualResult = "Valid instance created";
                test.passed = true;
            } else {
                test.actualResult = "Null instance returned";
                test.passed = false;
                test.failureReason = "createInstance returned nullptr";
            }
            
            return test;
        });
        
        // Test 002: Adapter Enumeration
        addTest("002", "Adapter Enumeration", [this]() {
            TestCase test;
            test.input = "Valid instance";
            test.expectedResult = "At least 1 adapter found";
            test.expectedCallstack = "WebGPUInstance::enumerateAdapters() -> wgpuInstanceRequestAdapter()";
            
            if (!_instance) {
                test.actualResult = "No instance available";
                test.passed = false;
                test.failureReason = "Test 001 must pass first";
                return test;
            }
            
            auto adapters = _instance->enumerateAdapters();
            
            if (!adapters.empty()) {
                test.actualResult = std::to_string(adapters.size()) + " adapter(s) found";
                test.passed = true;
                _physicalDevice = adapters[0];
            } else {
                test.actualResult = "No adapters found";
                test.passed = false;
                test.failureReason = "enumerateAdapters returned empty vector";
            }
            
            return test;
        });
        
        // Test 003: Device Creation
        addTest("003", "Device Creation", [this]() {
            TestCase test;
            test.input = "Valid adapter";
            test.expectedResult = "Valid device created";
            test.expectedCallstack = "WebGPUPhysicalDevice::createLogicalDevice() -> wgpuAdapterRequestDevice()";
            
            if (!_physicalDevice) {
                test.actualResult = "No physical device available";
                test.passed = false;
                test.failureReason = "Test 002 must pass first";
                return test;
            }
            
            DeviceDesc deviceDesc;
            deviceDesc.label = "Test Device";
            
            _device = _physicalDevice->createLogicalDevice(deviceDesc);
            
            if (_device) {
                test.actualResult = "Valid device created";
                test.passed = true;
            } else {
                test.actualResult = "Null device returned";
                test.passed = false;
                test.failureReason = "createLogicalDevice returned nullptr";
            }
            
            return test;
        });
        
        // Test 004: Queue Creation
        addTest("004", "Queue Creation", [this]() {
            TestCase test;
            test.input = "Valid device";
            test.expectedResult = "Valid queue created";
            test.expectedCallstack = "WebGPULogicalDevice::createDefaultQueue() -> wgpuDeviceGetQueue()";
            
            if (!_device) {
                test.actualResult = "No device available";
                test.passed = false;
                test.failureReason = "Test 003 must pass first";
                return test;
            }
            
            auto queue = _device->getQueue();
            
            if (queue) {
                test.actualResult = "Valid queue created";
                test.passed = true;
                _queue = queue;
            } else {
                test.actualResult = "Null queue returned";
                test.passed = false;
                test.failureReason = "getQueue returned nullptr";
            }
            
            return test;
        });
        
        // Test 005: Command Encoder Creation
        addTest("005", "Command Encoder Creation", [this]() {
            TestCase test;
            test.input = "Valid device";
            test.expectedResult = "Valid encoder created";
            test.expectedCallstack = "WebGPULogicalDevice::createCommandEncoder() -> wgpuDeviceCreateCommandEncoder()";
            
            if (!_device) {
                test.actualResult = "No device available";
                test.passed = false;
                test.failureReason = "Test 003 must pass first";
                return test;
            }
            
            auto encoder = _device->createCommandEncoder();
            
            if (encoder) {
                test.actualResult = "Valid encoder created";
                test.passed = true;
            } else {
                test.actualResult = "Null encoder returned";
                test.passed = false;
                test.failureReason = "createCommandEncoder returned nullptr";
            }
            
            return test;
        });
        
        // Test 026: SwapChain Creation
        addTest("026", "SwapChain Creation", [this]() {
            TestCase test;
            test.input = "Valid surface + desc";
            test.expectedResult = "Valid swapchain";
            test.expectedCallstack = "WebGPUSwapChain::WebGPUSwapChain() -> wgpuDeviceCreateSwapChain()";
            
            if (!_device) {
                test.actualResult = "No device available";
                test.passed = false;
                test.failureReason = "Device creation must pass first";
                return test;
            }
            
            // Note: Surface creation requires window system integration
            // For unit test, we skip actual surface creation
            test.actualResult = "Skipped - requires window surface";
            test.passed = false;
            test.failureReason = "Window surface not available in unit test";
            
            return test;
        });
        
        // Test 028: Command Buffer Recording
        addTest("028", "Command Buffer Recording", [this]() {
            TestCase test;
            test.input = "Valid encoder";
            test.expectedResult = "Valid command buffer";
            test.expectedCallstack = "WebGPUCommandEncoder::finish() -> wgpuCommandEncoderFinish()";
            
            if (!_device) {
                test.actualResult = "No device available";
                test.passed = false;
                test.failureReason = "Device creation must pass first";
                return test;
            }
            
            auto encoder = _device->createCommandEncoder();
            if (!encoder) {
                test.actualResult = "Failed to create encoder";
                test.passed = false;
                test.failureReason = "createCommandEncoder failed";
                return test;
            }
            
            auto commandBuffer = encoder->finish();
            
            if (commandBuffer) {
                test.actualResult = "Valid command buffer created";
                test.passed = true;
            } else {
                test.actualResult = "Null command buffer";
                test.passed = false;
                test.failureReason = "finish() returned nullptr";
            }
            
            return test;
        });
        
        // Test 029: RenderPass Begin
        addTest("029", "RenderPass Begin", [this]() {
            TestCase test;
            test.input = "Valid RenderPassDesc";
            test.expectedResult = "RenderPass started";
            test.expectedCallstack = "WebGPUCommandEncoder::beginRenderPass() -> wgpuCommandEncoderBeginRenderPass()";
            
            if (!_device) {
                test.actualResult = "No device available";
                test.passed = false;
                test.failureReason = "Device creation must pass first";
                return test;
            }
            
            auto encoder = _device->createCommandEncoder();
            if (!encoder) {
                test.actualResult = "Failed to create encoder";
                test.passed = false;
                test.failureReason = "createCommandEncoder failed";
                return test;
            }
            
            // Create minimal render pass descriptor
            RenderPassDesc desc;
            desc.label = "Test RenderPass";
            
            // Note: Without valid texture views, this will likely fail
            // This is expected in unit test without full setup
            try {
                auto renderPass = encoder->beginRenderPass(desc);
                if (renderPass) {
                    test.actualResult = "RenderPass created (unexpected)";
                    test.passed = true;
                } else {
                    test.actualResult = "Null renderpass (expected without textures)";
                    test.passed = false;
                    test.failureReason = "Expected - no texture views available";
                }
            } catch (...) {
                test.actualResult = "Exception thrown (expected)";
                test.passed = false;
                test.failureReason = "Expected - no texture views available";
            }
            
            return test;
        });
    }
    
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _device;
    std::shared_ptr<IQueue> _queue;
};

// Register test suite
void registerCriticalPathTests(TestRegistry& registry) {
    registry.addSuite(std::make_unique<CriticalPathTests>());
}

} // namespace pers::tests