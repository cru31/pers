# Log Capture Mechanism

## Overview

The log capture system is a critical component that enables per-test log collection in a thread-safe manner. It integrates with the Pers Logger system to capture all log messages generated during test execution and associate them with specific test cases.

## Architecture

### Component Interaction

```
┌─────────────────────────────────────────────────────────┐
│                    Test Thread                           │
│  ┌─────────────┐                  ┌─────────────────┐  │
│  │ Test Runner │──────────────────>│ Log Capture     │  │
│  └─────────────┘                  └────────┬────────┘  │
│         │                                   │           │
│         ▼                                   ▼           │
│  ┌─────────────┐    Log Callback    ┌─────────────┐   │
│  │ Test Case   │◄───────────────────│ Logger      │   │
│  │ Execution   │                    │ System      │   │
│  └─────────────┘                    └─────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## Implementation Details

### LogCapture Class

```cpp
class LogCapture {
private:
    std::vector<std::string> _capturedLogs;
    std::mutex _logMutex;
    std::thread::id _captureThreadId;
    bool _isCapturing;
    Logger::CallbackHandle _callbackHandle;
    
public:
    LogCapture() 
        : _isCapturing(false)
        , _captureThreadId(std::this_thread::get_id()) {}
    
    void startCapture();
    void stopCapture();
    std::vector<std::string> getCapturedLogs() const;
    
private:
    void onLogMessage(const LogInfo& info);
    std::string formatLogMessage(const LogInfo& info);
};
```

### Thread-Safe Capture

```cpp
void LogCapture::startCapture() {
    std::lock_guard<std::mutex> lock(_logMutex);
    
    if (_isCapturing) {
        return;  // Already capturing
    }
    
    _capturedLogs.clear();
    _isCapturing = true;
    
    // Register callback with logger
    _callbackHandle = Logger::getInstance().addCallback(
        [this](const LogInfo& info) {
            // Only capture logs from the test thread
            if (std::this_thread::get_id() == _captureThreadId) {
                this->onLogMessage(info);
            }
        }
    );
}

void LogCapture::onLogMessage(const LogInfo& info) {
    std::lock_guard<std::mutex> lock(_logMutex);
    
    if (!_isCapturing) return;
    
    std::string formatted = formatLogMessage(info);
    _capturedLogs.push_back(formatted);
}
```

## Log Format

### LogInfo Structure

```cpp
struct LogInfo {
    LogLevel level;           // TRACE, DEBUG, INFO, WARNING, ERROR, etc.
    std::string category;     // Component name (e.g., "WebGPUInstance")
    std::string message;      // Log message content
    LogSource source;         // Source location info
    std::chrono::time_point<> timestamp;
};

struct LogSource {
    const char* file;         // __FILE__ macro value
    int line;                 // __LINE__ macro value  
    const char* function;     // __FUNCTION__ macro value
};
```

### Formatted Output

The log capture system formats messages in a standardized format:

```
[LEVEL] Category: Message (function @ file:line)
```

Example:
```
[INFO] WebGPUInstance: Created successfully (initialize @ WebGPUInstance.cpp:105)
```

### Special Log Levels

#### TodoOrDie Detection

TodoOrDie logs are specially handled for NYI (Not Yet Implemented) detection:

```cpp
bool isNotYetImplemented(const std::string& logMessage) {
    return logMessage.find("[TODO_OR_DIE]") != std::string::npos ||
           logMessage.find("[TODO!]") != std::string::npos;
}
```

## Thread Safety Considerations

### Per-Thread Capture

Each test runs with its own log capture instance bound to the test thread:

```cpp
void runTest(const TestCase& test) {
    // Create capture for this thread
    LogCapture capture;
    
    // Start capturing
    capture.startCapture();
    
    // Run test (may spawn other threads)
    auto result = executeTest(test);
    
    // Stop capturing
    capture.stopCapture();
    
    // Get logs only from this thread
    result.logMessages = capture.getCapturedLogs();
}
```

### Multi-threaded Test Support

For tests that spawn additional threads:

```cpp
class MultiThreadLogCapture {
private:
    struct ThreadCapture {
        std::thread::id threadId;
        std::vector<std::string> logs;
        std::mutex mutex;
    };
    
    std::map<std::thread::id, ThreadCapture> _threadCaptures;
    std::mutex _mapMutex;
    
public:
    void captureFromThread(std::thread::id tid) {
        std::lock_guard<std::mutex> lock(_mapMutex);
        _threadCaptures[tid] = ThreadCapture{tid, {}, {}};
    }
    
    void onLog(const LogInfo& info) {
        auto tid = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(_mapMutex);
        
        if (auto it = _threadCaptures.find(tid); 
            it != _threadCaptures.end()) {
            std::lock_guard<std::mutex> threadLock(it->second.mutex);
            it->second.logs.push_back(formatLog(info));
        }
    }
};
```

## Integration with Logger System

### Logger Callback System

```cpp
class Logger {
public:
    using LogCallback = std::function<void(const LogInfo&)>;
    using CallbackHandle = size_t;
    
    CallbackHandle addCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        auto handle = _nextHandle++;
        _callbacks[handle] = std::move(callback);
        return handle;
    }
    
    void removeCallback(CallbackHandle handle) {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        _callbacks.erase(handle);
    }
    
