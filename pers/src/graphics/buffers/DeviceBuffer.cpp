#include "pers/graphics/buffers/INativeBuffer.h"
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
    : _size(0)
    , _usage(BufferUsage::None)
    , _debugName()
    , _totalBytesTransferred(0)
    , _transferCount(0)
    , _created(false) {
}

DeviceBuffer::~DeviceBuffer() {
    destroy();
}

bool DeviceBuffer::create(uint64_t size, DeviceBufferUsage usage, const std::shared_ptr<ILogicalDevice>& device, const std::string& debugName) {
    if (_created) {
        LOG_ERROR("DeviceBuffer", "Buffer already created");
        return false;
    }
    
    if (size == 0) {
        LOG_ERROR("DeviceBuffer", "Invalid buffer size (0)");
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
    
    _size = size;
    _usage = toBufferUsage(usage);
    _debugName = debugName;
    
    // Force GPU-only configuration
    BufferDesc deviceDesc;
    deviceDesc.size = size;
    deviceDesc.debugName = debugName;
    deviceDesc.usage = toBufferUsage(usage);
    deviceDesc.mappedAtCreation = false;
    _usage = deviceDesc.usage;
    
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
    ss << "Created device buffer '" << _debugName << "' size=" << _size 
       << " usage=0x" << std::hex << static_cast<uint32_t>(_usage);
    LOG_DEBUG("DeviceBuffer", ss.str().c_str());
    
    return true;
}

void DeviceBuffer::destroy() {
    if (!_created) {
        return;
    }
    
    if (_transferCount > 0) {
        std::stringstream ss;
        ss << "Destroyed device buffer '" << _debugName 
           << "' - total transfers: " << _transferCount 
           << ", bytes: " << _totalBytesTransferred;
        LOG_DEBUG("DeviceBuffer", ss.str().c_str());
    }
    
    _buffer.reset();
    _totalBytesTransferred = 0;
    _transferCount = 0;
    _created = false;
    _size = 0;
    _usage = BufferUsage::None;
    _debugName.clear();
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
    return _created ? _debugName : empty;
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
    return AccessPattern::Static;
}

} // namespace pers

