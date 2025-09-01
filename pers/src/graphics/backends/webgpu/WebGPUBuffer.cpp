#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoOrDie.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

static WGPUBufferUsage convertBufferUsage(BufferUsage usage) {
    WGPUBufferUsage flags = 0;
    uint32_t usageFlags = static_cast<uint32_t>(usage);
    
    // Map our BufferUsage enum values to WebGPU's values
    // Our enum: MapRead=1<<0, MapWrite=1<<1, CopySrc=1<<2, CopyDst=1<<3, 
    //           Index=1<<4, Vertex=1<<5, Uniform=1<<6, Storage=1<<7
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

WebGPUBuffer::WebGPUBuffer(const BufferDesc& desc, WGPUDevice device) 
    : _size(desc.size)
    , _usage(desc.usage)
    , _debugName(desc.debugName.empty() ? "Buffer" : desc.debugName)
    , _buffer(nullptr)
    , _mappedData(nullptr) {
    
    if (!device || _size == 0) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUBuffer",
            "Invalid parameters for buffer creation", PERS_SOURCE_LOC);
        return;
    }
    
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = _size;
    bufferDesc.usage = convertBufferUsage(_usage);
    bufferDesc.mappedAtCreation = desc.mappedAtCreation;
    bufferDesc.label = WGPUStringView{_debugName.data(), _debugName.length()};
    
    _buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    
    if (!_buffer) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUBuffer",
            PERS_SOURCE_LOC, "Failed to create buffer '%s' of size %llu", 
            _debugName.c_str(), _size);
    } else {
        Logger::Instance().LogFormat(LogLevel::Debug, "WebGPUBuffer",
            PERS_SOURCE_LOC, "Created buffer '%s' of size %llu", 
            _debugName.c_str(), _size);
        
        // If mapped at creation, get the mapped pointer
        if (desc.mappedAtCreation) {
            _mappedData = wgpuBufferGetMappedRange(_buffer, 0, _size);
        }
    }
}

WebGPUBuffer::~WebGPUBuffer() {
    if (_buffer) {
        // Unmap if still mapped
        if (_mappedData) {
            wgpuBufferUnmap(_buffer);
            _mappedData = nullptr;
        }
        wgpuBufferRelease(_buffer);
    }
}

uint64_t WebGPUBuffer::getSize() const {
    return _size;
}

BufferUsage WebGPUBuffer::getUsage() const {
    return _usage;
}

void* WebGPUBuffer::map(uint64_t offset, uint64_t size) {
    if (!_buffer) return nullptr;
    if (_mappedData) return static_cast<uint8_t*>(_mappedData) + offset;
    
    // Determine size to map
    uint64_t mapSize = size > 0 ? size : (_size - offset);
    
    // WebGPU async map - for now we use sync wrapper
    TodoOrDie::Log("WebGPUBuffer::map",
                   "Implement proper async mapping with callbacks", PERS_SOURCE_LOC);
    
    // For now, return nullptr as async mapping needs proper implementation
    return nullptr;
}

void WebGPUBuffer::unmap() {
    if (!_buffer || !_mappedData) return;
    
    wgpuBufferUnmap(_buffer);
    _mappedData = nullptr;
}

NativeBufferHandle WebGPUBuffer::getNativeBufferHandle() const {
    return NativeBufferHandle{_buffer};
}

WGPUBuffer WebGPUBuffer::getNativeHandle() const {
    return _buffer;
}

} // namespace webgpu
} // namespace pers