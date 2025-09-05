#pragma once

#include "pers/utils/SourceLocation.h"
#include <mutex>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <string>

namespace pers {

// Forward declaration
template<bool DebuggingEnabled>
class Mutex;

namespace detail {
    // Helper to extract filename from path
    inline const char* getFileName(const char* path) {
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
}

// Template specialization for non-debug mode (just wraps std::mutex)
template<>
class Mutex<false> {
private:
    mutable std::mutex _mutex;
    
public:
    explicit Mutex(const char* /*name*/ = nullptr) {}
    
    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }
    bool try_lock() { return _mutex.try_lock(); }
    
    // Overloads that ignore LogSource when debugging is disabled
    void lock(const LogSource&) { _mutex.lock(); }
    void unlock(const LogSource&) { _mutex.unlock(); }
    bool tryLock(const LogSource&) { return _mutex.try_lock(); }
};

// Template specialization for debug mode (includes logging)
template<>
class Mutex<true> {
private:
    mutable std::mutex _mutex;
    const char* _name;
    static std::atomic<int> _globalLockId;
    mutable int _currentLockId{0};
    
public:
    explicit Mutex(const char* name = "unnamed") 
        : _name(name) {}
    
    void lock(const LogSource& loc = {nullptr, 0, nullptr}) {
        _currentLockId = ++_globalLockId;
        
        if (loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Attempting to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Attempting to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
        
        _mutex.lock();
        
        if (loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Acquired lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Acquired lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
    }
    
    void unlock(const LogSource& loc = {nullptr, 0, nullptr}) {
        if (loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Releasing lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Releasing lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
        _mutex.unlock();
    }
    
    bool tryLock(const LogSource& loc = {nullptr, 0, nullptr}) {
        _currentLockId = ++_globalLockId;
        
        if (loc.file) {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Trying to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() 
                      << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                      << " (" << loc.function << ")" << std::endl;
        } else {
            std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                      << " | " << std::left << std::setw(20) << "Trying to lock" 
                      << " '" << _name 
                      << "', Thread:" << std::this_thread::get_id() << std::endl;
        }
        
        bool acquired = _mutex.try_lock();
        
        if (acquired) {
            if (loc.file) {
                std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                          << " | " << std::left << std::setw(20) << "Acquired lock (try)" 
                          << " '" << _name 
                          << "', Thread:" << std::this_thread::get_id() 
                          << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                          << " (" << loc.function << ")" << std::endl;
            } else {
                std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                          << " | " << std::left << std::setw(20) << "Acquired lock (try)" 
                          << " '" << _name 
                          << "', Thread:" << std::this_thread::get_id() << std::endl;
            }
        } else {
            if (loc.file) {
                std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                          << " | " << std::left << std::setw(20) << "Failed to lock" 
                          << " '" << _name 
                          << "', Thread:" << std::this_thread::get_id() 
                          << ", " << detail::getFileName(loc.file) << ":" << loc.line 
                          << " (" << loc.function << ")" << std::endl;
            } else {
                std::cerr << "[MUTEX] lock:" << std::setw(6) << std::setfill(' ') << _currentLockId 
                          << " | " << std::left << std::setw(20) << "Failed to lock" 
                          << " '" << _name 
                          << "', Thread:" << std::this_thread::get_id() << std::endl;
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
};

// Template lock guard that works with both debug and non-debug Mutex
template<bool DebuggingEnabled>
class LockGuard {
private:
    Mutex<DebuggingEnabled>& _mutex;
    LogSource _loc;
    
public:
    // Constructor for debug mode with location tracking
    LockGuard(Mutex<DebuggingEnabled>& mutex, const LogSource& loc) 
        : _mutex(mutex), _loc(loc) {
        if constexpr (DebuggingEnabled) {
            _mutex.lock(_loc);
        } else {
            _mutex.lock();
        }
    }
    
    // Constructor for non-debug mode or when no location is provided
    explicit LockGuard(Mutex<DebuggingEnabled>& mutex) 
        : _mutex(mutex), _loc{nullptr, 0, nullptr} {
        _mutex.lock();
    }
    
    ~LockGuard() {
        if constexpr (DebuggingEnabled) {
            _mutex.unlock(_loc);
        } else {
            _mutex.unlock();
        }
    }
    
    // Delete copy operations
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

// Convenience function for creating lock guard with location tracking
template<bool DebuggingEnabled>
inline auto makeLockGuard(Mutex<DebuggingEnabled>& mutex, const LogSource& loc) {
    return LockGuard<DebuggingEnabled>(mutex, loc);
}

} // namespace pers