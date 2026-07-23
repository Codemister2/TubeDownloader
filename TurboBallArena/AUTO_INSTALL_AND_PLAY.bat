@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title Turbo Ball Arena 4.0 - Automatic Setup

echo ============================================================
echo   TURBO BALL ARENA 4.0 - DIRECT RENDER EDITION
echo   AUTOMATIC REQUIREMENTS, BUILD, AND LAUNCH
echo ============================================================
echo.

where winget >nul 2>nul
if errorlevel 1 (
  echo [ERROR] Windows Package Manager winget is required.
  echo Install App Installer from Microsoft Store, then run again.
  pause
  exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
  echo [SETUP] Installing CMake...
  winget install --id Kitware.CMake --exact --accept-package-agreements --accept-source-agreements
)

where git >nul 2>nul
if errorlevel 1 (
  echo [SETUP] Installing Git...
  winget install --id Git.Git --exact --accept-package-agreements --accept-source-agreements
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [SETUP] Installing Visual Studio 2022 C++ Build Tools...
  winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --accept-package-agreements --accept-source-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
)

if not exist "%VSWHERE%" (
  echo [ERROR] Visual Studio Build Tools installation was not found.
  pause
  exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
  echo [ERROR] Visual C++ tools are missing.
  echo Re-run Visual Studio Installer and add Desktop development with C++.
  pause
  exit /b 1
)

call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
  echo [ERROR] Could not load the Visual C++ environment.
  pause
  exit /b 1
)

echo [BUILD] Configuring...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :fail

echo [BUILD] Compiling Release...
cmake --build build --config Release --parallel
if errorlevel 1 goto :fail

set "GAME=build\bin\TurboBallArena.exe"
if not exist "%GAME%" set "GAME=build\bin\Release\TurboBallArena.exe"
if not exist "%GAME%" (
  echo [ERROR] Build succeeded but TurboBallArena.exe was not found.
  pause
  exit /b 1
)

echo [PLAY] Launching Turbo Ball Arena...
start "" "%GAME%"
exit /b 0

:fail
echo.
echo [ERROR] The game did not compile successfully.
echo Run CLEAN_REBUILD_AND_PLAY.bat to retry from scratch.
pause
exit /b 1
