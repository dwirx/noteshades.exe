@echo off
REM xNote Installer Build Script
REM This script builds the xNote installer using NSIS

setlocal enabledelayedexpansion

echo ========================================
echo xNote Installer Build Script
echo ========================================
echo.

REM Check if we're in the installer directory
if not exist "xnote.nsi" (
    echo Error: xnote.nsi not found!
    echo Please run this script from the installer directory.
    exit /b 1
)

REM Check if xnote.exe exists
if not exist "..\xnote.exe" (
    echo xnote.exe not found. Building xNote first...
    echo.
    pushd ..
    call build.bat
    if errorlevel 1 (
        echo Error: Failed to build xNote!
        popd
        exit /b 1
    )
    popd
    echo.
)

REM Check if NSIS is installed
where makensis >nul 2>&1
if errorlevel 1 (
    REM Try common installation paths
    set "NSIS_PATH="
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files (x86)\NSIS\makensis.exe"
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files\NSIS\makensis.exe"
    )
    
    if "!NSIS_PATH!"=="" (
        echo Error: NSIS not found!
        echo Please install NSIS from https://nsis.sourceforge.io/
        echo Or add makensis.exe to your PATH.
        exit /b 1
    )
) else (
    set "NSIS_PATH=makensis"
)

echo Building installer...
echo.

REM Run NSIS compiler
"!NSIS_PATH!" xnote.nsi
if errorlevel 1 (
    echo.
    echo Error: Failed to build installer!
    exit /b 1
)

echo.
echo ========================================
echo Installer built successfully!
echo Output: xnote-setup-1.0.0.exe
echo ========================================

exit /b 0
