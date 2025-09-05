#include "pers/utils/Logger.h"
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
class FileOutput::Impl {
public:
    explicit Impl(const std::string& filename, bool append)
        : file(filename, append ? std::ios::app : std::ios::out) {
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
    
    void Write(const LogEntry& entry) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (FileOutput::Write)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t);
#else
        localtime_r(&time_t, &tm_buf);
#endif
        
        file << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] ";
        
        switch (entry.level) {
            case LogLevel::Trace:       file << "[TRACE] "; break;
            case LogLevel::Debug:       file << "[DEBUG] "; break;
            case LogLevel::Info:        file << "[INFO ] "; break;
            case LogLevel::TodoSomeday: file << "[TODO ] "; break;
            case LogLevel::Warning:     file << "[WARN ] "; break;
            case LogLevel::TodoOrDie:   file << "[TODO!] "; break;
            case LogLevel::Error:       file << "[ERROR] "; break;
            case LogLevel::Critical:    file << "[FATAL] "; break;
        }
        
        if (!entry.category.empty()) {
            file << "[" << entry.category << "] ";
        }
        
        file << entry.message << std::endl;
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (FileOutput::Write)" << std::endl;
    }
    
    void Flush() {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (FileOutput::Flush)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        file.flush();
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (FileOutput::Flush)" << std::endl;
    }
    
private:
    std::ofstream file;
    std::mutex mutex;
};

FileOutput::FileOutput(const std::string& filename, bool append)
    : impl(std::make_unique<Impl>(filename, append)) {
}

FileOutput::~FileOutput() = default;

void FileOutput::Write(const LogEntry& entry) {
    impl->Write(entry);
}

void FileOutput::Flush() {
    impl->Flush();
}

// Logger implementation
class Logger::Impl {
public:
    Impl() : minLevel(LogLevel::Trace) {
        // Enable all log levels by default
        for (int i = static_cast<int>(LogLevel::Trace); i <= static_cast<int>(LogLevel::Critical); ++i) {
            enabledLevels[static_cast<LogLevel>(i)] = true;
        }
    }
    
