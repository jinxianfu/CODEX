@echo off
setlocal
cd /d "%~dp0"
if not exist "bin\x64\Release\EmployeeManagement.exe" (
  call build.bat Release
  if errorlevel 1 exit /b 1
)
"bin\x64\Release\EmployeeManagement.exe" --config "%~dp0database.ini"
set "RUN_RESULT=%errorlevel%"
pause
exit /b %RUN_RESULT%
