@echo off
echo Building Win32 Notepad v2.0...
echo.

REM Kill running instance if any
taskkill /F /IM notepad.exe >nul 2>&1

echo [1/5] Compiling main.c...
gcc -Wall -Wextra -O2 -DUNICODE -D_UNICODE -c src/main.c -o src/main.o
if errorlevel 1 goto error

echo [2/5] Compiling file_ops.c...
gcc -Wall -Wextra -O2 -DUNICODE -D_UNICODE -c src/file_ops.c -o src/file_ops.o
if errorlevel 1 goto error

echo [3/5] Compiling edit_ops.c...
gcc -Wall -Wextra -O2 -DUNICODE -D_UNICODE -c src/edit_ops.c -o src/edit_ops.o
if errorlevel 1 goto error

echo [4/5] Compiling dialogs.c...
gcc -Wall -Wextra -O2 -DUNICODE -D_UNICODE -c src/dialogs.c -o src/dialogs.o
if errorlevel 1 goto error

echo [5/6] Compiling line_numbers.c...
gcc -Wall -Wextra -O2 -DUNICODE -D_UNICODE -c src/line_numbers.c -o src/line_numbers.o
if errorlevel 1 goto error

echo [6/6] Compiling resources...
windres "--preprocessor=gcc -E -xc -DRC_INVOKED" src/notepad.rc -o src/notepad.o
if errorlevel 1 goto error

echo.
echo Linking...
gcc src/main.o src/file_ops.o src/edit_ops.o src/dialogs.o src/line_numbers.o src/notepad.o -o notepad.exe -mwindows -lcomctl32 -lcomdlg32
if errorlevel 1 goto error

echo.
echo ========================================
echo Build successful!
echo.
echo Features:
echo   - Multiple tabs with close button (X)
echo   - Ctrl+T: New tab, Ctrl+W: Close tab
echo   - Large file support (up to 500MB)
echo   - Fast streaming for big files
echo   - UTF-8 encoding
echo   - Word wrap toggle
echo   - Line numbers toggle
echo.
echo Run notepad.exe to start the application.
echo ========================================
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
