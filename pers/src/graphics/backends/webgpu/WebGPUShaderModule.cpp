#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoOrDie.h"
#include <webgpu/webgpu.h>
#include <string>

namespace pers {
namespace webgpu {

// Auto-detect shader stage from WGSL code
static ShaderStage detectShaderStage(const std::string& code) {
    if (code.find("@vertex") != std::string::npos) {
        return ShaderStage::Vertex;
    }
    if (code.find("@fragment") != std::string::npos) {
        return ShaderStage::Fragment;
    }
    if (code.find("@compute") != std::string::npos) {
        return ShaderStage::Compute;
    }
    return ShaderStage::None;
}

class WebGPUShaderModule::Impl {
public:
    Impl(const ShaderModuleDesc& desc) 
        : _stage(desc.stage)
        , _entryPoint(desc.entryPoint)
        , _debugName(desc.debugName)
        , _shaderModule(nullptr) {
        
        // Auto-detect stage if not specified
        if (_stage == ShaderStage::None) {
            _stage = detectShaderStage(desc.code);
            if (_stage == ShaderStage::None) {
                Logger::Instance().Log(LogLevel::Error, "WebGPUShaderModule",
                    "Failed to detect shader stage from code", PERS_SOURCE_LOC);
                return;
            }
        }
        
        // Set debug name if not provided
        if (_debugName.empty()) {
            switch (_stage) {
                case ShaderStage::Vertex:
                    _debugName = "VertexShader";
                    break;
                case ShaderStage::Fragment:
                    _debugName = "FragmentShader";
                    break;
                case ShaderStage::Compute:
                    _debugName = "ComputeShader";
                    break;
                default:
                    _debugName = "UnknownShader";
            }
        }
        
        // Store code for later compilation
        _code = desc.code;
        
        // Actual WebGPU shader module creation will happen when device is available
        TodoOrDie::Log("WebGPUShaderModule::Impl - Create WGPUShaderModule from code");
    }
    
    ~Impl() {
        if (_shaderModule) {
            wgpuShaderModuleRelease(_shaderModule);
        }
    }
    
    void createShaderModule(WGPUDevice device) {
        if (_shaderModule || !device) {
            return;
        }
        
        WGPUShaderModuleWGSLDescriptor wgslDesc = {};
        wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgslDesc.code = _code.c_str();
        
        WGPUShaderModuleDescriptor desc = {};
        desc.nextInChain = &wgslDesc.chain;
        desc.label = _debugName.c_str();
        
        _shaderModule = wgpuDeviceCreateShaderModule(device, &desc);
        
        if (!_shaderModule) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUShaderModule", 
                PERS_SOURCE_LOC, "Failed to create shader module: %s", _debugName.c_str());
        } else {
            Logger::Instance().LogFormat(LogLevel::Info, "WebGPUShaderModule",
                PERS_SOURCE_LOC, "Created shader module: %s", _debugName.c_str());
        }
    }
    
    ShaderStage _stage;
    std::string _entryPoint;
    std::string _debugName;
    std::string _code;
    WGPUShaderModule _shaderModule;
};

WebGPUShaderModule::WebGPUShaderModule(const ShaderModuleDesc& desc)
    : _impl(std::make_unique<Impl>(desc)) {
}

WebGPUShaderModule::~WebGPUShaderModule() = default;

ShaderStage WebGPUShaderModule::getStage() const {
    return _impl->_stage;
}

const std::string& WebGPUShaderModule::getEntryPoint() const {
    return _impl->_entryPoint;
}

const std::string& WebGPUShaderModule::getDebugName() const {
    return _impl->_debugName;
}

bool WebGPUShaderModule::isValid() const {
    return _impl->_shaderModule != nullptr;
}

void* WebGPUShaderModule::getNativeHandle() const {
    return _impl->_shaderModule;
}

void WebGPUShaderModule::createShaderModule(void* device) {
    _impl->createShaderModule(static_cast<WGPUDevice>(device));
}

} // namespace webgpu
} // namespace pers