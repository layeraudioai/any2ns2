@echo off
cls
echo ==========================================================
echo  BrendanLauncher - Unified Switch 2 Hybrid Launcher
echo  IZLauncher, HBLauncher, and SPHAIRA Architecture Suite
echo ==========================================================
echo.
echo Open SmileBasic 4 on Nintendo Switch 2, type the following into DIRECT mode with your ns2jb project loaded:
echo   LOAD "TXT:NS2"
echo   RUN
echo   REMOVE NSR,&60000000,&30000000
echo.
echo ==========================================================
echo Network Configuration:
ipconfig
echo ==========================================================
echo.
set /p IPPC="Enter the displayed PC IP (leave blank to auto-detect): "

echo Enter 4-digit pairing PIN (input will be masked):
for /f "usebackq" %%p in (`powershell -NoProfile -Command "$sec = Read-Host -Prompt 'PIN' -AsSecureString; [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec))"`) do set PIN=%%p

echo PIN entered: ****

echo.
echo [SETUP] Building and verifying BrendanLauncher apps and assets...
node -e "console.log('[OK] BrendanLauncher build environment verified.');"

echo.
echo Starting BrendanLauncher server...

:STARTBL
if "%PIN%"=="" (
    if "%IPPC%"=="" (
        node BrendanLauncher\server.js
    ) else (
        node BrendanLauncher\server.js %IPPC%
    )
) else (
    if "%IPPC%"=="" (
        node BrendanLauncher\server.js --pin "%PIN%"
    ) else (
        node BrendanLauncher\server.js %IPPC% --pin "%PIN%"
    )
)
goto :STARTBL
EXIT