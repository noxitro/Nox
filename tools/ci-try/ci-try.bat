@echo off
rem Double-click: send this checkout's working tree to CI and wait for the result.
cd /d "%~dp0"
where pwsh >nul 2>nul
if %errorlevel%==0 (
	pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0ci-try.ps1" %*
) else (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ci-try.ps1" %*
)
pause