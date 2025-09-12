#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <vector>

#include <iostream>
namespace pers::tests {

// Expected behavior for test validation
struct ExpectedBehavior {
    // Expected return value: "not_null", "nullptr", "true", "false", or specific value
    std::string returnValue;
    
    // Property validations
    std::unordered_map<std::string, std::any> properties;
    
    // Error expectations
    std::string errorCode;
    std::vector<std::string> errorMessageContains;
    
    // Numeric validations (for limits, sizes, etc)
    std::unordered_map<std::string, std::string> numericChecks; // e.g., "maxBufferSize": ">=256"
};

// Test variation with options and expected behavior
struct TestVariation {
    int id;
    std::string combinedId;  // Combined ID with file ID (e.g., "001-a")
    std::string variationName;
    std::unordered_map<std::string, std::any> options;
    ExpectedBehavior expectedBehavior;
};

// Test type definition
struct TestTypeDefinition {
    std::string category;
    std::string testType;
    std::string handlerClass;
    std::vector<TestVariation> variations;
};

// Structured log entry
struct LogEntry {
    std::string timestamp;      // "HH:MM:SS.mmm"
    std::string level;          // "INFO", "ERROR", etc.
    std::string category;       // Log category
    std::string message;        // Log message
    std::string file;           // Source file path
    int line;                   // Line number
    std::string function;       // Function name
};

// Test execution result
struct TestResult {
    bool passed;
    std::string actualBehavior;
    std::string failureReason;
    std::unordered_map<std::string, std::any> actualProperties;
    std::vector<LogEntry> logMessages;  // Structured log messages
};

// Helper to parse size strings like "1MB", "64KB", etc.
inline size_t parseSizeString(const std::string& sizeStr) {
    size_t value = 0;
    size_t multiplier = 1;
    
    // Try to find unit suffix
    size_t numEnd = sizeStr.find_first_not_of("0123456789");
    if (numEnd == std::string::npos) {
        // Plain number, no suffix
        return std::stoull(sizeStr);
    }
    
    // Parse the numeric part
    value = std::stoull(sizeStr.substr(0, numEnd));
    
    // Parse the unit suffix
    std::string unit = sizeStr.substr(numEnd);
    if (unit == "KB" || unit == "kb" || unit == "K" || unit == "k") {
        multiplier = 1024;
    } else if (unit == "MB" || unit == "mb" || unit == "M" || unit == "m") {
        multiplier = 1024 * 1024;
    } else if (unit == "GB" || unit == "gb" || unit == "G" || unit == "g") {
        multiplier = 1024 * 1024 * 1024;
    }
    
    return value * multiplier;
}

// Helper to get option value with type safety - NO TRY-CATCH!
template<typename T>
T getOption(const std::unordered_map<std::string, std::any>& options, 
            const std::string& key, 
            const T& defaultValue = T{}) {
    auto it = options.find(key);
    if (it == options.end()) {
        return defaultValue;
    }
    
    // Direct cast attempt - if it works, return immediately
    if (it->second.type() == typeid(T)) {
        return std::any_cast<T>(it->second);
    }
    
    // Special handling for size_t when source is string
    if constexpr (std::is_same_v<T, size_t>) {
        if (it->second.type() == typeid(std::string)) {
            const std::string& strValue = std::any_cast<const std::string&>(it->second);
            return parseSizeString(strValue);
        }
        if (it->second.type() == typeid(const char*)) {
            std::string strValue(std::any_cast<const char*>(it->second));
            return parseSizeString(strValue);
        }
        // Try int conversion
        if (it->second.type() == typeid(int)) {
            return static_cast<size_t>(std::any_cast<int>(it->second));
        }
    }
    
    // Special handling for string
    if constexpr (std::is_same_v<T, std::string>) {
        if (it->second.type() == typeid(const char*)) {
            return std::string(std::any_cast<const char*>(it->second));
        }
    }
    
    // If we cant convert, log error and return default
    std::cerr << "[ERROR] getOption: Type mismatch for key " << key 
              << ". Expected type: " << typeid(T).name() 
              << ", Actual type: " << it->second.type().name() << std::endl;
    return defaultValue;
}


// Helper to check numeric conditions
inline bool checkNumericCondition(const std::string& condition, double actualValue) {
    if (condition.empty()) return true;
    
    if (condition.substr(0, 2) == ">=") {
        double expected = std::stod(condition.substr(2));
        return actualValue >= expected;
    } else if (condition.substr(0, 2) == "<=") {
        double expected = std::stod(condition.substr(2));
        return actualValue <= expected;
    } else if (condition.substr(0, 2) == "==") {
        double expected = std::stod(condition.substr(2));
        return actualValue == expected;
    } else if (condition.substr(0, 2) == "!=") {
        double expected = std::stod(condition.substr(2));
        return actualValue != expected;
    } else if (condition[0] == '>') {
        double expected = std::stod(condition.substr(1));
        return actualValue > expected;
    } else if (condition[0] == '<') {
        double expected = std::stod(condition.substr(1));
        return actualValue < expected;
    }
    
    // If no operator, assume exact match
    double expected = std::stod(condition);
    return actualValue == expected;
}

} // namespace pers::tests