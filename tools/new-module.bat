@echo off
rem Adds a module project under runtime\modules and registers it in runtime\runtime.slnx.
rem The work itself is done by new-module.ps1 (see README.md). Without arguments it asks for the name.
rem Keep this file ASCII only: cmd.exe reads it in the console code page.
setlocal

where pwsh >nul 2>nul
if %errorlevel%==0 (
	pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0new-module.ps1" %*
) else (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0new-module.ps1" %*
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
