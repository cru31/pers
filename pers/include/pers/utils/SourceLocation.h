#pragma once

namespace pers {

// Source location info for debugging
struct SourceLocation {
    const char* file;
    int line;
    const char* function;
};

// Alias for compatibility
using LogSource = SourceLocation;

// Macro to capture source location information
#define PERS_SOURCE_LOC pers::SourceLocation{__FILE__, __LINE__, __FUNCTION__}

} // namespace pers