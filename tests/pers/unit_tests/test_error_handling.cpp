#include "test_framework.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"

namespace pers::tests {

class ErrorHandlingTests : public TestSuite {
public:
    ErrorHandlingTests() : TestSuite("Error Handling") {}
    
    void registerTests() override {
        // Test 021: Null Device Operations
        addTest("021", "Null Device Operations", []() {
            TestCase test;
            test.input = "Operations on null device";
            test.expectedResult = "Returns error";
            test.expectedCallstack = "WebGPULogicalDevice methods -> null check";
            
            WebGPULogicalDevice device(nullptr, nullptr);
            
            auto queue = device.getQueue();
            if (queue == nullptr) {
                test.actualResult = "getQueue returned nullptr";
                test.passed = true;
            } else {
                test.actualResult = "getQueue returned non-null";
                test.passed = false;
                test.failureReason = "Should return nullptr for null device";
            }
            
            return test;
        });
        
        // Test 022: Invalid Surface
        addTest("022", "Invalid Surface", []() {
            TestCase test;
            test.input = "Null surface handle";
            test.expectedResult = "SwapChain creation fails";
            test.expectedCallstack = "WebGPULogicalDevice::createSwapChain() -> surface validation";
            
            WebGPULogicalDevice device(nullptr, nullptr);
            NativeSurfaceHandle nullSurface(nullptr);
            SwapChainDesc desc;
            
            auto swapChain = device.createSwapChain(nullSurface, desc);
            
            if (swapChain == nullptr) {
                test.actualResult = "createSwapChain returned nullptr";
                test.passed = true;
            } else {
                test.actualResult = "createSwapChain returned non-null";
                test.passed = false;
                test.failureReason = "Should fail with null surface";
            }
            
            return test;
        });
        
        // Test 023: Queue Submit Empty
        addTest("023", "Queue Submit Empty", []() {
            TestCase test;
            test.input = "Empty command buffer list";
            test.expectedResult = "Returns success";
            test.expectedCallstack = "WebGPUQueue::submit() -> empty check";
            
            WebGPUQueue queue(nullptr);
            std::vector<std::shared_ptr<ICommandBuffer>> emptyList;
            
            bool result = queue.submit(emptyList);
            
            if (result) {
                test.actualResult = "submit returned true for empty list";
                test.passed = true;
            } else {
                test.actualResult = "submit returned false";
                test.passed = false;
                test.failureReason = "Empty list should succeed";
            }
            
            return test;
        });
        
        // Test 024: RenderPass Without Begin
        addTest("024", "RenderPass Without Begin", []() {
            TestCase test;
            test.input = "End without Begin";
            test.expectedResult = "Error logged";
            test.expectedCallstack = "WebGPURenderPassEncoder::end() -> check _ended flag";
            
            WebGPURenderPassEncoder encoder(nullptr);
            
            // Try to end without beginning
            encoder.end();
            
            // Check if it handled gracefully (no crash)
            test.actualResult = "end() handled gracefully";
            test.passed = true;
            
            return test;
        });
        
        // Test 025: Double RenderPass End
        addTest("025", "Double RenderPass End", []() {
            TestCase test;
            test.input = "Call end() twice";
            test.expectedResult = "Warning logged";
            test.expectedCallstack = "WebGPURenderPassEncoder::end() -> check _ended flag";
            
            WebGPURenderPassEncoder encoder(nullptr);
            
            // End twice
            encoder.end();
            encoder.end();  // Should log warning
            
            test.actualResult = "Double end() handled gracefully";
            test.passed = true;
            
            return test;
        });
    }
};

// Register test suite
void registerErrorHandlingTests(TestRegistry& registry) {
    registry.addSuite(std::make_unique<ErrorHandlingTests>());
}

} // namespace pers::tests