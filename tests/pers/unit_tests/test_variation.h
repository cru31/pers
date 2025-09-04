#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <vector>

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

// Test execution result
struct TestResult {
    bool passed;
    std::string actualBehavior;
    std::string failureReason;
    std::unordered_map<std::string, std::any> actualProperties;
    std::vector<std::string> logMessages;  // Captured log messages during test
};

// Helper to get option value with type safety
template<typename T>
T getOption(const std::unordered_map<std::string, std::any>& options, 
            const std::string& key, 
            const T& defaultValue = T{}) {
    auto it = options.find(key);
    if (it != options.end()) {
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            // Try to convert if possible
            if constexpr (std::is_same_v<T, std::string>) {
                try {
                    // Try to get as const char*
                    return std::string(std::any_cast<const char*>(it->second));
                } catch (...) {}
            }
            return defaultValue;
        }
    }
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