#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

static WGPUBufferUsageFlags convertBufferUsage(BufferUsage usage) {
    WGPUBufferUsageFlags flags = 0;
    
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Vertex))
        flags |= WGPUBufferUsage_Vertex;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Index))
        flags |= WGPUBufferUsage_Index;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Uniform))
        flags |= WGPUBufferUsage_Uniform;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Storage))
        flags |= WGPUBufferUsage_Storage;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::CopySrc))
        flags |= WGPUBufferUsage_CopySrc;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::CopyDst))
        flags |= WGPUBufferUsage_CopyDst;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::MapRead))
        flags |= WGPUBufferUsage_MapRead;
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::MapWrite))
        flags |= WGPUBufferUsage_MapWrite;
    
    return flags;
}

class WebGPUBuffer::Impl {
public:
    Impl(const BufferDesc& desc, WGPUDevice device) 
        : _size(desc.size)
        , _usage(desc.usage)
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
        bufferDesc.label = "Buffer";
        
        _buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        
        if (!_buffer) {
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPUBuffer",
                PERS_SOURCE_LOC, "Failed to create buffer of size %llu", _size);
        } else {
            Logger::Instance().LogFormat(LogLevel::Debug, "WebGPUBuffer",
                PERS_SOURCE_LOC, "Created buffer of size %llu", _size);
            
            // If mapped at creation, get the mapped pointer
            if (desc.mappedAtCreation) {
                _mappedData = wgpuBufferGetMappedRange(_buffer, 0, _size);
            }
        }
    }
    
    ~Impl() {
        if (_buffer) {
            // Unmap if still mapped
            if (_mappedData) {
                wgpuBufferUnmap(_buffer);
                _mappedData = nullptr;
            }
            wgpuBufferRelease(_buffer);
        }
    }
    
    void* map(uint64_t offset, uint64_t size) {
        if (!_buffer) return nullptr;
        if (_mappedData) return static_cast<uint8_t*>(_mappedData) + offset;
        
        // Determine size to map
        uint64_t mapSize = size > 0 ? size : (_size - offset);
        
        // WebGPU async map - for now we use sync wrapper
        // TODO: Implement proper async mapping with callbacks
        
        // For now, return nullptr as async mapping needs proper implementation
        Logger::Instance().Log(LogLevel::Warning, "WebGPUBuffer",
            "Async buffer mapping not yet implemented", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    void unmap() {
        if (!_buffer || !_mappedData) return;
        
        wgpuBufferUnmap(_buffer);
        _mappedData = nullptr;
    }
    
    uint64_t _size;
    BufferUsage _usage;
    WGPUBuffer _buffer;
    void* _mappedData;
};

WebGPUBuffer::WebGPUBuffer(const BufferDesc& desc, void* device)
    : _impl(std::make_unique<Impl>(desc, static_cast<WGPUDevice>(device))) {
}

WebGPUBuffer::~WebGPUBuffer() = default;

uint64_t WebGPUBuffer::getSize() const {
    return _impl->_size;
}

BufferUsage WebGPUBuffer::getUsage() const {
    return _impl->_usage;
}

void* WebGPUBuffer::map(uint64_t offset, uint64_t size) {
    return _impl->map(offset, size);
}

void WebGPUBuffer::unmap() {
    _impl->unmap();
}

NativeBufferHandle WebGPUBuffer::getNativeBufferHandle() const {
    return NativeBufferHandle{_impl->_buffer};
}

void* WebGPUBuffer::getNativeHandle() const {
    return _impl->_buffer;
}

} // namespace webgpu
} // namespace pers