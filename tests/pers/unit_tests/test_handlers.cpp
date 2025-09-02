#include "test_handlers.h"
#include "webgpu_instance_handler.h"
#include "adapter_request_handler.h"
#include "device_creation_handler.h"
#include "queue_creation_handler.h"
#include "command_encoder_handler.h"
#include "buffer_creation_handler.h"
#include "shader_handler.h"
#include "queue_operations_handler.h"

namespace pers::tests::json {

// Register all handlers
static void registerAllHandlers() {
    auto& registry = TestHandlerRegistry::Instance();
    
    // Core tests
    registry.registerHandler("WebGPU Instance Creation", std::make_shared<WebGPUInstanceCreationHandler>());
    registry.registerHandler("Adapter Request", std::make_shared<AdapterRequestHandler>());
    registry.registerHandler("Device Creation", std::make_shared<DeviceCreationHandler>());
    registry.registerHandler("Queue Creation", std::make_shared<QueueCreationHandler>());
    registry.registerHandler("Command Encoder Creation", std::make_shared<CommandEncoderCreationHandler>());
    
    // Buffer tests
    registry.registerHandler("Buffer Creation", std::make_shared<BufferCreationHandler>());
    registry.registerHandler("Buffer Creation 64KB", std::make_shared<BufferSpecificTestHandler>());
    registry.registerHandler("Buffer Creation 0 Size", std::make_shared<BufferSpecificTestHandler>());
    
    // Shader tests
    registry.registerHandler("Shader Module Creation", std::make_shared<ShaderModuleCreationHandler>());
    
    // Queue operations tests
    registry.registerHandler("Queue Submit Empty", std::make_shared<QueueSubmitEmptyHandler>());
}

// Automatically register handlers when the program starts
static bool _handlers_registered = []() {
    registerAllHandlers();
    return true;
}();

} // namespace pers::tests::json