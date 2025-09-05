#pragma once

#include "test_variation.h"
#include <memory>
#include <unordered_map>
#include <pers/utils/Logger.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace pers::tests {

// Base interface for all test handlers
class ITestHandler {
public:
    virtual ~ITestHandler() = default;
    
    // Execute test with given variation
    virtual TestResult execute(const TestVariation& variation) = 0;
    
    // Get test type this handler handles
    virtual std::string getTestType() const = 0;
};

// Base class with common functionality
class TestHandlerBase : public ITestHandler {
protected:
    std::vector<LogEntry> _capturedLogs;
    
    void setupLogCapture() {
        _capturedLogs.clear();
        
        auto captureCallback = [this](pers::LogLevel level, const std::string& category, 
                                      const std::string& message, const pers::LogSource& source,
                                      const std::chrono::system_clock::time_point& timestamp,
                                      bool& skipLogging) {
            // Create structured log entry
            LogEntry entry;
            
            const char* levelStr = "UNKNOWN";
            switch (level) {
                case pers::LogLevel::Trace: levelStr = "TRACE"; break;
                case pers::LogLevel::Debug: levelStr = "DEBUG"; break;
                case pers::LogLevel::Info: levelStr = "INFO"; break;
                case pers::LogLevel::TodoSomeday: levelStr = "TODO_SOMEDAY"; break;
                case pers::LogLevel::Warning: levelStr = "WARNING"; break;
                case pers::LogLevel::TodoOrDie: levelStr = "TODO_OR_DIE"; break;
                case pers::LogLevel::Error: levelStr = "ERROR"; break;
                case pers::LogLevel::Critical: levelStr = "CRITICAL"; break;
            }
            
            // Format timestamp
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()).count() % 1000;
            std::tm tm_buf;
#ifdef _WIN32
            localtime_s(&tm_buf, &time_t);
#else
            localtime_r(&time_t, &tm_buf);
#endif
            
            // Format: HH:MM:SS.mmm
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm_buf);
            std::stringstream ss;
            ss << timeStr << "." << std::setfill('0') << std::setw(3) << millis;
            
            entry.timestamp = ss.str();
            entry.level = levelStr;
            entry.category = category;
            entry.message = message;
            entry.file = source.file ? source.file : "";
            entry.line = source.line;
            entry.function = source.function ? source.function : "";
            
            _capturedLogs.push_back(entry);
            skipLogging = false;
        };
        
        // Register callbacks
        pers::Logger::Instance().setCallback(pers::LogLevel::Trace, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::Debug, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::Info, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoSomeday, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::Warning, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::Error, captureCallback);
        pers::Logger::Instance().setCallback(pers::LogLevel::Critical, captureCallback);
    }
    
    void clearLogCallbacks() {
        pers::Logger::Instance().setCallback(pers::LogLevel::Trace, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::Debug, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::Info, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoSomeday, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::Warning, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::Error, nullptr);
        pers::Logger::Instance().setCallback(pers::LogLevel::Critical, nullptr);
    }
    
    void transferLogsToResult(TestResult& result) {
        clearLogCallbacks();
        result.logMessages = _capturedLogs;
    }
    
public:
    virtual ~TestHandlerBase() = default;
};

// Registry for test handlers
class TestHandlerRegistry {
public:
    static TestHandlerRegistry& Instance() {
        static TestHandlerRegistry instance;
        return instance;
    }
    
    void registerHandler(const std::string& testType, std::shared_ptr<ITestHandler> handler) {
        _handlers[testType] = handler;
    }
    
    std::shared_ptr<ITestHandler> getHandler(const std::string& testType) const {
        auto it = _handlers.find(testType);
        if (it != _handlers.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool hasHandler(const std::string& testType) const {
        return _handlers.find(testType) != _handlers.end();
    }
    
    std::vector<std::string> getAllTestTypes() const {
        std::vector<std::string> types;
        for (const auto& [type, handler] : _handlers) {
            types.push_back(type);
        }
        return types;
    }
    
    void clear() {
        _handlers.clear();
    }
    
private:
    TestHandlerRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<ITestHandler>> _handlers;
};

// Simple registration helper - no macros, no templates
class TestHandlerAutoRegistrar {
public:
    TestHandlerAutoRegistrar(const std::string& testType, std::shared_ptr<ITestHandler> handler) {
        TestHandlerRegistry::Instance().registerHandler(testType, handler);
    }
};

} // namespace pers::tests