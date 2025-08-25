#include "pers/utils/NotImplemented.h"
#include "pers/utils/Logger.h"

namespace pers {

void NotImplemented::Log(const std::string& functionName, const std::string& todoDescription, const LogSource& source) {
    Logger::Instance().Log(LogLevel::Warning, "NotImplemented", 
                          "Function not yet implemented: " + functionName + " - TODO: " + todoDescription, 
                          source);
}

} // namespace pers