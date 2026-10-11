@echo off
rem Double-click: download the runtime.exe / TypeDB built by CI on main (Debug).
cd /d "%~dp0"
where pwsh >nul 2>nul
if %errorlevel%==0 (
	pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0fetch-runtime.ps1" %*
) else (
	powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0fetch-runtime.ps1" %*
)
pause
