#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <rapidjson/document.h>

namespace pers::tests::json {

struct JsonTestMetadata {
    std::string version;
    std::string date;
    int totalTests = 0;
    std::vector<std::string> categories;
};

struct JsonTestCase {
    std::string id;
    std::string category;
    std::string testType;
    std::unordered_map<std::string, std::string> inputValues;
    std::string expectedResult;
    std::vector<std::string> expectedCallstack;
    int timeoutMs = 5000;
    bool enabled = true;
    std::vector<std::string> dependencies;
    std::string reason;
};

class JsonTestLoader {
public:
    JsonTestLoader() = default;
    ~JsonTestLoader() = default;
    
    bool loadFromFile(const std::string& filePath);
    
    const JsonTestMetadata& getMetadata() const { return _metadata; }
    const std::vector<JsonTestCase>& getTestCases() const { return _testCases; }
    std::vector<JsonTestCase> getTestsByCategory(const std::string& category) const;
    std::vector<JsonTestCase> getTestSuite(const std::string& suiteName) const;
    
    bool executeTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason);
    bool executeOptionBasedTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason);
    
private:
    JsonTestCase parseTestCase(const rapidjson::Value& value);
    
    rapidjson::Document _document;
    JsonTestMetadata _metadata;
    std::vector<JsonTestCase> _testCases;
    std::unordered_map<std::string, std::vector<std::string>> _testSuites;
};

} // namespace pers::tests::json