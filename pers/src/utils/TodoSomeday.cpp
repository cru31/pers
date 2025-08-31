#include "pers/utils/TodoSomeday.h"
#include "pers/utils/Logger.h"

namespace pers {

void TodoSomeday::Log(const std::string& context,
                     const std::string& description,
                     const LogSource& source) {
    Logger::Instance().Log(LogLevel::TodoSomeday, 
                          context, 
                          description,
                          source);
}

} // namespace pers