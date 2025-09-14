#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/utils/Logger.h"
#include <algorithm>
#include <sstream>

namespace pers {

DeviceBuffer::DeviceBuffer()
    : _desc()
    , _totalBytesTransferred(0)
    , _transferCount(0)
    , _created(false) {
}

DeviceBuffer::~DeviceBuffer() {
    destroy();
}

bool DeviceBuffer::create(const BufferDesc& desc, const std::shared_ptr<ILogicalDevice>& device) {
    if (_created) {
        LOG_ERROR("DeviceBuffer", "Buffer already created");
        return false;
    }
    
    if (!desc.isValid()) {
        LOG_ERROR("DeviceBuffer", "Invalid buffer description");
        return false;
    }
    
    if (!device) {
        LOG_ERROR("DeviceBuffer", "Device is null");
        return false;
    }
    
    const auto& resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        LOG_ERROR("DeviceBuffer", "Failed to get resource factory from device");
        return false;
    }
    
    _desc = desc;
    
    // Force GPU-only configuration
    BufferDesc deviceDesc = desc;
    deviceDesc.mappedAtCreation = false;
    deviceDesc.usage |= BufferUsage::CopyDst;
    
    if (deviceDesc.memoryLocation == MemoryLocation::Auto) {
        deviceDesc.memoryLocation = MemoryLocation::DeviceLocal;
    }
    
    _buffer = resourceFactory->createBuffer(deviceDesc);
    if (!_buffer) {
        LOG_ERROR("DeviceBuffer", "Failed to create underlying buffer");
        return false;
    }
    
    _created = true;
    _totalBytesTransferred = 0;
    _transferCount = 0;
    
    std::stringstream ss;
    ss << "Created device buffer '" << _desc.debugName << "' size=" << _desc.size 
       << " usage=0x" << std::hex << static_cast<uint32_t>(_desc.usage);
    LOG_DEBUG("DeviceBuffer", ss.str().c_str());
    
    return true;
}

void DeviceBuffer::destroy() {
    if (!_created) {
        return;
    }
    
    if (_transferCount > 0) {
        std::stringstream ss;
        ss << "Destroyed device buffer '" << _desc.debugName 
           << "' - total transfers: " << _transferCount 
           << ", bytes: " << _totalBytesTransferred;
        LOG_DEBUG("DeviceBuffer", ss.str().c_str());
    }
    
    _buffer.reset();
    _totalBytesTransferred = 0;
    _transferCount = 0;
    _created = false;
    _desc = BufferDesc();
}

DeviceBuffer::DeviceBuffer(DeviceBuffer&& other) noexcept
    : _buffer(std::move(other._buffer))
    , _desc(std::move(other._desc))
    , _totalBytesTransferred(other._totalBytesTransferred)
    , _transferCount(other._transferCount)
    , _created(other._created) {
    other._totalBytesTransferred = 0;
    other._transferCount = 0;
    other._created = false;
}

DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        
        _buffer = std::move(other._buffer);
        _desc = std::move(other._desc);
        _totalBytesTransferred = other._totalBytesTransferred;
        _transferCount = other._transferCount;
        _created = other._created;
        
        other._totalBytesTransferred = 0;
        other._transferCount = 0;
        other._created = false;
    }
    return *this;
}

// IBuffer interface
uint64_t DeviceBuffer::getSize() const {
    return _created && _buffer ? _buffer->getSize() : 0;
}

BufferUsage DeviceBuffer::getUsage() const {
    return _created && _buffer ? _buffer->getUsage() : BufferUsage::None;
}

const std::string& DeviceBuffer::getDebugName() const {
    static const std::string empty;
    return _created ? _desc.debugName : empty;
}

NativeBufferHandle DeviceBuffer::getNativeHandle() const {
    return _created && _buffer ? _buffer->getNativeHandle() : NativeBufferHandle{};
}

bool DeviceBuffer::isValid() const {
    return _created && _buffer && _buffer->isValid();
}

BufferState DeviceBuffer::getState() const {
    if (!_created || !_buffer) return BufferState::Uninitialized;
    return BufferState::Ready;
}

MemoryLocation DeviceBuffer::getMemoryLocation() const {
    return _created && _buffer ? _buffer->getMemoryLocation() : MemoryLocation::Auto;
}

AccessPattern DeviceBuffer::getAccessPattern() const {
    return _desc.accessPattern;
}

} // namespace pers

