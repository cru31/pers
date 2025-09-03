#pragma once

#include "test_variation.h"
#include <string>
#include <vector>

namespace pers::tests {

class JsonTestLoader {
public:
    // Load test types from JSON file with file identification
    static bool loadTestTypes(const std::string& filePath, 
                             std::vector<TestTypeDefinition>& outTestTypes);
    
    // Save test results to JSON file
    static bool saveTestResults(const std::string& filePath,
                               const std::vector<TestTypeDefinition>& testTypes,
                               const std::vector<std::vector<TestResult>>& results,
                               const std::string& testCaseJsonPath = "");
    
    // Generate a 5-character hash from file path and timestamp
    static std::string generateFileHash(const std::string& filePath);
    
private:
    // Parse variation from JSON object
    static TestVariation parseVariation(const void* jsonObj);
    
    // Parse expected behavior from JSON object
    static ExpectedBehavior parseExpectedBehavior(const void* jsonObj);
    
    // Parse options from JSON object
    static std::unordered_map<std::string, std::any> parseOptions(const void* jsonObj);
};

} // namespace pers::tests