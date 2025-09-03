#include "json_test_loader.h"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <ctime>

namespace pers::tests {

using namespace rapidjson;

bool JsonTestLoader::loadTestTypes(const std::string& filePath, 
                                   std::vector<TestTypeDefinition>& outTestTypes) {
    FILE* fp = std::fopen(filePath.c_str(), "rb");
    if (!fp) {
        return false;
    }
    
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    std::fclose(fp);
    
    if (doc.HasParseError()) {
        return false;
    }
    
    // Parse test types
    if (!doc.HasMember("testTypes") || !doc["testTypes"].IsArray()) {
        return false;
    }
    
    const auto& testTypes = doc["testTypes"];
    for (SizeType i = 0; i < testTypes.Size(); i++) {
        const auto& testTypeObj = testTypes[i];
        
        TestTypeDefinition testType;
        
        if (testTypeObj.HasMember("category") && testTypeObj["category"].IsString()) {
            testType.category = testTypeObj["category"].GetString();
        }
        
        if (testTypeObj.HasMember("testType") && testTypeObj["testType"].IsString()) {
            testType.testType = testTypeObj["testType"].GetString();
        }
        
        if (testTypeObj.HasMember("handlerClass") && testTypeObj["handlerClass"].IsString()) {
            testType.handlerClass = testTypeObj["handlerClass"].GetString();
        }
        
        // Parse variations
        if (testTypeObj.HasMember("variations") && testTypeObj["variations"].IsArray()) {
            const auto& variations = testTypeObj["variations"];
            for (SizeType j = 0; j < variations.Size(); j++) {
                testType.variations.push_back(parseVariation(&variations[j]));
            }
        }
        
        outTestTypes.push_back(testType);
    }
    
    return true;
}

TestVariation JsonTestLoader::parseVariation(const void* jsonObj) {
    const Value& obj = *static_cast<const Value*>(jsonObj);
    TestVariation variation;
    
    if (obj.HasMember("id") && obj["id"].IsInt()) {
        variation.id = obj["id"].GetInt();
    }
    
    if (obj.HasMember("variationName") && obj["variationName"].IsString()) {
        variation.variationName = obj["variationName"].GetString();
    }
    
    if (obj.HasMember("options") && obj["options"].IsObject()) {
        variation.options = parseOptions(&obj["options"]);
    }
    
    if (obj.HasMember("expectedBehavior") && obj["expectedBehavior"].IsObject()) {
        variation.expectedBehavior = parseExpectedBehavior(&obj["expectedBehavior"]);
    }
    
    return variation;
}

ExpectedBehavior JsonTestLoader::parseExpectedBehavior(const void* jsonObj) {
    const Value& obj = *static_cast<const Value*>(jsonObj);
    ExpectedBehavior behavior;
    
    if (obj.HasMember("shouldSucceed") && obj["shouldSucceed"].IsBool()) {
        behavior.shouldSucceed = obj["shouldSucceed"].GetBool();
    }
    
    if (obj.HasMember("returnValue") && obj["returnValue"].IsString()) {
        behavior.returnValue = obj["returnValue"].GetString();
    }
    
    if (obj.HasMember("errorCode") && obj["errorCode"].IsString()) {
        behavior.errorCode = obj["errorCode"].GetString();
    }
    
    // Parse properties
    if (obj.HasMember("properties") && obj["properties"].IsObject()) {
        const auto& props = obj["properties"];
        for (auto it = props.MemberBegin(); it != props.MemberEnd(); ++it) {
            const std::string key = it->name.GetString();
            const auto& value = it->value;
            
            if (value.IsBool()) {
                behavior.properties[key] = value.GetBool();
            } else if (value.IsInt()) {
                behavior.properties[key] = value.GetInt();
            } else if (value.IsUint()) {
                behavior.properties[key] = value.GetUint();
            } else if (value.IsInt64()) {
                behavior.properties[key] = value.GetInt64();
            } else if (value.IsUint64()) {
                behavior.properties[key] = value.GetUint64();
            } else if (value.IsDouble()) {
                behavior.properties[key] = value.GetDouble();
            } else if (value.IsString()) {
                behavior.properties[key] = std::string(value.GetString());
            }
        }
    }
    
    // Parse numeric checks
    if (obj.HasMember("numericChecks") && obj["numericChecks"].IsObject()) {
        const auto& checks = obj["numericChecks"];
        for (auto it = checks.MemberBegin(); it != checks.MemberEnd(); ++it) {
            if (it->value.IsString()) {
                behavior.numericChecks[it->name.GetString()] = it->value.GetString();
            }
        }
    }
    
    // Parse error message contains
    if (obj.HasMember("errorMessageContains") && obj["errorMessageContains"].IsArray()) {
        const auto& messages = obj["errorMessageContains"];
        for (SizeType i = 0; i < messages.Size(); i++) {
            if (messages[i].IsString()) {
                behavior.errorMessageContains.push_back(messages[i].GetString());
            }
        }
    }
    
    return behavior;
}

std::unordered_map<std::string, std::any> JsonTestLoader::parseOptions(const void* jsonObj) {
    const Value& obj = *static_cast<const Value*>(jsonObj);
    std::unordered_map<std::string, std::any> options;
    
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        const std::string key = it->name.GetString();
        const auto& value = it->value;
        
        if (value.IsBool()) {
            options[key] = value.GetBool();
        } else if (value.IsInt()) {
            options[key] = value.GetInt();
        } else if (value.IsUint()) {
            options[key] = static_cast<size_t>(value.GetUint());
        } else if (value.IsInt64()) {
            options[key] = value.GetInt64();
        } else if (value.IsUint64()) {
            options[key] = static_cast<size_t>(value.GetUint64());
        } else if (value.IsDouble()) {
            options[key] = value.GetDouble();
        } else if (value.IsString()) {
            options[key] = std::string(value.GetString());
        } else if (value.IsObject()) {
            // For nested objects, store as string (JSON representation)
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            value.Accept(writer);
            options[key] = std::string(buffer.GetString());
        }
    }
    
