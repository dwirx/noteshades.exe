@echo off
echo Building XNote v1.0...
echo.

REM Kill running instance if any
taskkill /F /IM xnote.exe >nul 2>&1

echo [1/8] Compiling main.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/main.c -o src/main.o
if errorlevel 1 goto error

echo [2/8] Compiling file_ops.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/file_ops.c -o src/file_ops.o
if errorlevel 1 goto error

echo [3/8] Compiling edit_ops.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/edit_ops.c -o src/edit_ops.o
if errorlevel 1 goto error

echo [4/8] Compiling dialogs.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/dialogs.c -o src/dialogs.o
if errorlevel 1 goto error

echo [5/8] Compiling line_numbers.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/line_numbers.c -o src/line_numbers.o
if errorlevel 1 goto error

echo [6/8] Compiling statusbar.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/statusbar.c -o src/statusbar.o
if errorlevel 1 goto error

echo [7/8] Compiling syntax.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/syntax.c -o src/syntax.o
if errorlevel 1 goto error

echo [8/8] Compiling resources...
windres "--preprocessor=gcc -E -xc -DRC_INVOKED" src/notepad.rc -o src/notepad.o
if errorlevel 1 goto error

echo.
echo Linking...
gcc src/main.o src/file_ops.o src/edit_ops.o src/dialogs.o src/line_numbers.o src/statusbar.o src/syntax.o src/notepad.o -o xnote.exe -mwindows -lcomctl32 -lcomdlg32 -s
if errorlevel 1 goto error

echo.
echo ========================================
echo Build successful!
echo.
echo XNote - Fast and Lightweight Text Editor
echo.
echo Features:
echo   - Multiple tabs with close button
echo   - Line numbers (View menu)
echo   - Syntax highlighting for 20+ languages
echo   - Large file support (up to 100MB)
echo   - UTF-8 encoding
echo   - Word wrap toggle
echo   - Status bar with file info
echo.
echo Supported Languages:
echo   C, C++, Java, JavaScript, TypeScript, Python,
echo   Go, Rust, HTML, CSS, JSON, XML, YAML, SQL,
echo   PHP, Ruby, Shell, Batch, PowerShell, Markdown
echo.
echo Shortcuts:
echo   Ctrl+T: New tab
echo   Ctrl+W: Close tab
echo   Ctrl+N: New document
echo.
echo Run xnote.exe to start the application.
echo ========================================
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
