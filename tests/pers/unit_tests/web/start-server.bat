@echo off
setlocal

:: Kill existing Node.js processes on port 5000
echo [INFO] Checking for existing server on port 5000...
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :5000 ^| findstr LISTENING') do (
    echo [INFO] Found existing process PID: %%a
    taskkill /PID %%a /F >nul 2>&1
    echo [INFO] Terminated existing server process
)

:: Wait a moment for port to be released
timeout /t 1 /nobreak >nul

:: Start new server
echo [INFO] Starting test result viewer server...
cd /d "%~dp0"
start /B node server.js

:: Wait for server to start
timeout /t 2 /nobreak >nul

echo [INFO] Server started on http://localhost:5000
echo [INFO] Opening browser...
start http://localhost:5000

endlocal