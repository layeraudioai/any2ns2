echo off
cls
echo Open SmileBasic 4 on Nintendo Switch 2, type the following into DIRECT mode with your ns2jb project loaded.
echo "             LOAD "TXT:NS2"            "
echo "                  RUN                  "
echo "  REMOVE NSR,&60000000,&30000000   "
pause
ipconfig
Set /P IPPC="Enter the displayed IP:"
echo Enter into the DIRECT mode the "V=" line to pair the Switch 2 and PC.
pause
cd HBLauncher
:HBL
node server.js %IPPC%
goto :HBL
pause