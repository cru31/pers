#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/ILogicalDevice.h>

namespace pers::tests {

class DeviceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _adapter;
    
    bool initializeAdapter() {
        if (_adapter) {
            return true;
        }
        
        // Create instance
        if (!_instance) {
            InstanceDesc instanceDesc;
            instanceDesc.applicationName = "Device Test";
            instanceDesc.enableValidation = false;
            _instance = _factory->createInstance(instanceDesc);
            if (!_instance) return false;
        }
        
        // Get adapter
        PhysicalDeviceOptions options;
        _adapter = _instance->requestPhysicalDevice(options);
        return _adapter != nullptr;
    }
    
public:
    DeviceCreationHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Device Creation";
    }
    
    TestResult execute(const TestVariation& variation) override {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Initialize adapter if needed
        if (!initializeAdapter()) {
            result.passed = false;
            result.actualBehavior = "Adapter initialization failed";
            result.failureReason = "Failed to get adapter for device creation";
            result.actualProperties["adapterInitialized"] = false;
            return result;
        }
        
        // Extract options
        bool enableValidation = getOption<bool>(variation.options, "enableValidation", false);
        std::string debugName = getOption<std::string>(variation.options, "debugName", "");
        
        // Handle special case
        if (debugName == "[LONG_STRING_1024]") {
            debugName = std::string(1024, 'A');
        }
        
        // Create device
        LogicalDeviceDesc desc;
        desc.enableValidation = enableValidation;
        desc.debugName = debugName;
        
        // Handle required features
        if (variation.options.count("requiredFeatures")) {
            try {
                auto features = std::any_cast<std::vector<std::string>>(variation.options.at("requiredFeatures"));
                for (const auto& featureStr : features) {
                    // Convert string to DeviceFeature enum
                    if (featureStr == "timestamp-query") {
                        desc.requiredFeatures.push_back(DeviceFeature::TimestampQuery);
                    } else if (featureStr == "texture-compression-bc") {
                        desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionBC);
                    } else if (featureStr == "texture-compression-etc2") {
                        desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionETC2);
                    } else if (featureStr == "texture-compression-astc") {
                        desc.requiredFeatures.push_back(DeviceFeature::TextureCompressionASTC);
                    } else if (featureStr == "depth-clip-control") {
                        desc.requiredFeatures.push_back(DeviceFeature::DepthClipControl);
                    } else if (featureStr == "depth32float-stencil8") {
                        desc.requiredFeatures.push_back(DeviceFeature::Depth32FloatStencil8);
                    } else if (featureStr == "pipeline-statistics-query") {
                        desc.requiredFeatures.push_back(DeviceFeature::PipelineStatisticsQuery);
                    } else if (featureStr == "indirect-first-instance") {
                        desc.requiredFeatures.push_back(DeviceFeature::IndirectFirstInstance);
                    } else if (featureStr == "shader-f16") {
                        desc.requiredFeatures.push_back(DeviceFeature::ShaderF16);
                    } else if (featureStr == "rg11b10ufloat-renderable") {
                        desc.requiredFeatures.push_back(DeviceFeature::RG11B10UfloatRenderable);
                    } else if (featureStr == "bgra8unorm-storage") {
                        desc.requiredFeatures.push_back(DeviceFeature::BGRA8UnormStorage);
                    } else if (featureStr == "float32-filterable") {
                        desc.requiredFeatures.push_back(DeviceFeature::Float32Filterable);
                    }
                }
            } catch (...) {
                // If parsing as vector fails, it might be empty
            }
        }
        
        // Handle required limits  
        if (variation.options.count("requiredLimits")) {
            try {
                auto limitsMap = std::any_cast<std::unordered_map<std::string, std::any>>(
                    variation.options.at("requiredLimits"));
                
                auto limits = std::make_shared<DeviceLimits>();
                
                // Parse each limit from the nested map
                for (const auto& [key, value] : limitsMap) {
                    if (key == "maxBindGroups") {
                        try {
                            limits->maxBindGroups = std::any_cast<int>(value);
                        } catch (...) {
                            // Try size_t if int fails
                            try {
                                limits->maxBindGroups = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxStorageBufferBindingSize") {
                        try {
                            limits->maxStorageBufferBindingSize = std::any_cast<int>(value);
                        } catch (...) {
                            // Try size_t if int fails
                            try {
                                limits->maxStorageBufferBindingSize = std::any_cast<size_t>(value);
                            } catch (...) {}
                        }
                    } else if (key == "maxBufferSize") {
                        // Note: DeviceLimits doesn't have maxBufferSize field
                        // This may need to be handled differently or ignored
                    } else if (key == "maxTextureDimension2D") {
                        try {
                            limits->maxTextureDimension2D = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxTextureDimension2D = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxTextureDimension1D") {
                        try {
                            limits->maxTextureDimension1D = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxTextureDimension1D = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxTextureDimension3D") {
                        try {
                            limits->maxTextureDimension3D = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxTextureDimension3D = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxTextureArrayLayers") {
                        try {
                            limits->maxTextureArrayLayers = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxTextureArrayLayers = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxBindingsPerBindGroup") {
                        try {
                            limits->maxBindingsPerBindGroup = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxBindingsPerBindGroup = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxDynamicUniformBuffersPerPipelineLayout") {
                        try {
                            limits->maxDynamicUniformBuffersPerPipelineLayout = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxDynamicUniformBuffersPerPipelineLayout = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxDynamicStorageBuffersPerPipelineLayout") {
                        try {
                            limits->maxDynamicStorageBuffersPerPipelineLayout = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxDynamicStorageBuffersPerPipelineLayout = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxSampledTexturesPerShaderStage") {
                        try {
                            limits->maxSampledTexturesPerShaderStage = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxSampledTexturesPerShaderStage = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxSamplersPerShaderStage") {
                        try {
                            limits->maxSamplersPerShaderStage = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxSamplersPerShaderStage = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxStorageBuffersPerShaderStage") {
                        try {
                            limits->maxStorageBuffersPerShaderStage = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxStorageBuffersPerShaderStage = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxStorageTexturesPerShaderStage") {
                        try {
                            limits->maxStorageTexturesPerShaderStage = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxStorageTexturesPerShaderStage = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxUniformBuffersPerShaderStage") {
                        try {
                            limits->maxUniformBuffersPerShaderStage = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxUniformBuffersPerShaderStage = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxUniformBufferBindingSize") {
                        try {
                            limits->maxUniformBufferBindingSize = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxUniformBufferBindingSize = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxVertexBuffers") {
                        try {
                            limits->maxVertexBuffers = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxVertexBuffers = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxVertexAttributes") {
                        try {
                            limits->maxVertexAttributes = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxVertexAttributes = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxVertexBufferArrayStride") {
                        try {
                            limits->maxVertexBufferArrayStride = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxVertexBufferArrayStride = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxInterStageShaderVariables") {
                        try {
                            limits->maxInterStageShaderVariables = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxInterStageShaderVariables = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeWorkgroupStorageSize") {
                        try {
                            limits->maxComputeWorkgroupStorageSize = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeWorkgroupStorageSize = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeInvocationsPerWorkgroup") {
                        try {
                            limits->maxComputeInvocationsPerWorkgroup = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeInvocationsPerWorkgroup = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeWorkgroupSizeX") {
                        try {
                            limits->maxComputeWorkgroupSizeX = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeWorkgroupSizeX = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeWorkgroupSizeY") {
                        try {
                            limits->maxComputeWorkgroupSizeY = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeWorkgroupSizeY = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeWorkgroupSizeZ") {
                        try {
                            limits->maxComputeWorkgroupSizeZ = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeWorkgroupSizeZ = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    } else if (key == "maxComputeWorkgroupsPerDimension") {
                        try {
                            limits->maxComputeWorkgroupsPerDimension = std::any_cast<int>(value);
                        } catch (...) {
                            try {
                                limits->maxComputeWorkgroupsPerDimension = static_cast<uint32_t>(std::any_cast<size_t>(value));
                            } catch (...) {}
                        }
                    }
                }
                
                desc.requiredLimits = limits;
            } catch (...) {
                // If it's not a map, it might be an empty object
            }
        }
        
        auto device = _adapter->createLogicalDevice(desc);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Check result
        bool deviceCreated = (device != nullptr);
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        
        // Populate actual properties - include input parameters for web viewer
        result.actualProperties["deviceCreated"] = deviceCreated;
        result.actualProperties["validationEnabled"] = enableValidation;
        result.actualProperties["debugName"] = debugName;
        
        // Store requiredFeatures as input parameter
        if (!desc.requiredFeatures.empty()) {
            std::vector<std::string> featureStrings;
            for (const auto& feature : desc.requiredFeatures) {
                switch(feature) {
                    case DeviceFeature::TimestampQuery:
                        featureStrings.push_back("timestamp-query");
                        break;
                    case DeviceFeature::TextureCompressionBC:
                        featureStrings.push_back("texture-compression-bc");
                        break;
                    case DeviceFeature::TextureCompressionETC2:
                        featureStrings.push_back("texture-compression-etc2");
                        break;
                    case DeviceFeature::TextureCompressionASTC:
                        featureStrings.push_back("texture-compression-astc");
                        break;
                    case DeviceFeature::DepthClipControl:
                        featureStrings.push_back("depth-clip-control");
                        break;
                    case DeviceFeature::Depth32FloatStencil8:
                        featureStrings.push_back("depth32float-stencil8");
                        break;
                    case DeviceFeature::PipelineStatisticsQuery:
                        featureStrings.push_back("pipeline-statistics-query");
                        break;
                    case DeviceFeature::IndirectFirstInstance:
                        featureStrings.push_back("indirect-first-instance");
                        break;
                    case DeviceFeature::ShaderF16:
                        featureStrings.push_back("shader-f16");
                        break;
                    case DeviceFeature::RG11B10UfloatRenderable:
                        featureStrings.push_back("rg11b10ufloat-renderable");
                        break;
                    case DeviceFeature::BGRA8UnormStorage:
                        featureStrings.push_back("bgra8unorm-storage");
                        break;
                    case DeviceFeature::Float32Filterable:
                        featureStrings.push_back("float32-filterable");
                        break;
                }
            }
            result.actualProperties["requiredFeatures"] = featureStrings;
        }
        
        // Store requiredLimits as input parameter - use original input values
        if (variation.options.count("requiredLimits")) {
            try {
                auto inputLimitsMap = std::any_cast<std::unordered_map<std::string, std::any>>(
                    variation.options.at("requiredLimits"));
                result.actualProperties["requiredLimits"] = inputLimitsMap;
            } catch (...) {
                // Fallback: shouldn't happen but just in case
                result.actualProperties["requiredLimits"] = std::unordered_map<std::string, std::any>();
            }
        }
        
        if (device) {
            result.actualProperties["returnValue"] = std::string("not_null");
            
            // Check if resource factory is available
            auto resourceFactory = device->getResourceFactory();
            result.actualProperties["resourceFactoryAvailable"] = (resourceFactory != nullptr);
            
            // Check if queue is available
            auto queue = device->getQueue();
            result.actualProperties["queueAvailable"] = (queue != nullptr);
        } else {
            result.actualProperties["returnValue"] = std::string("nullptr");
        }
        
        // Check expected properties
        if (variation.expectedBehavior.properties.count("validationEnabled")) {
            bool expectedValidation = std::any_cast<bool>(
                variation.expectedBehavior.properties.at("validationEnabled"));
            if (deviceCreated && expectedValidation != enableValidation) {
                result.passed = false;
                result.failureReason = "Validation state mismatch";
                return result;
            }
        }
        
        // Determine pass/fail
        result.passed = (deviceCreated == expectedNotNull);
        
        if (result.passed) {
            result.actualBehavior = deviceCreated 
                ? "Device created successfully"
                : "Device creation failed as expected";
        } else {
            result.actualBehavior = deviceCreated 
                ? "Device created unexpectedly"
                : "Failed to create device";
            if (result.failureReason.empty()) {
                result.failureReason = deviceCreated
                    ? "Expected nullptr but got valid device"
                    : "Expected valid device but got nullptr";
            }
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Device Creation", DeviceCreationHandler)

} // namespace pers::tests