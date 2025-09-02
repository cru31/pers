#pragma once

#include <string>
#include <functional>
#include "pers/utils/Logger.h"

namespace pers {

class TodoOrDie {
public:
    // Callback type
    // skipLogging: callback can set this to true to skip the default logger output
    using Callback = std::function<void(const std::string& functionName, 
                                        const std::string& todoDescription,
                                        const LogSource& source,
                                        bool& skipLogging)>;
    
    // Set callback function
    static void setCallback(const Callback& callback);
    
    // Clear callback
    static void clearCallback();
    
    // Main logging function
    static void Log(const std::string& functionName, 
                   const std::string& todoDescription,
                   const LogSource& source);
                   
private:
    static Callback _callback;
};

} // namespace pers