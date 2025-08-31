#pragma once

#include <string>
#include "pers/utils/Logger.h"

namespace pers {

class TodoOrDie {
public:
    static void Log(const std::string& functionName, 
                   const std::string& todoDescription,
                   const LogSource& source);
};

} // namespace pers