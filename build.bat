@echo off
echo Building XNote v1.0...
echo.

REM Kill running instance if any
taskkill /F /IM xnote.exe >nul 2>&1

echo [1/12] Compiling main.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/main.c -o src/main.o
if errorlevel 1 goto error

echo [2/12] Compiling file_ops.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/file_ops.c -o src/file_ops.o
if errorlevel 1 goto error

echo [3/12] Compiling edit_ops.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/edit_ops.c -o src/edit_ops.o
if errorlevel 1 goto error

echo [4/12] Compiling dialogs.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/dialogs.c -o src/dialogs.o
if errorlevel 1 goto error

echo [5/12] Compiling line_numbers.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/line_numbers.c -o src/line_numbers.o
if errorlevel 1 goto error

echo [6/12] Compiling statusbar.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/statusbar.c -o src/statusbar.o
if errorlevel 1 goto error

echo [7/12] Compiling syntax.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/syntax.c -o src/syntax.o
if errorlevel 1 goto error

echo [8/12] Compiling vim_mode.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/vim_mode.c -o src/vim_mode.o
if errorlevel 1 goto error

echo [9/12] Compiling session.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/session.c -o src/session.o
if errorlevel 1 goto error

echo [10/12] Compiling theme.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/theme.c -o src/theme.o
if errorlevel 1 goto error

echo [11/12] Compiling settings.c...
gcc -Wall -Wextra -O3 -DUNICODE -D_UNICODE -c src/settings.c -o src/settings.o
if errorlevel 1 goto error

echo [12/12] Compiling resources...
windres "--preprocessor=gcc -E -xc -DRC_INVOKED" src/notepad.rc -o src/notepad.o
if errorlevel 1 goto error

echo.
echo Linking...
gcc src/main.o src/file_ops.o src/edit_ops.o src/dialogs.o src/line_numbers.o src/statusbar.o src/syntax.o src/vim_mode.o src/session.o src/theme.o src/settings.o src/notepad.o -o xnote.exe -mwindows -lcomctl32 -lcomdlg32 -lshell32 -s
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
