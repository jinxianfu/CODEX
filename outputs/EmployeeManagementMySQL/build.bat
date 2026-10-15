@echo off
setlocal
cd /d "%~dp0"
set "BUILD_CONFIG=%~1"
if "%BUILD_CONFIG%"=="" set "BUILD_CONFIG=Release"
if /I not "%BUILD_CONFIG%"=="Release" if /I not "%BUILD_CONFIG%"=="Debug" (
  echo Usage: build.bat [Release or Debug]
  exit /b 1
)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Install Visual Studio 2022 with Desktop development with C++.
  exit /b 1
)
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%I"
if not defined MSBUILD (
  echo MSBuild not found. Install Visual Studio 2022 C++ tools.
  exit /b 1
)
"%MSBUILD%" EmployeeManagement.sln /m:1 /nr:false /p:Configuration=%BUILD_CONFIG% /p:Platform=x64 /verbosity:minimal
exit /b %errorlevel%