private:
    void log(LogLevel level, const std::string& category, 
             const std::string& message, const LogSource& source) {
        LogInfo info{level, category, message, source, now()};
        
        // Notify all callbacks
        std::lock_guard<std::mutex> lock(_callbackMutex);
        for (const auto& [handle, callback] : _callbacks) {
            callback(info);
        }
    }
};
```

### TodoOrDie Integration

TodoOrDie macro integration for NYI detection:

```cpp
#define TODO_OR_DIE(context, message) \
    do { \
        LOG_TODO_OR_DIE(context, message); \
        if (pers::TodoOrDie::shouldDie()) { \
            pers::TodoOrDie::die(context, message, __FILE__, __LINE__); \
        } \
    } while(0)

// In test context, TodoOrDie logs but doesn't terminate
class TodoOrDie {
public:
    static bool shouldDie() {
        return !isInTestContext();
    }
    
    static bool isInTestContext() {
        return _testContextActive.load();
    }
    
private:
    static std::atomic<bool> _testContextActive;
};
```

## Performance Optimization

### String Formatting

Efficient log message formatting:

```cpp
class LogFormatter {
private:
    // Pre-allocated buffer to avoid allocations
    thread_local static char _buffer[4096];
    
public:
    static std::string format(const LogInfo& info) {
        // Use stack buffer for formatting
        int len = snprintf(_buffer, sizeof(_buffer),
            "[%s] %s: %s (%s @ %s:%d)",
            levelToString(info.level),
            info.category.c_str(),
            info.message.c_str(),
            info.source.function,
            basename(info.source.file),
            info.source.line
        );
        
        return std::string(_buffer, len);
    }
};
```

### Circular Buffer Option

For high-frequency logging:

```cpp
class CircularLogBuffer {
private:
    static constexpr size_t BUFFER_SIZE = 10000;
    std::array<std::string, BUFFER_SIZE> _buffer;
    std::atomic<size_t> _writeIndex{0};
    size_t _readIndex{0};
    
public:
    void push(std::string log) {
        size_t index = _writeIndex.fetch_add(1) % BUFFER_SIZE;
        _buffer[index] = std::move(log);
    }
    
    std::vector<std::string> drain() {
        std::vector<std::string> result;
        size_t endIndex = _writeIndex.load();
        
        while (_readIndex < endIndex) {
            result.push_back(_buffer[_readIndex % BUFFER_SIZE]);
            _readIndex++;
        }
        
        return result;
    }
};
```

## Memory Management

### Log Size Limits

Prevent memory exhaustion:

```cpp
class BoundedLogCapture {
private:
    static constexpr size_t MAX_LOGS = 1000;
    static constexpr size_t MAX_LOG_LENGTH = 1024;
    
    std::deque<std::string> _logs;
    
public:
    void addLog(std::string log) {
        // Truncate if too long
        if (log.length() > MAX_LOG_LENGTH) {
            log.resize(MAX_LOG_LENGTH);
            log += "... (truncated)";
        }
        
        // Maintain size limit
        if (_logs.size() >= MAX_LOGS) {
            _logs.pop_front();
        }
        
        _logs.push_back(std::move(log));
    }
};
```

## Error Handling

### Callback Exception Safety

```cpp
void Logger::notifyCallbacks(const LogInfo& info) {
    std::lock_guard<std::mutex> lock(_callbackMutex);
    
    for (const auto& [handle, callback] : _callbacks) {
        try {
            callback(info);
        } catch (const std::exception& e) {
            // Log callback error without triggering callbacks
            directLog(LogLevel::ERROR, "Logger", 
                     std::string("Callback error: ") + e.what());
        } catch (...) {
            directLog(LogLevel::ERROR, "Logger", 
                     "Unknown callback error");
        }
    }
}
```

### RAII Cleanup

Ensure proper cleanup:

```cpp
class ScopedLogCapture {
private:
    LogCapture _capture;
    bool _started;
    
public:
    ScopedLogCapture() : _started(false) {}
    
    ~ScopedLogCapture() {
        if (_started) {
            _capture.stopCapture();
        }
    }
    
    void start() {
        _capture.startCapture();
        _started = true;
    }
    
    std::vector<std::string> getLogs() {
        if (_started) {
            _capture.stopCapture();
            _started = false;
        }
        return _capture.getCapturedLogs();
    }
};
```

## Testing the Log Capture

### Unit Tests

```cpp
TEST(LogCapture, CapturesLogsFromCurrentThread) {
    LogCapture capture;
    capture.startCapture();
    
    LOG_INFO("Test", "Test message");
    LOG_WARNING("Test", "Warning message");
    
    capture.stopCapture();
    auto logs = capture.getCapturedLogs();
    
    EXPECT_EQ(logs.size(), 2);
    EXPECT_TRUE(logs[0].find("[INFO]") != std::string::npos);
    EXPECT_TRUE(logs[1].find("[WARNING]") != std::string::npos);
}

TEST(LogCapture, IgnoresLogsFromOtherThreads) {
    LogCapture capture;
    capture.startCapture();
    
    std::thread other([]{
        LOG_INFO("Other", "Should not capture");
    });
    other.join();
    
    capture.stopCapture();
    auto logs = capture.getCapturedLogs();
    
    EXPECT_EQ(logs.size(), 0);
}
```

## Best Practices

1. **Start/Stop Pairing**: Always pair startCapture() with stopCapture()
2. **Thread Affinity**: Create capture instances in the thread that will generate logs
3. **Exception Safety**: Use RAII wrappers for automatic cleanup
4. **Performance**: Consider log level filtering for performance-critical tests
5. **Memory**: Set reasonable limits on captured log count and size