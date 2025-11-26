@echo off
echo Building XNote Benchmark Tool...
gcc -O3 benchmark.c -o benchmark.exe
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo.
    echo Running benchmark...
    benchmark.exe
) else (
    echo Build failed!
)
pause
