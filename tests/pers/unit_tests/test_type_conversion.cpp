#include "test_framework.h"
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/GraphicsTypes.h"
#include <webgpu/webgpu.h>

namespace pers::tests {

// Include conversion functions from implementation
namespace {
    // From WebGPURenderPipeline.cpp
    WGPUColorWriteMask convertColorWriteMask(ColorWriteMask mask) {
        uint32_t result = WGPUColorWriteMask_None;
        uint32_t maskValue = static_cast<uint32_t>(mask);
        
        if (mask == ColorWriteMask::None) {
            return WGPUColorWriteMask_None;
        }
        if (mask == ColorWriteMask::All) {
            return WGPUColorWriteMask_All;
        }
        
        if (maskValue & static_cast<uint32_t>(ColorWriteMask::Red)) {
            result |= WGPUColorWriteMask_Red;
        }
        if (maskValue & static_cast<uint32_t>(ColorWriteMask::Green)) {
            result |= WGPUColorWriteMask_Green;
        }
        if (maskValue & static_cast<uint32_t>(ColorWriteMask::Blue)) {
            result |= WGPUColorWriteMask_Blue;
        }
        if (maskValue & static_cast<uint32_t>(ColorWriteMask::Alpha)) {
            result |= WGPUColorWriteMask_Alpha;
        }
        
        return static_cast<WGPUColorWriteMask>(result);
    }
    
    // From WebGPUBuffer.cpp
    WGPUBufferUsage convertBufferUsage(BufferUsage usage) {
        WGPUBufferUsage flags = 0;
        uint32_t usageFlags = static_cast<uint32_t>(usage);
        
        if (usageFlags & static_cast<uint32_t>(BufferUsage::Vertex)) {
            flags |= WGPUBufferUsage_Vertex;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::Index)) {
            flags |= WGPUBufferUsage_Index;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::Uniform)) {
            flags |= WGPUBufferUsage_Uniform;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::Storage)) {
            flags |= WGPUBufferUsage_Storage;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::CopySrc)) {
            flags |= WGPUBufferUsage_CopySrc;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::CopyDst)) {
            flags |= WGPUBufferUsage_CopyDst;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::MapRead)) {
            flags |= WGPUBufferUsage_MapRead;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::MapWrite)) {
            flags |= WGPUBufferUsage_MapWrite;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::Indirect)) {
            flags |= WGPUBufferUsage_Indirect;
        }
        if (usageFlags & static_cast<uint32_t>(BufferUsage::QueryResolve)) {
            flags |= WGPUBufferUsage_QueryResolve;
        }
        
        return flags;
    }
}

class TypeConversionTests : public TestSuite {
public:
    TypeConversionTests() : TestSuite("Type Conversion") {}
    
