#include "pers/graphics/backends/webgpu/buffers/WebGPUMappableBuffer.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <sstream>
#include <cstring>

namespace pers {

WebGPUMappableBuffer::WebGPUMappableBuffer(WGPUDevice device, const BufferDesc& desc)
    : _impl(std::make_unique<WebGPUBuffer>(device, desc))
    , _mappedData(nullptr)
    , _isMapped(false)
    , _mappedRange{0, 0} {
    
    if (!_impl || !_impl->isValid()) {
        LOG_ERROR("WebGPUMappableBuffer", "Failed to create underlying WebGPU buffer");
        return;
    }
    
    // If mapped at creation, get the mapped data
    if (desc.mappedAtCreation) {
        _mappedData = _impl->getMappedDataAtCreation();
        if (_mappedData) {
            _isMapped = true;
            _mappedRange = {0, desc.size};
        }
    }
    
    std::stringstream ss;
    ss << "Created WebGPU mappable buffer '" << desc.debugName << "' size=" << desc.size;
    if (_isMapped) {
        ss << " (mapped at creation)";
    }
    LOG_DEBUG("WebGPUMappableBuffer", ss.str().c_str());
}

WebGPUMappableBuffer::~WebGPUMappableBuffer() {
    if (_isMapped.load() && _impl) {
        unmap();
    }
}

WebGPUMappableBuffer::WebGPUMappableBuffer(WebGPUMappableBuffer&& other) noexcept
    : _impl(std::move(other._impl))
    , _mappedData(other._mappedData)
    , _isMapped(other._isMapped.load())
    , _mappedRange(other._mappedRange) {
    
    other._mappedData = nullptr;
    other._isMapped = false;
    other._mappedRange = {0, 0};
}

WebGPUMappableBuffer& WebGPUMappableBuffer::operator=(WebGPUMappableBuffer&& other) noexcept {
    if (this != &other) {
        if (_isMapped && _impl) {
            unmap();
        }
        
        _impl = std::move(other._impl);
        _mappedData = other._mappedData;
        _isMapped = other._isMapped.load();
        _mappedRange = other._mappedRange;
        
        other._mappedData = nullptr;
        other._isMapped = false;
        other._mappedRange = {0, 0};
    }
    return *this;
}

void* WebGPUMappableBuffer::getMappedData() {
    return _mappedData;
}

const void* WebGPUMappableBuffer::getMappedData() const {
    return _mappedData;
}

struct MapAsyncContext {
    std::promise<MappedData> promise;
    WebGPUMappableBuffer* buffer;
    uint64_t offset;
    uint64_t size;
};

static void mapAsyncCallback(WGPUMapAsyncStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
    auto* context = static_cast<MapAsyncContext*>(userdata1);
    
    if (status == WGPUMapAsyncStatus_Success) {
        auto wgpuBuffer = context->buffer->getNativeHandle().as<WGPUBuffer>();
        void* data = wgpuBufferGetMappedRange(wgpuBuffer, context->offset, context->size);
        
        if (data) {
            context->buffer->_mappedData = data;
            context->buffer->_isMapped = true;
            context->buffer->_mappedRange = {context->offset, context->size};
            context->promise.set_value(MappedData{data, context->size, nullptr});
        } else {
            LOG_ERROR("WebGPUMappableBuffer", "Failed to get mapped range after successful map");
            context->promise.set_value(MappedData{nullptr, 0, nullptr});
        }
    } else {
        std::stringstream ss;
        ss << "Map async failed with status: " << status;
        LOG_ERROR("WebGPUMappableBuffer", ss.str().c_str());
        context->promise.set_value(MappedData{nullptr, 0, nullptr});
    }
    
    delete context;
}

std::future<MappedData> WebGPUMappableBuffer::mapAsync(MapMode mode, const BufferMapRange& range) {
    if (!_impl || !_impl->isValid()) {
        std::promise<MappedData> promise;
        promise.set_value(MappedData{nullptr, 0, nullptr});
        return promise.get_future();
    }
    
    if (_isMapped) {
        LOG_WARNING("WebGPUMappableBuffer", "Buffer is already mapped");
        std::promise<MappedData> promise;
        promise.set_value(MappedData{_mappedData, _mappedRange.size, nullptr});
        return promise.get_future();
    }
    
    auto wgpuBuffer = _impl->getNativeHandle().as<WGPUBuffer>();
    if (!wgpuBuffer) {
        std::promise<MappedData> promise;
        promise.set_value(MappedData{nullptr, 0, nullptr});
        return promise.get_future();
    }
    
    WGPUMapMode mapMode = WGPUMapMode_None;
    if (mode == MapMode::Read) {
        mapMode = WGPUMapMode_Read;
    } else if (mode == MapMode::Write) {
        mapMode = WGPUMapMode_Write;
    }
    
    uint64_t offset = range.offset;
    uint64_t size = range.size;
    
    if (size == BufferMapRange::WHOLE_BUFFER) {
        size = _impl->getSize() - offset;
    }
    
    auto* context = new MapAsyncContext();
    context->buffer = this;
    context->offset = offset;
    context->size = size;
    
    // Must get future before calling wgpuBufferMapAsync because the callback
    // might run synchronously with WGPUCallbackMode_AllowSpontaneous and delete context
    auto future = context->promise.get_future();
    
    WGPUBufferMapCallbackInfo callbackInfo{};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = mapAsyncCallback;
    callbackInfo.userdata1 = context;
    callbackInfo.userdata2 = nullptr;
    
    wgpuBufferMapAsync(wgpuBuffer, mapMode, offset, size, callbackInfo);
    
    return future;
}

void WebGPUMappableBuffer::unmap() {
    // Use atomic exchange to ensure unmap is only called once
    bool wasMapped = _isMapped.exchange(false);
    if (!wasMapped) {
        return;  // Already unmapped
    }
    
    if (!_impl || !_impl->isValid()) {
        _mappedData = nullptr;
        _mappedRange = {0, 0};
        return;
    }
    
    auto wgpuBuffer = _impl->getNativeHandle().as<WGPUBuffer>();
    if (wgpuBuffer) {
        wgpuBufferUnmap(wgpuBuffer);
    }
    
    _mappedData = nullptr;
    _mappedRange = {0, 0};
}

bool WebGPUMappableBuffer::isMapped() const {
    return _isMapped;
}

bool WebGPUMappableBuffer::isMapPending() const {
    // WebGPU doesn't provide a way to check pending status directly
    // This would need to be tracked separately if needed
    return false;
}

void WebGPUMappableBuffer::flushMappedRange(uint64_t offset, uint64_t size) {
    // WebGPU doesn't require explicit flush operations
    // Data is automatically synchronized when unmapped
}

void WebGPUMappableBuffer::invalidateMappedRange(uint64_t offset, uint64_t size) {
    // WebGPU doesn't require explicit invalidate operations
    // Data is automatically synchronized when mapped
}

// IBuffer interface delegation
uint64_t WebGPUMappableBuffer::getSize() const {
    return _impl ? _impl->getSize() : 0;
}

BufferUsage WebGPUMappableBuffer::getUsage() const {
    return _impl ? _impl->getUsage() : BufferUsage::None;
}

const std::string& WebGPUMappableBuffer::getDebugName() const {
    static const std::string empty;
    return _impl ? _impl->getDebugName() : empty;
}

NativeBufferHandle WebGPUMappableBuffer::getNativeHandle() const {
    return _impl ? _impl->getNativeHandle() : NativeBufferHandle::fromBackend(nullptr);
}

bool WebGPUMappableBuffer::isValid() const {
    return _impl && _impl->isValid();
}

BufferState WebGPUMappableBuffer::getState() const {
    if (!_impl) {
        return BufferState::Uninitialized;
    }
    if (_isMapped) {
        return BufferState::Mapped;
    }
    return BufferState::Ready;
}

MemoryLocation WebGPUMappableBuffer::getMemoryLocation() const {
    return _impl ? _impl->getMemoryLocation() : MemoryLocation::DeviceLocal;
}

AccessPattern WebGPUMappableBuffer::getAccessPattern() const {
    return _impl ? _impl->getAccessPattern() : AccessPattern::Static;
}

} // namespace pers
