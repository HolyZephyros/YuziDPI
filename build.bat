@echo off
setlocal
set "GPP_DIR=C:\Users\Zephyros\.gemini\antigravity\brain\e2d25411-05fe-45ab-91d3-0b1207181808\scratch\w64devkit\bin"
if exist "%GPP_DIR%\g++.exe" (
    set "PATH=%GPP_DIR%;%PATH%"
)

set "WAS_RUNNING=0"
tasklist /FI "IMAGENAME eq YuziDPI.exe" 2>nul | "%SystemRoot%\System32\find.exe" /I "YuziDPI.exe" >nul && set "WAS_RUNNING=1"
taskkill /F /IM YuziDPI.exe >nul 2>&1
taskkill /F /IM ZeroDPI.exe >nul 2>&1
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings" /v ProxyEnable /t REG_DWORD /d 0 /f >nul 2>&1
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings" /v ProxyServer /f >nul 2>&1
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings" /v AutoConfigURL /f >nul 2>&1

echo [*] YuziDPI Kaynak ve Logo Ikonu Derleniyor...
windres src\app.rc -O coff -o src\app_res.o

echo [*] YuziDPI Native Win32 C++ Derleniyor...
g++ -std=c++17 -O2 -static -mwindows -municode ^
    src\main.cpp src\dpi_engine.cpp src\app_res.o ^
    -o YuziDPI.exe ^
    -lws2_32 -liphlpapi -lwinhttp -lwininet -ldwmapi -lgdiplus -lgdi32 -luser32 -lshell32 -lpsapi -ladvapi32 -lcomctl32 -lcomdlg32 -lole32 -luuid

if %ERRORLEVEL% EQU 0 (
    del /q src\app_res.o >nul 2>&1
    del /q ZeroDPI.exe >nul 2>&1
    del /q zerodpi.ini >nul 2>&1
    echo [OK] Derleme Basarili: YuziDPI.exe hazir!
    if "%WAS_RUNNING%"=="1" (
        start "" "%~dp0YuziDPI.exe"
    )
) else (
    echo [!] Derleme Hatasi!
    exit /b %ERRORLEVEL%
)
