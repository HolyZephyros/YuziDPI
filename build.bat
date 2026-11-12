@echo off
setlocal

echo [*] Compiling resources...
windres src\app.rc -O coff -o src\app_res.o
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo [*] Compiling YuziDPI...
g++ -std=c++17 -O2 -static -mwindows -municode ^
    src\main.cpp src\dpi_engine.cpp src\app_res.o ^
    -o YuziDPI.exe ^
    -lws2_32 -liphlpapi -lwinhttp -lwininet -ldwmapi -lgdiplus -lgdi32 -luser32 -lshell32 -lpsapi -ladvapi32 -lcomctl32 -lcomdlg32 -lole32 -luuid

if %ERRORLEVEL% EQU 0 (
    del /q src\app_res.o >nul 2>&1
    echo [OK] Build complete: YuziDPI.exe
) else (
    echo [!] Build failed!
    exit /b %ERRORLEVEL%
)
