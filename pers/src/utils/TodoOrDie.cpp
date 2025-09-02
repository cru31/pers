#include "pers/utils/TodoOrDie.h"
#include "pers/utils/Logger.h"
#include <cstdlib>
#include <sstream>

namespace pers {

// Static member definition
TodoOrDie::Callback TodoOrDie::_callback = nullptr;

void TodoOrDie::setCallback(const Callback& callback) {
    _callback = callback;
}

void TodoOrDie::clearCallback() {
    _callback = nullptr;
}

void TodoOrDie::Log(const std::string& functionName, const std::string& todoDescription, const LogSource& source) {
    bool skipLogging = false;
    
    // Call callback if set
    if (_callback) {
        _callback(functionName, todoDescription, source, skipLogging);
    }
    
    // Log unless skipped
    if (!skipLogging) {
        Logger::Instance().Log(LogLevel::TodoOrDie, "TodoOrDie", 
                              "Function must be implemented: " + functionName + " - TODO: " + todoDescription, 
                              source);
    }
    
    // Abort if no callback
    if (!_callback) {
        std::abort();
    }
}

} // namespace pers