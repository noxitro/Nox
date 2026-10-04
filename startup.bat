@echo off
rem Builds the code generators in runtime\bin\source and deploys them to runtime\bin.
rem The work itself is done by startup.ps1 (see README.md).
rem Keep this file ASCII only: cmd.exe reads it in the console code page.
setlocal

where pwsh >nul 2>nul
if %errorlevel%==0 (
	pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0startup.ps1" %*
) else (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0startup.ps1" %*
)
set "RESULT=%errorlevel%"

rem Keep the window open on failure so the error can be read.
if not "%RESULT%"=="0" (
	pause
	exit /b %RESULT%
)

rem On success, pause only when started by double-click (Explorer runs "cmd /c <this file>").
echo %cmdcmdline% | find /i "%~nx0" >nul && pause
exit /b 0
