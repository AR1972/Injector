@ECHO OFF
SET SLMGR=cscript //NOLOGO "%SYSTEMROOT%\System32\slmgr.vbs"
"%~dp0Installer_EFI_cli.exe"
IF ERRORLEVEL 2 PAUSE & EXIT
"%~dp0LicenseData.exe" 1 ASUS_FLASH
"%~dp0LicenseData.exe" 2 "%~dp0ASUS.BIN"
ECHO installing certificate...
%SLMGR% -ilc "%~dp0ASUS.XRM-MS" >nul
ECHO installing key...
CALL :PRODUCT_VER_CHECK
%SLMGR% -ipk %PID_KEY% >nul
ECHO restart computer to finish activation.
PAUSE
EXIT

:PRODUCT_VER_CHECK

   REG QUERY "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" /v "ProductName" | FINDSTR /C:"Windows 7" >nul
   IF ERRORLEVEL 1 ECHO ERROR: not Windows 7 & PAUSE & EXIT

   FOR /F "tokens=3" %%A IN ('REG QUERY "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" /v "EditionID"') DO SET EditionID=%%A

   ECHO %EditionID% | FINDSTR /I "Starter" >nul
   IF NOT ERRORLEVEL 1 SET PID_KEY=6K6WB-X73TD-KG794-FJYHG-YCJVG & GOTO :EOF

   ECHO %EditionID% | FINDSTR /I "HomeBasic" >nul
   IF NOT ERRORLEVEL 1 SET PID_KEY=89G97-VYHYT-Y6G8H-PJXV6-77GQM & GOTO :EOF

   ECHO %EditionID% | FINDSTR /I "HomePremium" >nul
   IF NOT ERRORLEVEL 1 SET PID_KEY=2QDBX-9T8HR-2QWT6-HCQXJ-9YQTR & GOTO :EOF

   ECHO %EditionID% | FINDSTR /I "Professional" >nul
   IF NOT ERRORLEVEL 1 SET PID_KEY=2WCJK-R8B4Y-CWRF2-TRJKB-PV9HW & GOTO :EOF

   ECHO %EditionID% | FINDSTR /I "Ultimate" >nul
   IF NOT ERRORLEVEL 1 SET PID_KEY=2Y4WT-DHTBF-Q6MMK-KYK6X-VKM6G & GOTO :EOF

   IF ERRORLEVEL 1 ECHO ERROR: OS is unsupported & PAUSE & EXIT
GOTO :EOF