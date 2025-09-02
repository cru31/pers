@echo off
for /f "tokens=5" %%a in ('netstat -aon ^| findstr :5000 ^| findstr LISTENING') do (
    echo Killing existing server with PID %%a
    taskkill /F /PID %%a >nul 2>&1
)
