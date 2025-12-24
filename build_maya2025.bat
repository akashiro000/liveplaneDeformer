@echo off
REM Build script for Maya 2025 on Windows

echo Building livePlaneDeformer for Maya 2025...

REM Set Maya location
set MAYA_LOCATION=C:\Program Files\Autodesk\Maya2025

REM Check if Maya is installed
if not exist "%MAYA_LOCATION%" (
    echo Error: Maya 2025 not found at %MAYA_LOCATION%
    echo Please edit this script to set the correct MAYA_LOCATION
    pause
    exit /b 1
)

REM Create build directory
if not exist build_maya2025 mkdir build_maya2025
cd build_maya2025

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 16 2019" -A x64 -DMAYA_LOCATION="%MAYA_LOCATION%"

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b %errorlevel%
)

REM Build
echo Building...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo.
echo Build successful!
echo Plugin location: build_maya2025\Release\livePlaneDeformer.mll
echo.
echo To install, copy the plugin to:
echo %USERPROFILE%\Documents\maya\2025\plug-ins\
echo.

pause
