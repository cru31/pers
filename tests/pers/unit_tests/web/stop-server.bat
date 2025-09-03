@echo off
setlocal

echo [INFO] Stopping test result viewer server...

:: Kill Node.js processes on port 5000
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :5000 ^| findstr LISTENING') do (
    echo [INFO] Terminating process PID: %%a
    taskkill /PID %%a /F >nul 2>&1
)

echo [INFO] Server stopped

endlocal