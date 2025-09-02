#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace pers {

// Macro to capture source location information
#define PERS_SOURCE_LOC pers::LogSource{__FILE__, __LINE__, __FUNCTION__}

// Logger macros for easy usage
#define LOG_TRACE(category, message) pers::Logger::Instance().Log(pers::LogLevel::Trace, category, message, PERS_SOURCE_LOC)
#define LOG_DEBUG(category, message) pers::Logger::Instance().Log(pers::LogLevel::Debug, category, message, PERS_SOURCE_LOC)
#define LOG_INFO(category, message) pers::Logger::Instance().Log(pers::LogLevel::Info, category, message, PERS_SOURCE_LOC)
#define LOG_WARNING(category, message) pers::Logger::Instance().Log(pers::LogLevel::Warning, category, message, PERS_SOURCE_LOC)
#define LOG_ERROR(category, message) pers::Logger::Instance().Log(pers::LogLevel::Error, category, message, PERS_SOURCE_LOC)
#define LOG_CRITICAL(category, message) pers::Logger::Instance().Log(pers::LogLevel::Critical, category, message, PERS_SOURCE_LOC)
#define LOG_TODO_SOMEDAY(category, message) pers::Logger::Instance().Log(pers::LogLevel::TodoSomeday, category, message, PERS_SOURCE_LOC)
#define LOG_TODO_OR_DIE(category, message) pers::Logger::Instance().Log(pers::LogLevel::TodoOrDie, category, message, PERS_SOURCE_LOC)

// Compatibility macros
#define TODO_OR_DIE(functionName, todoDescription) LOG_TODO_OR_DIE(functionName, todoDescription)
#define TODO_SOMEDAY(functionName, todoDescription) LOG_TODO_SOMEDAY(functionName, todoDescription)

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    TodoSomeday = 3,  // Non-critical future improvements
    Warning = 4,
    TodoOrDie = 5,    // Critical TODOs that must be implemented
    Error = 6,
    Critical = 7
};

struct LogEntry {
    LogLevel level;
    std::chrono::system_clock::time_point timestamp;
    std::string category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::thread::id threadId;
};

class ILogOutput {
public:
    virtual ~ILogOutput() = default;
    virtual void Write(const LogEntry& entry) = 0;
    virtual void Flush() = 0;
};

class ConsoleOutput : public ILogOutput {
public:
    explicit ConsoleOutput(bool useColors = true);
    void Write(const LogEntry& entry) override;
    void Flush() override;
    
private:
    bool useColors;
};

class FileOutput : public ILogOutput {
public:
    explicit FileOutput(const std::string& filename, bool append = true);
    ~FileOutput();
    void Write(const LogEntry& entry) override;
    void Flush() override;
    
private:
    class Impl;
    std::shared_ptr<Impl> impl;
};

// Source location info for logging
struct LogSource {
    const char* file;
    int line;
    const char* function;
};

class Logger {
public:
    // Callback type for log events
    using LogCallback = std::function<void(LogLevel level,
                                          const std::string& category,
                                          const std::string& message,
                                          const LogSource& source,
                                          bool& skipLogging)>;
    
    static Logger& Instance();
    
    // Callback management
    void setCallback(LogLevel level, const LogCallback& callback);
    void clearCallback(LogLevel level);
    void clearAllCallbacks();
    
    void AddOutput(const std::shared_ptr<ILogOutput>& output);
    void RemoveAllOutputs();
    
    void SetMinLevel(LogLevel level);
    LogLevel GetMinLevel() const;
    
    // Enable/disable specific log levels
    void SetLogLevelEnabled(LogLevel level, bool enabled);
    bool IsLogLevelEnabled(LogLevel level) const;
    
    // Convenience methods to quickly toggle common levels
    void EnableInfoLogs(bool enable) { SetLogLevelEnabled(LogLevel::Info, enable); }
    void EnableDebugLogs(bool enable) { SetLogLevelEnabled(LogLevel::Debug, enable); }
    void EnableTraceLogs(bool enable) { SetLogLevelEnabled(LogLevel::Trace, enable); }
    
    void SetCategoryFilter(const std::string& pattern);
    
    void Log(LogLevel level,
             const std::string& category,
             const std::string& message,
             const LogSource& source);
    template<typename... Args>
    void LogFormat(LogLevel level,
                   const std::string& category,
                   const LogSource& source,
                   const char* format,
                   Args... args) {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), format, args...);
        Log(level, category, buffer, source);
    }
    
    void Flush();
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    class Impl;
    std::shared_ptr<Impl> impl;
};

class LogStream {
public:
    LogStream(LogLevel level, const std::string& category, 
              const LogSource& source);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream << value;
        return *this;
    }
    
private:
    LogLevel level;
    std::string category;
    LogSource source;
    std::ostringstream stream;
};

} // namespace pers