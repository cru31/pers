#pragma once

#include "pers/utils/Logger.h"
#include <mutex>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <string>

namespace pers {

class Mutex {
private:
    mutable std::mutex _mutex;
    const char* _name;
    bool _enableLogging;
    static std::atomic<int> _globalLockId;
    
    // Helper to extract filename from path
    static const char* getFileName(const char* path) {
        const char* file = path;
        const char* lastSlash = nullptr;
        while (*path) {
            if (*path == '/' || *path == '\\') {
                lastSlash = path;
            }
            ++path;
        }
        return lastSlash ? lastSlash + 1 : file;
    }
    
public:
    explicit Mutex(const char* name = "unnamed", bool enableLogging = false) 
        : _name(name), _enableLogging(enableLogging) {}
    
    void lock(const LogSource& loc = {nullptr, 0, nullptr}) {
        int lockId = 0;
        if (_enableLogging) {
            lockId = ++_globalLockId;
        }
        
        if (_enableLogging && loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << lockId 
                      << " | " << std::left << std::setw(20) << "Attempting to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else if (_enableLogging) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << lockId 
                      << " | " << std::left << std::setw(20) << "Attempting to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
        
        _mutex.lock();
        
        if (_enableLogging && loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << lockId 
                      << " | " << std::left << std::setw(20) << "Acquired lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else if (_enableLogging) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << lockId 
                      << " | " << std::left << std::setw(20) << "Acquired lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
    }
    
    void unlock(const LogSource& loc = {nullptr, 0, nullptr}) {
        if (_enableLogging && loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _globalLockId 
                      << " | " << std::left << std::setw(20) << "Releasing lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else if (_enableLogging) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _globalLockId 
                      << " | " << std::left << std::setw(20) << "Releasing lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
        _mutex.unlock();
    }
    
    bool tryLock(const LogSource& loc = {nullptr, 0, nullptr}) {
        if (_enableLogging && loc.file) {
            std::cerr << "[MUTEX] " << getFileName(loc.file) << ":" << loc.line 
                      << " - trying to lock '" << _name << "'" << std::endl;
        } else if (_enableLogging) {
            std::cerr << "[MUTEX] Thread " << std::this_thread::get_id() 
                      << " trying to lock '" << _name << "'" << std::endl;
        }
        
        bool acquired = _mutex.try_lock();
        
        if (acquired) {
            if (_enableLogging && loc.file) {
                std::cerr << "[MUTEX] " << getFileName(loc.file) << ":" << loc.line 
                          << " - acquired lock (try_lock)" << std::endl;
            } else if (_enableLogging) {
                std::cerr << "[MUTEX] Thread " << std::this_thread::get_id() 
                          << " acquired lock '" << _name << "' (try_lock)" << std::endl;
            }
        } else {
            if (_enableLogging && loc.file) {
                std::cerr << "[MUTEX] " << getFileName(loc.file) << ":" << loc.line 
                          << " - failed to lock (try_lock)" << std::endl;
            } else if (_enableLogging) {
                std::cerr << "[MUTEX] Thread " << std::this_thread::get_id() 
                          << " failed to lock '" << _name << "' (try_lock)" << std::endl;
            }
        }
        
        return acquired;
    }
    
    // Standard lock/unlock for compatibility with std::lock_guard
    void lock() {
        lock({nullptr, 0, nullptr});
    }
    
    void unlock() {
        unlock({nullptr, 0, nullptr});
    }
    
    bool try_lock() {
        return tryLock({nullptr, 0, nullptr});
    }
    
    // Enable/disable logging at runtime
    void setLoggingEnabled(bool enabled) {
        _enableLogging = enabled;
    }
};

// Template lock guard that can optionally track location
template<bool TrackLocation = false>
class LockGuard {
private:
    Mutex& _mutex;
    LogSource _loc;
    
public:
    LockGuard(Mutex& mutex, const char* file = nullptr, int line = 0, const char* function = nullptr) 
        : _mutex(mutex), _loc{file, line, function} {
        if constexpr (TrackLocation) {
            _mutex.lock(_loc);
        } else {
            _mutex.lock();
        }
    }
    
    ~LockGuard() {
        if constexpr (TrackLocation) {
            _mutex.unlock(_loc);
        } else {
            _mutex.unlock();
        }
    }
    
    // Delete copy operations
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

// Convenience function for creating tracked lock guard
inline auto makeLockGuard(Mutex& mutex, const LogSource& loc) {
    return LockGuard<true>(mutex, loc.file, loc.line, loc.function);
}

} // namespace pers