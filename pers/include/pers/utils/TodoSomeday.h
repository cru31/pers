#pragma once

#include <string>
#include "pers/utils/Logger.h"

namespace pers {

/**
 * @brief TodoSomeday marks features that should be implemented in the future
 * but are not critical for current functionality
 * 
 * Unlike TodoOrDie which marks critical missing features,
 * TodoSomeday is for nice-to-have improvements or features
 * waiting for external dependencies to stabilize.
 */
class TodoSomeday {
public:
    /**
     * @brief Log a future improvement or feature
     * @param context The class or module context
     * @param description Description of what needs to be done in the future
     * @param source Source location info
     */
    static void Log(const std::string& context,
                   const std::string& description,
                   const LogSource& source);
};

} // namespace pers