    void setCallback(LogLevel level, const Logger::LogCallback& callback) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (setCallback)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        callbacks[level] = callback;
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (setCallback)" << std::endl;
    }
    
    void clearCallback(LogLevel level) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (clearCallback)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.erase(level);
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (clearCallback)" << std::endl;
    }
    
    void clearAllCallbacks() {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (clearAllCallbacks)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.clear();
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (clearAllCallbacks)" << std::endl;
    }
    
    void AddOutput(const std::shared_ptr<ILogOutput>& output) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (AddOutput)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        outputs.push_back(output);
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (AddOutput)" << std::endl;
    }
    
    void RemoveAllOutputs() {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (RemoveAllOutputs)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        outputs.clear();
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (RemoveAllOutputs)" << std::endl;
    }
    
    void SetMinLevel(LogLevel level) {
        minLevel = level;
    }
    
    LogLevel GetMinLevel() const {
        return minLevel;
    }
    
    void SetLogLevelEnabled(LogLevel level, bool enabled) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (SetLogLevelEnabled)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        enabledLevels[level] = enabled;
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (SetLogLevelEnabled)" << std::endl;
    }
    
    bool IsLogLevelEnabled(LogLevel level) const {
        static thread_local int mutexLockCount = 0;  // mutex 진입 횟수
        std::cerr << "[MUTEX] Entering lock #" << mutexLockCount << " (IsLogLevelEnabled)" << std::endl;

        bool ret = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = enabledLevels.find(level);
            ret = it != enabledLevels.end() ? it->second : true;
            std::cerr << "[MUTEX] Exiting lock #" << mutexLockCount << " (IsLogLevelEnabled)" << std::endl;

            mutexLockCount++;
        }
        return ret;
    }
    
    void SetCategoryFilter(const std::string& pattern) {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (SetCategoryFilter)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        categoryFilter = pattern;
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (SetCategoryFilter)" << std::endl;
    }
    
    void Log(const LogEntry& entry, const LogSource& source) {
        static thread_local int callbackDepth = 0;
        static thread_local int mutexLockCount = 0;  // mutex 진입 횟수
        
        if (callbackDepth > 0) {
            std::cerr << "[LOGGER] Recursive logging detected (depth=" << callbackDepth 
                     << ", next_lockId_would_be=" << (mutexLockCount + 1)
                     << "): " << entry.message << std::endl;
            return;
        }
        
        if (entry.level < minLevel) {
            return;
        }
        
        // Check if this log level is enabled
        {
            int lockId = ++mutexLockCount;  // mutex 진입 ID: 1
            std::cerr << "[MUTEX] Entering lock #" << lockId << " (check level)" << std::endl;
            std::lock_guard<std::mutex> lock(mutex);
            auto it = enabledLevels.find(entry.level);
            if (it != enabledLevels.end() && !it->second) {
                std::cerr << "[MUTEX] Exiting lock #" << lockId << " (check level)" << std::endl;
                return;
            }

            std::cerr << "[MUTEX] Exiting lock #" << lockId << " (check level)" << std::endl;
        }
        
        bool skipLogging = false;
        
        // Check for callback
        {
            int lockId = ++mutexLockCount;  // mutex 진입 ID: 2
            std::cerr << "[MUTEX] Entering lock #" << lockId << " (callback)" << std::endl;
            std::lock_guard<std::mutex> lock(mutex);
            auto callbackIt = callbacks.find(entry.level);
            if (callbackIt != callbacks.end() && callbackIt->second) {
                ++callbackDepth;
                callbackIt->second(entry.level, entry.category, entry.message, source, skipLogging);
                --callbackDepth;
            }

            std::cerr << "[MUTEX] Exiting lock #" << lockId << " (callback)" << std::endl;
        }
        
        // If callback requested to skip logging, return
        if (skipLogging) {
            return;
        }
        
        int lockId = ++mutexLockCount;  // mutex 진입 ID: 3
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (write output)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        
        // Category filter check
        if (!categoryFilter.empty() && entry.category.find(categoryFilter) == std::string::npos) {

            std::cerr << "[MUTEX] Exiting lock #" << lockId << " (write output)" << std::endl;
            return;
        }
        
        // Write to all outputs
        for (auto& output : outputs) {
            output->Write(entry);
        }

        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (write output)" << std::endl;
    }
    
    void Flush() {
        static thread_local int mutexLockCount = 0;
        int lockId = ++mutexLockCount;
        std::cerr << "[MUTEX] Entering lock #" << lockId << " (Flush)" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& output : outputs) {
            output->Flush();
        }
        std::cerr << "[MUTEX] Exiting lock #" << lockId << " (Flush)" << std::endl;
    }
    
private:
    std::vector<std::shared_ptr<ILogOutput>> outputs;
    std::atomic<LogLevel> minLevel;
    std::string categoryFilter;
    std::map<LogLevel, bool> enabledLevels;
    std::map<LogLevel, Logger::LogCallback> callbacks;
    mutable std::mutex mutex;
};

Logger::Logger() : impl(std::make_unique<Impl>()) {
    // added basic ConsoleOutput
    AddOutput(std::make_shared<ConsoleOutput>(true));
}

Logger::~Logger() = default;

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::AddOutput(const std::shared_ptr<ILogOutput>& output) {
    impl->AddOutput(output);
}

void Logger::RemoveAllOutputs() {
    impl->RemoveAllOutputs();
}

void Logger::SetMinLevel(LogLevel level) {
    impl->SetMinLevel(level);
}

LogLevel Logger::GetMinLevel() const {
    return impl->GetMinLevel();
}

void Logger::SetCategoryFilter(const std::string& pattern) {
    impl->SetCategoryFilter(pattern);
}

void Logger::SetLogLevelEnabled(LogLevel level, bool enabled) {
    impl->SetLogLevelEnabled(level, enabled);
}

bool Logger::IsLogLevelEnabled(LogLevel level) const {
    return impl->IsLogLevelEnabled(level);
}

void Logger::setCallback(LogLevel level, const LogCallback& callback) {
    impl->setCallback(level, callback);
}

void Logger::clearCallback(LogLevel level) {
    impl->clearCallback(level);
}

void Logger::clearAllCallbacks() {
    impl->clearAllCallbacks();
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
    
    impl->Log(entry, source);
}

void Logger::Flush() {
    impl->Flush();
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