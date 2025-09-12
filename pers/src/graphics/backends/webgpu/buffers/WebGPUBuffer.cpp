#include "pers/graphics/backends/webgpu/buffers/WebGPUBuffer.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <sstream>

namespace pers {

static uint32_t convertBufferUsage(BufferUsage usage) {
    uint32_t result = 0;
    
    uint32_t u = static_cast<uint32_t>(usage);
    
    if (u & static_cast<uint32_t>(BufferUsage::MapRead)) {
        result = result | WGPUBufferUsage_MapRead;
    }
    if (u & static_cast<uint32_t>(BufferUsage::MapWrite)) {
        result = result | WGPUBufferUsage_MapWrite;
    }
    if (u & static_cast<uint32_t>(BufferUsage::CopySrc)) {
        result = result | WGPUBufferUsage_CopySrc;
    }
    if (u & static_cast<uint32_t>(BufferUsage::CopyDst)) {
        result = result | WGPUBufferUsage_CopyDst;
    }
    if (u & static_cast<uint32_t>(BufferUsage::Index)) {
        result = result | WGPUBufferUsage_Index;
    }
    if (u & static_cast<uint32_t>(BufferUsage::Vertex)) {
        result = result | WGPUBufferUsage_Vertex;
    }
    if (u & static_cast<uint32_t>(BufferUsage::Uniform)) {
        result = result | WGPUBufferUsage_Uniform;
    }
    if (u & static_cast<uint32_t>(BufferUsage::Storage)) {
        result = result | WGPUBufferUsage_Storage;
    }
    if (u & static_cast<uint32_t>(BufferUsage::Indirect)) {
        result = result | WGPUBufferUsage_Indirect;
    }
    if (u & static_cast<uint32_t>(BufferUsage::QueryResolve)) {
        result = result | WGPUBufferUsage_QueryResolve;
    }
    
    return result;
}

WebGPUBuffer::WebGPUBuffer(WGPUDevice device, const BufferDesc& desc)
    : _device(device)
    , _buffer(nullptr)
    , _desc(desc)
    , _mappedData(nullptr) {
    
    if (!device) {
        LOG_ERROR("WebGPUBuffer", "Device is null");
        return;
    }
    
    if (!desc.isValid()) {
        LOG_ERROR("WebGPUBuffer", "Invalid buffer description");
        return;
    }
    
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.nextInChain = nullptr;
    WGPUStringView labelView = {_desc.debugName.c_str(), _desc.debugName.length()};
    bufferDesc.label = labelView;
    bufferDesc.usage = convertBufferUsage(_desc.usage);
    bufferDesc.size = _desc.size;
    bufferDesc.mappedAtCreation = _desc.mappedAtCreation;
    
    _buffer = wgpuDeviceCreateBuffer(_device, &bufferDesc);
    
    if (!_buffer) {
        LOG_ERROR("WebGPUBuffer", "Failed to create WebGPU buffer");
        return;
    }
    
    if (_desc.mappedAtCreation) {
        _mappedData = wgpuBufferGetMappedRange(_buffer, 0, _desc.size);
        if (!_mappedData) {
            LOG_ERROR("WebGPUBuffer", "Failed to get mapped range for buffer created with mappedAtCreation");
        }
    }
    
    std::stringstream ss;
    ss << "Created WebGPU buffer '" << _desc.debugName << "' size=" << _desc.size 
       << " usage=0x" << std::hex << bufferDesc.usage;
    LOG_DEBUG("WebGPUBuffer", ss.str().c_str());
}

WebGPUBuffer::~WebGPUBuffer() {
    if (_buffer) {
        wgpuBufferDestroy(_buffer);
        wgpuBufferRelease(_buffer);
        _buffer = nullptr;
    }
}

WebGPUBuffer::WebGPUBuffer(WebGPUBuffer&& other) noexcept
    : _device(other._device)
    , _buffer(other._buffer)
    , _desc(std::move(other._desc))
    , _mappedData(other._mappedData) {
    
    other._device = nullptr;
    other._buffer = nullptr;
    other._mappedData = nullptr;
}

WebGPUBuffer& WebGPUBuffer::operator=(WebGPUBuffer&& other) noexcept {
    if (this != &other) {
        if (_buffer) {
            if (_mappedData) {
                wgpuBufferUnmap(_buffer);
            }
            wgpuBufferDestroy(_buffer);
            wgpuBufferRelease(_buffer);
        }
        
        _device = other._device;
        _buffer = other._buffer;
        _desc = std::move(other._desc);
        _mappedData = other._mappedData;
        
        other._device = nullptr;
        other._buffer = nullptr;
        other._mappedData = nullptr;
    }
    return *this;
}

uint64_t WebGPUBuffer::getSize() const {
    return _desc.size;
}

BufferUsage WebGPUBuffer::getUsage() const {
    return _desc.usage;
}

const std::string& WebGPUBuffer::getDebugName() const {
    return _desc.debugName;
}

NativeBufferHandle WebGPUBuffer::getNativeHandle() const {
    return NativeBufferHandle::fromBackend(_buffer);
}

bool WebGPUBuffer::isValid() const {
    return _buffer != nullptr;
}

BufferState WebGPUBuffer::getState() const {
    if (!_buffer) {
        return BufferState::Uninitialized;
    }
    if (_mappedData) {
        return BufferState::Mapped;
    }
    return BufferState::Ready;
}

MemoryLocation WebGPUBuffer::getMemoryLocation() const {
    return _desc.memoryLocation;
}

AccessPattern WebGPUBuffer::getAccessPattern() const {
    return _desc.accessPattern;
}

void* WebGPUBuffer::getMappedDataAtCreation() {
    return _mappedData;
}

void WebGPUBuffer::unmapAtCreation() {
    if (_mappedData && _buffer) {
        wgpuBufferUnmap(_buffer);
        _mappedData = nullptr;
    }
}

} // namespace pers
