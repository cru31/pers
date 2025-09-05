#include "pers/utils/Logger.h"
#include "pers/utils/Mutex.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <map>

// Platform detection check
#if defined(__APPLE__)
    #if defined(_WIN32)
        #error "CRITICAL: _WIN32 is defined on macOS! This should never happen. Check your build configuration."
    #endif
    #if defined(_MSC_VER)
        #error "CRITICAL: _MSC_VER is defined on macOS! This indicates MSVC compiler on macOS which is impossible."
    #endif
#elif defined(__linux__)
    #if defined(_WIN32)
        #error "CRITICAL: _WIN32 is defined on Linux! This should never happen. Check your build configuration."
    #endif
    #if defined(_MSC_VER)
        #error "CRITICAL: _MSC_VER is defined on Linux! This indicates MSVC compiler on Linux which is impossible."
    #endif
#endif

namespace pers {

// ConsoleOutput implementation
ConsoleOutput::ConsoleOutput(bool useColors) 
    : useColors(useColors) {
}

void ConsoleOutput::Write(const LogEntry& entry) {
    std::ostream& out = (entry.level >= LogLevel::TodoOrDie) ? std::cerr : std::cout;
    
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    out << "[" << std::put_time(&tm_buf, "%H:%M:%S") << "] ";
    
    switch (entry.level) {
        case LogLevel::Trace:       out << "[TRACE] "; break;
        case LogLevel::Debug:       out << "[DEBUG] "; break;
        case LogLevel::Info:        out << "[INFO ] "; break;
        case LogLevel::TodoSomeday: out << "[TODO ] "; break;
        case LogLevel::Warning:     out << "[WARN ] "; break;
        case LogLevel::TodoOrDie:   out << "[TODO!] "; break;
        case LogLevel::Error:       out << "[ERROR] "; break;
        case LogLevel::Critical:    out << "[FATAL] "; break;
    }
    
    if (!entry.category.empty()) {
        out << "[" << entry.category << "] ";
    }
    
    // Add file and line information if available
    if (!entry.file.empty() && entry.line > 0) {
        // Extract just the filename from the full path
        size_t lastSlash = entry.file.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? 
            entry.file.substr(lastSlash + 1) : entry.file;
        out << "[" << filename << ":" << entry.line << "] ";
    }
    
    out << entry.message << std::endl;
}

void ConsoleOutput::Flush() {
    std::cout.flush();
}

// FileOutput implementation
FileOutput::FileOutput(const std::string& filename, bool append)
    : _file(filename, append ? std::ios::app : std::ios::out),
      _mutex("FileOutput") {
    if (!_file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

FileOutput::~FileOutput() = default;

void FileOutput::Write(const LogEntry& entry) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);

    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    _file << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] ";
    
    switch (entry.level) {
        case LogLevel::Trace:       _file << "[TRACE] "; break;
        case LogLevel::Debug:       _file << "[DEBUG] "; break;
        case LogLevel::Info:        _file << "[INFO ] "; break;
        case LogLevel::TodoSomeday: _file << "[TODO ] "; break;
        case LogLevel::Warning:     _file << "[WARN ] "; break;
        case LogLevel::TodoOrDie:   _file << "[TODO!] "; break;
        case LogLevel::Error:       _file << "[ERROR] "; break;
        case LogLevel::Critical:    _file << "[FATAL] "; break;
    }
    
    if (!entry.category.empty()) {
        _file << "[" << entry.category << "] ";
    }
    
    _file << entry.message << std::endl;
}

void FileOutput::Flush() {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _file.flush();
}

// Logger implementation
Logger::Logger() 
    : _minLevel(LogLevel::Trace),
      _mutex("Logger") {
    // Enable all log levels by default
    for (int i = static_cast<int>(LogLevel::Trace); i <= static_cast<int>(LogLevel::Critical); ++i) {
        _enabledLevels[static_cast<LogLevel>(i)] = true;
    }
    // Add basic ConsoleOutput
    AddOutput(std::make_shared<ConsoleOutput>(true));
}

Logger::~Logger() = default;

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::setCallback(LogLevel level, const LogCallback& callback) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _callbacks[level] = callback;
}

void Logger::clearCallback(LogLevel level) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _callbacks.erase(level);
}

void Logger::clearAllCallbacks() {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _callbacks.clear();
}

void Logger::AddOutput(const std::shared_ptr<ILogOutput>& output) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _outputs.push_back(output);
}

void Logger::RemoveAllOutputs() {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _outputs.clear();
}

void Logger::SetMinLevel(LogLevel level) {
    _minLevel = level;
}

LogLevel Logger::GetMinLevel() const {
    return _minLevel;
}

void Logger::SetLogLevelEnabled(LogLevel level, bool enabled) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _enabledLevels[level] = enabled;
}

bool Logger::IsLogLevelEnabled(LogLevel level) const {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    auto it = _enabledLevels.find(level);
    return it != _enabledLevels.end() ? it->second : true;
}

void Logger::SetCategoryFilter(const std::string& pattern) {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    _categoryFilter = pattern;
}

void Logger::LogInternal(const LogEntry& entry, const LogSource& source) {
    static thread_local int callbackDepth = 0;
    
    if (callbackDepth > 0) {
        std::cerr << "[LOGGER] Recursive logging detected (depth=" << callbackDepth 
                 << "): " << entry.message << std::endl;
        return;
    }
    
    if (entry.level < _minLevel) {
        return;
    }
    
    // Check if this log level is enabled
    {
        auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
        auto it = _enabledLevels.find(entry.level);
        if (it != _enabledLevels.end() && !it->second) {
            return;
        }
    }
    
    bool skipLogging = false;
    
    // Check for callback
    {
        auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
        auto callbackIt = _callbacks.find(entry.level);
        if (callbackIt != _callbacks.end() && callbackIt->second) {
            ++callbackDepth;
            callbackIt->second(entry.level, entry.category, entry.message, source, skipLogging);
            --callbackDepth;
        }
    }
    
    // If callback requested to skip logging, return
    if (skipLogging) {
        return;
    }
    
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);

    // Category filter check
    if (!_categoryFilter.empty() && entry.category.find(_categoryFilter) == std::string::npos) {
        return;
    }
    
    // Write to all outputs
    for (auto& output : _outputs) {
        output->Write(entry);
    }
}

void Logger::Log(LogLevel level,
                 const std::string& category,
                 const std::string& message,
                 const LogSource& source) {
    LogEntry entry;
    entry.level = level;
    entry.timestamp = std::chrono::system_clock::now();
    entry.category = category;
    entry.message = message;
    entry.file = source.file ? source.file : "";
    entry.line = source.line;
    entry.function = source.function ? source.function : "";
    entry.threadId = std::this_thread::get_id();
    
    LogInternal(entry, source);
}

void Logger::Flush() {
    auto guard = makeLockGuard(_mutex, PERS_SOURCE_LOC);
    for (auto& output : _outputs) {
        output->Flush();
    }
}

// LogStream implementation
LogStream::LogStream(LogLevel level, const std::string& category,
                     const LogSource& source)
    : level(level), category(category), source(source) {
}

LogStream::~LogStream() {
    Logger::Instance().Log(level, category, stream.str(), source);
}

} // namespace pers