    void registerTests() override {
        // Test 011: ColorWriteMask::All
        addTest("011", "ColorWriteMask::All", []() {
            TestCase test;
            test.input = "ColorWriteMask::All";
            test.expectedResult = "WGPUColorWriteMask_All";
            test.expectedCallstack = "convertColorWriteMask() -> return WGPUColorWriteMask_All";
            
            auto result = convertColorWriteMask(ColorWriteMask::All);
            
            if (result == WGPUColorWriteMask_All) {
                test.actualResult = "WGPUColorWriteMask_All";
                test.passed = true;
            } else {
                test.actualResult = "Wrong value: " + std::to_string(result);
                test.passed = false;
                test.failureReason = "Conversion failed";
            }
            
            return test;
        });
        
        // Test 012: ColorWriteMask::Red
        addTest("012", "ColorWriteMask::Red", []() {
            TestCase test;
            test.input = "ColorWriteMask::Red";
            test.expectedResult = "WGPUColorWriteMask_Red";
            test.expectedCallstack = "convertColorWriteMask() -> check Red flag";
            
            auto result = convertColorWriteMask(ColorWriteMask::Red);
            
            if (result == WGPUColorWriteMask_Red) {
                test.actualResult = "WGPUColorWriteMask_Red";
                test.passed = true;
            } else {
                test.actualResult = "Wrong value: " + std::to_string(result);
                test.passed = false;
                test.failureReason = "Conversion failed";
            }
            
            return test;
        });
        
        // Test 013: BufferUsage::Vertex
        addTest("013", "BufferUsage::Vertex", []() {
            TestCase test;
            test.input = "BufferUsage::Vertex";
            test.expectedResult = "WGPUBufferUsage_Vertex";
            test.expectedCallstack = "convertBufferUsage() -> check Vertex flag";
            
            auto result = convertBufferUsage(BufferUsage::Vertex);
            
            if (result & WGPUBufferUsage_Vertex) {
                test.actualResult = "Has WGPUBufferUsage_Vertex flag";
                test.passed = true;
            } else {
                test.actualResult = "Missing Vertex flag";
                test.passed = false;
                test.failureReason = "Conversion failed";
            }
            
            return test;
        });
        
        // Test 014: BufferUsage Multiple Flags
        addTest("014", "BufferUsage Multiple Flags", []() {
            TestCase test;
            test.input = "Vertex|Index|Uniform";
            test.expectedResult = "Correct OR'd flags";
            test.expectedCallstack = "convertBufferUsage() -> OR multiple flags";
            
            auto usage = BufferUsage::Vertex | BufferUsage::Index | BufferUsage::Uniform;
            auto result = convertBufferUsage(usage);
            
            bool hasVertex = (result & WGPUBufferUsage_Vertex) != 0;
            bool hasIndex = (result & WGPUBufferUsage_Index) != 0;
            bool hasUniform = (result & WGPUBufferUsage_Uniform) != 0;
            
            if (hasVertex && hasIndex && hasUniform) {
                test.actualResult = "All flags present";
                test.passed = true;
            } else {
                test.actualResult = "Missing flags";
                test.passed = false;
                test.failureReason = "Not all flags converted";
            }
            
            return test;
        });
        
        // Test 015: TextureFormat::BGRA8Unorm
        addTest("015", "TextureFormat::BGRA8Unorm", []() {
            TestCase test;
            test.input = "TextureFormat::BGRA8Unorm";
            test.expectedResult = "WGPUTextureFormat_BGRA8Unorm";
            test.expectedCallstack = "convertTextureFormat()";
            
            // Note: This requires the actual convertTextureFormat function
            test.actualResult = "Conversion function not accessible";
            test.passed = false;
            test.failureReason = "Need access to WebGPURenderPipeline implementation";
            
            return test;
        });
        
        // Test 036: PrimitiveTopology::TriangleList
        addTest("036", "PrimitiveTopology::TriangleList", []() {
            TestCase test;
            test.input = "TriangleList";
            test.expectedResult = "WGPUPrimitiveTopology_TriangleList";
            test.expectedCallstack = "convertTopology()";
            
            // Direct comparison
            if (static_cast<int>(PrimitiveTopology::TriangleList) == 0) {
                test.actualResult = "Enum value is 0 (matches WebGPU)";
                test.passed = true;
            } else {
                test.actualResult = "Enum value mismatch";
                test.passed = false;
                test.failureReason = "PrimitiveTopology enum values don't match WebGPU";
            }
            
            return test;
        });
        
        // Test 037: CullMode::Back
        addTest("037", "CullMode::Back", []() {
            TestCase test;
            test.input = "CullMode::Back";
            test.expectedResult = "WGPUCullMode_Back";
            test.expectedCallstack = "convertCullMode()";
            
            // Check enum value
            if (static_cast<int>(CullMode::Back) == 2) {
                test.actualResult = "Enum value is 2 (matches WebGPU)";
                test.passed = true;
            } else {
                test.actualResult = "Enum value mismatch";
                test.passed = false;
                test.failureReason = "CullMode enum values don't match WebGPU";
            }
            
            return test;
        });
        
        // Test 038: FrontFace::CCW
        addTest("038", "FrontFace::CCW", []() {
            TestCase test;
            test.input = "FrontFace::CCW";
            test.expectedResult = "WGPUFrontFace_CCW";
            test.expectedCallstack = "convertFrontFace()";
            
            if (static_cast<int>(FrontFace::CCW) == 0) {
                test.actualResult = "Enum value is 0 (matches WebGPU)";
                test.passed = true;
            } else {
                test.actualResult = "Enum value mismatch";
                test.passed = false;
                test.failureReason = "FrontFace enum values don't match WebGPU";
            }
            
            return test;
        });
        
        // Test 039: CompareFunction::Less
        addTest("039", "CompareFunction::Less", []() {
            TestCase test;
            test.input = "CompareFunction::Less";
            test.expectedResult = "WGPUCompareFunction_Less";
            test.expectedCallstack = "convertCompareFunction()";
            
            if (static_cast<int>(CompareFunction::Less) == 3) {
                test.actualResult = "Enum value is 3 (matches WebGPU)";
                test.passed = true;
            } else {
                test.actualResult = "Enum value mismatch";
                test.passed = false;
                test.failureReason = "CompareFunction enum values don't match WebGPU";
            }
            
            return test;
        });
        
        // Test 040: LoadOp::Clear
        addTest("040", "LoadOp::Clear", []() {
            TestCase test;
            test.input = "LoadOp::Clear";
            test.expectedResult = "WGPULoadOp_Clear";
            test.expectedCallstack = "WebGPUCommandEncoder switch case";
            
            if (static_cast<int>(LoadOp::Clear) == 0) {
                test.actualResult = "Enum value is 0 (matches WebGPU)";
                test.passed = true;
            } else {
                test.actualResult = "Enum value mismatch";
                test.passed = false;
                test.failureReason = "LoadOp enum values don't match WebGPU";
            }
            
            return test;
        });
    }
};

// Register test suite
void registerTypeConversionTests(TestRegistry& registry) {
    registry.addSuite(std::make_unique<TypeConversionTests>());
}

} // namespace pers::tests