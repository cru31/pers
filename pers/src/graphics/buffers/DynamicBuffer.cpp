#include "pers/graphics/buffers/DynamicBuffer.h"
#include "pers/graphics/buffers/IBufferFactory.h"
#include "pers/utils/Logger.h"
#include <sstream>

namespace pers {

DynamicBuffer::DynamicBuffer(const BufferDesc& desc, const std::shared_ptr<IBufferFactory>& factory, 
                           uint32_t frameCount)
    : _desc(desc)
    , _currentFrame(0)
    , _frameCount(frameCount) {
    
    if (!factory) {
        LOG_ERROR("DynamicBuffer", "Factory is null");
        return;
    }
    
    if (frameCount == 0 || frameCount > 10) {
        LOG_ERROR("DynamicBuffer", "Invalid frame count (must be 1-10)");
        return;
    }
    
    // Create ring buffer instances
    _buffers.reserve(frameCount);
    _mapped.resize(frameCount, false);
    
    BufferDesc bufferDesc = desc;
    bufferDesc.usage |= BufferUsage::MapWrite | BufferUsage::CopySrc;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.accessPattern = AccessPattern::Dynamic;
    
    if (bufferDesc.memoryLocation == MemoryLocation::Auto) {
        bufferDesc.memoryLocation = MemoryLocation::HostVisible;
    }
    
    for (uint32_t i = 0; i < frameCount; ++i) {
        std::stringstream ss;
        ss << desc.debugName << "_Frame" << i;
        bufferDesc.debugName = ss.str();
        
        auto buffer = factory->createMappableBuffer(bufferDesc);
        if (!buffer) {
            LOG_ERROR("DynamicBuffer", "Failed to create buffer for frame");
            _buffers.clear();
            return;
        }
        
        _buffers.push_back(std::move(buffer));
    }
    
    std::stringstream ss;
    ss << "Created dynamic buffer '" << desc.debugName 
       << "' with " << frameCount << " frames, size=" << desc.size;
    LOG_DEBUG("DynamicBuffer", ss.str().c_str());
}

DynamicBuffer::~DynamicBuffer() {
    for (size_t i = 0; i < _buffers.size(); ++i) {
        if (_mapped[i] && _buffers[i]) {
            _buffers[i]->unmap();
        }
    }
}

DynamicBuffer::DynamicBuffer(DynamicBuffer&& other) noexcept
    : _desc(std::move(other._desc))
    , _buffers(std::move(other._buffers))
    , _currentFrame(other._currentFrame.load())
    , _frameCount(other._frameCount)
    , _mapped(std::move(other._mapped)) {
    
    other._frameCount = 0;
    other._currentFrame = 0;
}

DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&& other) noexcept {
    if (this != &other) {
        // Unmap any mapped buffers
        for (size_t i = 0; i < _buffers.size(); ++i) {
            if (_mapped[i] && _buffers[i]) {
                _buffers[i]->unmap();
            }
        }
        
        _desc = std::move(other._desc);
        _buffers = std::move(other._buffers);
        _currentFrame = other._currentFrame.load();
        _frameCount = other._frameCount;
        _mapped = std::move(other._mapped);
        
        other._frameCount = 0;
        other._currentFrame = 0;
    }
    return *this;
}

DynamicBuffer::UpdateHandle DynamicBuffer::beginUpdate() {
    uint32_t frameIndex = _currentFrame % _frameCount;
    
    if (frameIndex >= _buffers.size() || !_buffers[frameIndex]) {
        LOG_ERROR("DynamicBuffer", "Invalid frame buffer");
        return UpdateHandle{nullptr, 0, frameIndex};
    }
    
    auto& buffer = _buffers[frameIndex];
    
    // Unmap if previously mapped
    if (_mapped[frameIndex]) {
        buffer->unmap();
        _mapped[frameIndex] = false;
    }
    
    // Map for writing
    auto mappedData = buffer->mapAsync(MapMode::Write).get();
    
    if (!mappedData.data()) {
        LOG_ERROR("DynamicBuffer", "Failed to map buffer for update");
        return UpdateHandle{nullptr, 0, frameIndex};
    }
    
    _mapped[frameIndex] = true;
    
    return UpdateHandle{
        mappedData.data(),
        mappedData.size(),
        frameIndex
    };
}

void DynamicBuffer::endUpdate(const UpdateHandle& handle) {
    if (handle.frameIndex >= _frameCount || handle.frameIndex >= _buffers.size()) {
        return;
    }
    
    if (_mapped[handle.frameIndex] && _buffers[handle.frameIndex]) {
        _buffers[handle.frameIndex]->unmap();
        _mapped[handle.frameIndex] = false;
    }
}

std::shared_ptr<IBuffer> DynamicBuffer::getCurrentFrameBuffer() const {
    uint32_t frameIndex = _currentFrame % _frameCount;
    
    if (frameIndex < _buffers.size()) {
        return _buffers[frameIndex];
    }
    
    return nullptr;
}

uint32_t DynamicBuffer::getCurrentFrameIndex() const {
    return _currentFrame % _frameCount;
}

void DynamicBuffer::nextFrame() {
    _currentFrame++;
}

uint64_t DynamicBuffer::getSize() const {
    return _desc.size;
}

BufferUsage DynamicBuffer::getUsage() const {
    return _desc.usage;
}

const std::string& DynamicBuffer::getDebugName() const {
    return _desc.debugName;
}

NativeBufferHandle DynamicBuffer::getNativeHandle() const {
    auto currentBuffer = getCurrentFrameBuffer();
    return currentBuffer ? currentBuffer->getNativeHandle() : NativeBufferHandle::fromBackend(nullptr);
}

bool DynamicBuffer::isValid() const {
    return !_buffers.empty() && _buffers[0] && _buffers[0]->isValid();
}

BufferState DynamicBuffer::getState() const {
    uint32_t frameIndex = _currentFrame % _frameCount;
    
    if (frameIndex < _buffers.size() && _buffers[frameIndex]) {
        return _buffers[frameIndex]->getState();
    }
    
    return BufferState::Uninitialized;
}

MemoryLocation DynamicBuffer::getMemoryLocation() const {
    return _desc.memoryLocation;
}

AccessPattern DynamicBuffer::getAccessPattern() const {
    return AccessPattern::Dynamic;
}

} // namespace pers