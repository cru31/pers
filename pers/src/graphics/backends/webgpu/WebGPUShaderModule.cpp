#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>

namespace pers {

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

WebGPUShaderModule::WebGPUShaderModule(const ShaderModuleDesc& desc) 
    : _stage(desc.stage)
    , _entryPoint(desc.entryPoint)
    , _debugName(desc.debugName)
    , _code(desc.code)
    , _shaderModule(nullptr) {
    
    // Auto-detect stage if not specified
    if (_stage == ShaderStage::None) {
        _stage = detectShaderStage(_code);
        if (_stage == ShaderStage::None) {
            LOG_ERROR("WebGPUShaderModule",
                "Failed to detect shader stage from code");
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
}

WebGPUShaderModule::~WebGPUShaderModule() {
    if (_shaderModule) {
        wgpuShaderModuleRelease(_shaderModule);
    }
}

ShaderStage WebGPUShaderModule::getStage() const {
    return _stage;
}

const std::string& WebGPUShaderModule::getEntryPoint() const {
    return _entryPoint;
}

const std::string& WebGPUShaderModule::getDebugName() const {
    return _debugName;
}

bool WebGPUShaderModule::isValid() const {
    return _shaderModule != nullptr;
}

WGPUShaderModule WebGPUShaderModule::getNativeHandle() const {
    return _shaderModule;
}

void WebGPUShaderModule::createShaderModule(WGPUDevice device) {
    if (_shaderModule || !device) {
        return;
    }
    
    WGPUShaderSourceWGSL wgslSource = {};
    wgslSource.chain.next = nullptr;
    wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSource.code = WGPUStringView{_code.data(), _code.length()};
    
    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = &wgslSource.chain;
    desc.label = WGPUStringView{_debugName.c_str(), _debugName.length()};
    
    _shaderModule = wgpuDeviceCreateShaderModule(device, &desc);
    
    if (!_shaderModule) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUShaderModule", 
            PERS_SOURCE_LOC, "Failed to create shader module: %s", _debugName.c_str());
    } else {
        Logger::Instance().LogFormat(LogLevel::Info, "WebGPUShaderModule",
            PERS_SOURCE_LOC, "Created shader module: %s", _debugName.c_str());
    }
}

} // namespace pers