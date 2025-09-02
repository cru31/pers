#include "json_test_loader.h"
#include "test_handlers.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <rapidjson/filereadstream.h>

namespace pers::tests::json {

bool JsonTestLoader::loadFromFile(const std::string& filePath) {
    FILE* fp = std::fopen(filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open JSON file: " << filePath << std::endl;
        return false;
    }
    
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    _document.ParseStream(is);
    std::fclose(fp);
    
    if (_document.HasParseError()) {
        std::cerr << "JSON parse error at offset " << _document.GetErrorOffset() << std::endl;
        return false;
    }
    
    // Parse metadata
    if (_document.HasMember("metadata") && _document["metadata"].IsObject()) {
        const auto& metadata = _document["metadata"];
        
        if (metadata.HasMember("version") && metadata["version"].IsString()) {
            _metadata.version = metadata["version"].GetString();
        }
        if (metadata.HasMember("date") && metadata["date"].IsString()) {
            _metadata.date = metadata["date"].GetString();
        }
        if (metadata.HasMember("total_tests") && metadata["total_tests"].IsInt()) {
            _metadata.totalTests = metadata["total_tests"].GetInt();
        }
        if (metadata.HasMember("categories") && metadata["categories"].IsArray()) {
            const auto& categories = metadata["categories"];
            for (rapidjson::SizeType i = 0; i < categories.Size(); i++) {
                if (categories[i].IsString()) {
                    _metadata.categories.push_back(categories[i].GetString());
                }
            }
        }
    }
    
    // Parse test cases
    if (_document.HasMember("test_cases") && _document["test_cases"].IsArray()) {
        const auto& testCases = _document["test_cases"];
        for (rapidjson::SizeType i = 0; i < testCases.Size(); i++) {
            if (testCases[i].IsObject()) {
                JsonTestCase testCase = parseTestCase(testCases[i]);
                _testCases.push_back(testCase);
            }
        }
    }
    
    // Parse test suites
    if (_document.HasMember("test_suites") && _document["test_suites"].IsObject()) {
        const auto& suites = _document["test_suites"];
        for (auto it = suites.MemberBegin(); it != suites.MemberEnd(); ++it) {
            std::string suiteName = it->name.GetString();
            std::vector<std::string> testIds;
            
            if (it->value.IsArray()) {
                const auto& ids = it->value;
                for (rapidjson::SizeType i = 0; i < ids.Size(); i++) {
                    if (ids[i].IsString()) {
                        testIds.push_back(ids[i].GetString());
                    }
                }
            } else if (it->value.IsString() && std::string(it->value.GetString()) == "all") {
                // Special case for "all"
                for (const auto& tc : _testCases) {
                    testIds.push_back(tc.id);
                }
            }
            
            _testSuites[suiteName] = testIds;
        }
    }
    
    return true;
}

JsonTestCase JsonTestLoader::parseTestCase(const rapidjson::Value& value) {
    JsonTestCase testCase;
    
    if (value.HasMember("id") && value["id"].IsString()) {
        testCase.id = value["id"].GetString();
    }
    
    if (value.HasMember("category") && value["category"].IsString()) {
        testCase.category = value["category"].GetString();
    }
    
    if (value.HasMember("test_type") && value["test_type"].IsString()) {
        testCase.testType = value["test_type"].GetString();
    }
    
    if (value.HasMember("input") && value["input"].IsObject()) {
        const auto& input = value["input"];
        
        // Check for type field
        if (input.HasMember("type") && input["type"].IsString()) {
            testCase.inputValues["type"] = input["type"].GetString();
        }
        
        // Parse options for option-based tests
        if (input.HasMember("options") && input["options"].IsObject()) {
            const auto& options = input["options"];
            for (auto it = options.MemberBegin(); it != options.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                std::string val;
                
                if (it->value.IsString()) {
                    val = it->value.GetString();
                } else if (it->value.IsInt()) {
                    val = std::to_string(it->value.GetInt());
                } else if (it->value.IsBool()) {
                    val = it->value.GetBool() ? "true" : "false";
                } else if (it->value.IsNull()) {
                    val = "null";
                } else if (it->value.IsArray()) {
                    // Handle array values
                    std::stringstream ss;
                    ss << "[";
                    for (rapidjson::SizeType i = 0; i < it->value.Size(); i++) {
                        if (i > 0) ss << ",";
                        if (it->value[i].IsString()) {
                            ss << "\"" << it->value[i].GetString() << "\"";
                        }
                    }
                    ss << "]";
                    val = ss.str();
                }
                
                testCase.inputValues[key] = val;
            }
        }
        
        // Parse values for regular tests
        if (input.HasMember("values") && input["values"].IsObject()) {
            const auto& values = input["values"];
            for (auto it = values.MemberBegin(); it != values.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                std::string val;
                
                if (it->value.IsString()) {
                    val = it->value.GetString();
                } else if (it->value.IsInt()) {
                    val = std::to_string(it->value.GetInt());
                } else if (it->value.IsBool()) {
                    val = it->value.GetBool() ? "true" : "false";
                } else if (it->value.IsNull()) {
                    val = "null";
                } else if (it->value.IsArray()) {
                    // Handle array values
                    std::stringstream ss;
                    ss << "[";
                    for (rapidjson::SizeType i = 0; i < it->value.Size(); i++) {
                        if (i > 0) ss << ",";
                        if (it->value[i].IsString()) {
                            ss << "\"" << it->value[i].GetString() << "\"";
                        }
                    }
                    ss << "]";
                    val = ss.str();
                }
                
                testCase.inputValues[key] = val;
            }
        }
    }
    
    if (value.HasMember("expected_result") && value["expected_result"].IsString()) {
        testCase.expectedResult = value["expected_result"].GetString();
    }
    
    if (value.HasMember("expected_callstack") && value["expected_callstack"].IsArray()) {
        const auto& callstack = value["expected_callstack"];
        for (rapidjson::SizeType i = 0; i < callstack.Size(); i++) {
            if (callstack[i].IsString()) {
                testCase.expectedCallstack.push_back(callstack[i].GetString());
            }
        }
    }
    
    if (value.HasMember("timeout_ms") && value["timeout_ms"].IsInt()) {
        testCase.timeoutMs = value["timeout_ms"].GetInt();
    }
    
    if (value.HasMember("enabled") && value["enabled"].IsBool()) {
        testCase.enabled = value["enabled"].GetBool();
    }
    
    if (value.HasMember("dependencies") && value["dependencies"].IsArray()) {
        const auto& deps = value["dependencies"];
        for (rapidjson::SizeType i = 0; i < deps.Size(); i++) {
            if (deps[i].IsString()) {
                testCase.dependencies.push_back(deps[i].GetString());
            }
        }
    }
    
    if (value.HasMember("reason") && value["reason"].IsString()) {
        testCase.reason = value["reason"].GetString();
    }
    
    return testCase;
}

std::vector<JsonTestCase> JsonTestLoader::getTestsByCategory(const std::string& category) const {
    std::vector<JsonTestCase> result;
    for (const auto& test : _testCases) {
        if (test.category == category) {
            result.push_back(test);
        }
    }
    return result;
}

std::vector<JsonTestCase> JsonTestLoader::getTestSuite(const std::string& suiteName) const {
    std::vector<JsonTestCase> result;
    
    auto it = _testSuites.find(suiteName);
    if (it != _testSuites.end()) {
        for (const auto& testId : it->second) {
            for (const auto& test : _testCases) {
                if (test.id == testId) {
                    result.push_back(test);
                    break;
                }
            }
        }
    }
    
    return result;
}

bool JsonTestLoader::executeOptionBasedTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason) {
    // All option-based tests are now handled by TestHandlerRegistry
    return TestHandlerRegistry::Instance().executeTest(testCase, actualResult, failureReason);
}

bool JsonTestLoader::executeTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason) {
    // Use the TestHandlerRegistry to execute tests
    return TestHandlerRegistry::Instance().executeTest(testCase, actualResult, failureReason);
}

} // namespace pers::tests::json