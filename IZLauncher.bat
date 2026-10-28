@echo off
cls
echo ==========================================================
echo  IZLauncher - IZ Switch 2 Homebrew Launcher
echo  Supports IZ2S Modules/Shells IZLauncher Apps
echo ==========================================================
echo.
echo Open SmileBasic 4 on Nintendo Switch 2, type the following into DIRECT mode with your ns2jb project loaded:
echo   " LOAD "TXT:NS2" "
echo   " RUN "
echo   " REMOVE NSR,&60000000,&30000000 "
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
echo [SETUP] Building and publishing IZLauncher modules (.izm packages)...
node -e "const {publish} = require('./IZ2S-Platform/sdk/publish-modules.cjs'); publish('./IZ2S-Platform').then(r => console.log('[OK] Published modules:', r.updated)).catch(console.error);"

echo.
echo Starting IZLauncher server...

:STARTIZL
if "%PIN%"=="" (
    if "%IPPC%"=="" (
        node IZ2S-Platform\start.cjs
    ) else (
        node IZ2S-Platform\start.cjs %IPPC%
    )
) else (
    if "%IPPC%"=="" (
        node IZ2S-Platform\start.cjs --pin "%PIN%"
    ) else (
        node IZ2S-Platform\start.cjs %IPPC% --pin "%PIN%"
    )
)
goto :STARTIZL
EXIT