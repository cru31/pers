#include "pers/utils/TodoOrDie.h"
#include "pers/utils/Logger.h"

namespace pers {

void TodoOrDie::Log(const std::string& functionName, const std::string& todoDescription, const LogSource& source) {
    Logger::Instance().Log(LogLevel::TodoOrDie, "TodoOrDie", 
                          "Function must be implemented: " + functionName + " - TODO: " + todoDescription, 
                          source);
}

} // namespace pers