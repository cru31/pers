#include "pers/graphics/buffers/DynamicBuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

DynamicBuffer::DynamicBuffer() 
    : _currentFrame(0)
    , _frameCount(0)
    , _created(false) {
}

DynamicBuffer::~DynamicBuffer() {
    TODO_OR_DIE("DynamicBuffer", "Implement destructor");
}

bool DynamicBuffer::create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device, uint32_t frameCount) {
    TODO_OR_DIE("DynamicBuffer", "Implement create method");
    return false;
}

void DynamicBuffer::destroy() {
    TODO_OR_DIE("DynamicBuffer", "Implement destroy method");
}

DynamicBuffer::DynamicBuffer(DynamicBuffer&& other) noexcept
    : _desc(std::move(other._desc))
    , _buffers(std::move(other._buffers))
    , _currentFrame(other._currentFrame.load())
    , _frameCount(other._frameCount)
    , _mapped(std::move(other._mapped))
    , _created(other._created) {
    other._created = false;
}

DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        _desc = std::move(other._desc);
        _buffers = std::move(other._buffers);
        _currentFrame = other._currentFrame.load();
        _frameCount = other._frameCount;
        _mapped = std::move(other._mapped);
        _created = other._created;
        other._created = false;
    }
    return *this;
}

DynamicBuffer::UpdateHandle DynamicBuffer::beginUpdate() {
    TODO_OR_DIE("DynamicBuffer", "Implement beginUpdate");
    return UpdateHandle{nullptr, 0, 0};
}

void DynamicBuffer::endUpdate(const UpdateHandle& handle) {
    TODO_OR_DIE("DynamicBuffer", "Implement endUpdate");
}

std::shared_ptr<IBuffer> DynamicBuffer::getCurrentFrameBuffer() const {
    TODO_OR_DIE("DynamicBuffer", "Implement getCurrentFrameBuffer");
    return nullptr;
}

uint32_t DynamicBuffer::getCurrentFrameIndex() const {
    return _currentFrame.load();
}

void DynamicBuffer::nextFrame() {
    _currentFrame = (_currentFrame + 1) % _frameCount;
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
    TODO_OR_DIE("DynamicBuffer", "Implement getNativeHandle");
    return NativeBufferHandle{};
}

bool DynamicBuffer::isValid() const {
    return _created && !_buffers.empty();
}

BufferState DynamicBuffer::getState() const {
    if (!_created) return BufferState::Uninitialized;
    uint32_t idx = _currentFrame.load();
    if (idx < _mapped.size() && _mapped[idx]) {
        return BufferState::Mapped;
    }
    return BufferState::Ready;
}

MemoryLocation DynamicBuffer::getMemoryLocation() const {
    return _desc.memoryLocation;
}

AccessPattern DynamicBuffer::getAccessPattern() const {
    return AccessPattern::Dynamic;
}

} // namespace pers