    return options;
}

bool JsonTestLoader::saveTestResults(const std::string& filePath,
                                     const std::vector<TestTypeDefinition>& testTypes,
                                     const std::vector<std::vector<TestResult>>& results,
                                     const std::string& testCaseJsonPath) {
    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    // Calculate summary statistics
    int totalTests = 0;
    int passed = 0;
    int failed = 0;
    int testNotImplemented = 0;  // Test handler not implemented
    int engineFeatureNYI = 0;     // Engine feature not yet implemented (TodoOrDie)
    int skipped = 0;
    double totalTime = 0.0;
    
    for (size_t i = 0; i < testTypes.size(); i++) {
        const auto& testType = testTypes[i];
        const auto& typeResults = i < results.size() ? results[i] : std::vector<TestResult>();
        
        for (size_t j = 0; j < testType.variations.size(); j++) {
            const auto& result = j < typeResults.size() ? typeResults[j] : TestResult();
            totalTests++;
            
            // Check for test not implemented
            if (result.actualBehavior.find("Test Not Implemented") != std::string::npos ||
                result.failureReason.find("No test handler") != std::string::npos) {
                testNotImplemented++;
            } else if (result.actualBehavior.find("SKIP") != std::string::npos) {
                skipped++;
            } else if (result.passed) {
                passed++;
            } else {
                // Check for TodoOrDie logs to detect engine NYI
                bool hasTodoOrDie = false;
                for (const auto& log : result.logMessages) {
                    if (log.find("[TODO_OR_DIE]") != std::string::npos ||
                        log.find("[TODO!]") != std::string::npos) {
                        hasTodoOrDie = true;
                        break;
                    }
                }
                
                if (hasTodoOrDie) {
                    engineFeatureNYI++;
                } else {
                    failed++;
                }
            }
            
            // Add execution time (from actualProperties if available)
            if (result.actualProperties.count("executionTime")) {
                totalTime += std::any_cast<double>(result.actualProperties.at("executionTime"));
            }
        }
    }
    
    double passRate = totalTests > 0 ? (100.0 * passed / totalTests) : 0.0;
    
    // Add metadata with proper statistics
    Value metadata(kObjectType);
    
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
    
    metadata.AddMember("timestamp", Value().SetString(timeStr, allocator), allocator);
    metadata.AddMember("total_tests", totalTests, allocator);
    metadata.AddMember("passed", passed, allocator);
    metadata.AddMember("failed", failed, allocator);
    metadata.AddMember("skipped", skipped, allocator);
    metadata.AddMember("test_not_implemented", testNotImplemented, allocator);
    metadata.AddMember("engine_feature_nyi", engineFeatureNYI, allocator);
    metadata.AddMember("pass_rate", passRate, allocator);
    metadata.AddMember("total_time_ms", totalTime, allocator);
    
    // Add test case JSON path
    if (!testCaseJsonPath.empty()) {
        metadata.AddMember("test_case_json", Value().SetString(testCaseJsonPath.c_str(), allocator), allocator);
    }
    
    doc.AddMember("metadata", metadata, allocator);
    
    // Add detailed results
    Value resultsArray(kArrayType);
    
    for (size_t i = 0; i < testTypes.size(); i++) {
        const auto& testType = testTypes[i];
        const auto& typeResults = i < results.size() ? results[i] : std::vector<TestResult>();
        
        for (size_t j = 0; j < testType.variations.size(); j++) {
            const auto& variation = testType.variations[j];
            const auto& result = j < typeResults.size() ? typeResults[j] : TestResult();
            
            Value resultObj(kObjectType);
            
            // Format ID with leading zeros
            char idStr[10];
            std::snprintf(idStr, sizeof(idStr), "%03d", variation.id);
            resultObj.AddMember("id", Value().SetString(idStr, allocator), allocator);
            
            resultObj.AddMember("category", Value().SetString(testType.category.c_str(), allocator), allocator);
            resultObj.AddMember("test_type", Value().SetString(testType.testType.c_str(), allocator), allocator);
            
            // Build input string from options
            std::string inputStr = "type=" + testType.testType;
            for (const auto& [key, value] : variation.options) {
                inputStr += ", " + key + "=";
                try {
                    if (value.type() == typeid(std::string)) {
                        inputStr += std::any_cast<std::string>(value);
                    } else if (value.type() == typeid(int)) {
                        inputStr += std::to_string(std::any_cast<int>(value));
                    } else if (value.type() == typeid(size_t)) {
                        inputStr += std::to_string(std::any_cast<size_t>(value));
                    } else if (value.type() == typeid(double)) {
                        inputStr += std::to_string(std::any_cast<double>(value));
                    } else if (value.type() == typeid(bool)) {
                        inputStr += std::any_cast<bool>(value) ? "true" : "false";
                    }
                } catch (...) {
                    inputStr += "unknown";
                }
            }
            resultObj.AddMember("input", Value().SetString(inputStr.c_str(), allocator), allocator);
            
            // Expected result from variation
            std::string expectedResult = variation.expectedBehavior.returnValue;
            if (expectedResult.empty()) {
                expectedResult = variation.expectedBehavior.shouldSucceed ? "Success" : "Failure";
            }
            resultObj.AddMember("expected_result", Value().SetString(expectedResult.c_str(), allocator), allocator);
            
            // Actual result
            resultObj.AddMember("actual_result", Value().SetString(result.actualBehavior.c_str(), allocator), allocator);
            
            // Add expected callstack (empty for now)
            Value expectedCallstack(kArrayType);
            resultObj.AddMember("expected_callstack", expectedCallstack, allocator);
            
            // Add actual callstack (empty for now)
            Value actualCallstack(kArrayType);
            resultObj.AddMember("actual_callstack", actualCallstack, allocator);
            
            resultObj.AddMember("passed", result.passed, allocator);
            resultObj.AddMember("failure_reason", Value().SetString(result.failureReason.c_str(), allocator), allocator);
            
            // Add execution time
            double execTime = 0.0;
            if (result.actualProperties.count("executionTime")) {
                execTime = std::any_cast<double>(result.actualProperties.at("executionTime"));
            }
            resultObj.AddMember("execution_time_ms", execTime, allocator);
            resultObj.AddMember("timestamp", Value().SetString(timeStr, allocator), allocator);
            
            // Add log messages from test result
            Value logMessages(kArrayType);
            for (const auto& log : result.logMessages) {
                logMessages.PushBack(Value().SetString(log.c_str(), allocator), allocator);
            }
            resultObj.AddMember("log_messages", logMessages, allocator);
            
            resultsArray.PushBack(resultObj, allocator);
        }
    }
    
    doc.AddMember("results", resultsArray, allocator);
    
    // Write to file
    FILE* fp = std::fopen(filePath.c_str(), "wb");
    if (!fp) {
        return false;
    }
    
    char writeBuffer[65536];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    PrettyWriter<FileWriteStream> writer(os);
    doc.Accept(writer);
    std::fclose(fp);
    
    return true;
}

} // namespace pers::tests