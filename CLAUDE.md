# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

XNote is a lightweight, Win32-based text editor written in C, featuring:
- Multiple tabs support
- Syntax highlighting for 20+ languages
- Vim mode with full modal editing
- Line numbers (absolute and relative)
- Multiple themes (Tokyo Night, Dracula, Nord, etc.)
- Session persistence
- Find/Replace functionality

## Build Commands

### Windows (MinGW-w64)

**Build the application:**
```bash
make                # Build using Makefile
# OR
build.bat          # Build using batch script
```

**Clean build artifacts:**
```bash
make clean
# OR
clean.bat
```

**Build and run:**
```bash
make run
```

**Rebuild from scratch:**
```bash
make rebuild
```

### Build Requirements
- MinGW-w64 with GCC compiler
- windres (Windows Resource Compiler)
- Libraries: comctl32, comdlg32, shell32

### Compiler Flags
- **CFLAGS**: `-Wall -Wextra -O3 -DUNICODE -D_UNICODE`
- **LDFLAGS**: `-mwindows -lcomctl32 -lcomdlg32 -lshell32 -s`

## Architecture

### Core Components

**Main Application Flow:**
```
WinMain → Message Loop → WndProc → Component Functions
```

**Key Global State:**
- `g_AppState` (AppState): Main application state with tab management
- `g_VimState` (VimState): Vim mode state machine
- `g_CurrentTheme` (ThemeType): Current active theme
- `g_FontSettings` (FontSettings): Global font configuration

**Module Structure:**
```
src/
├── main.c          - Entry point, window creation, message loop
├── file_ops.c      - File I/O (open, save, large file handling)
├── edit_ops.c      - Edit operations (cut, copy, paste, indent, etc.)
├── dialogs.c       - Dialog management (open, save, find, replace)
├── line_numbers.c  - Line number rendering and synchronization
├── statusbar.c     - Status bar updates
├── syntax.c        - Syntax highlighting engine
├── vim_mode.c      - Vim mode implementation (Normal, Insert, Visual, Command)
├── session.c       - Session persistence (JSON-based)
├── theme.c         - Theme management
└── settings.c      - Settings management
```

### Tab Management

Each tab maintains its own state in `TabState` structure:
- Edit control handle (RichEdit)
- File path and modification status
- Line number panel
- Syntax highlighting language
- Line ending type (CRLF/LF/CR)

Tabs are stored in `g_AppState.tabs[]` array (max 32 tabs).

### Vim Mode Architecture

Vim mode is implemented as a state machine with 6 modes:
- `VIM_MODE_NORMAL`: Navigation and commands
- `VIM_MODE_INSERT`: Text insertion
- `VIM_MODE_VISUAL`: Character-wise selection
- `VIM_MODE_VISUAL_LINE`: Line-wise selection
- `VIM_MODE_COMMAND`: Colon commands (`:w`, `:q`, etc.)
- `VIM_MODE_SEARCH`: Search mode (`/`, `?`)

**Key Processing:**
Keys are processed in `EditSubclassProc` → `ProcessVimKey()` which handles:
- Repeat counts (e.g., `5j` = down 5 lines)
- Pending operators (e.g., `d` in `dw`)
- Modal transitions
- Command execution

### Syntax Highlighting

Uses RichEdit control with manual color application:
1. `DetectLanguage()` determines language from file extension
2. `ApplySyntaxHighlighting()` tokenizes text and applies colors
3. Colors come from current theme via `GetSyntaxColor()`

**Color Types:**
- Keywords, Strings, Comments, Numbers
- Preprocessor directives, Types, Functions
- Operators, Variables, Constants

### Theme System

Themes define comprehensive color schemes stored in `theme.c`:
- Tokyo Night variants (default)
- Monokai, Dracula, One Dark
- Nord, Gruvbox, Everforest

Each theme provides 20+ colors for UI elements, syntax, and editor chrome.

### Session Persistence

Session data is automatically saved to `xnote_session.json` in AppData:
- All open tabs with file paths
- Cursor positions and scroll states
- Window geometry
- Editor settings (word wrap, line numbers, vim mode)
- Font configuration
- Current theme

Auto-saves every 5 seconds via timer.

## Important Implementation Details

### Line Number Synchronization

Line numbers are rendered in a custom window (`LineNumberWndProc`) that must stay synchronized with the edit control:
- Scroll events in edit control trigger `SyncLineNumberScroll()`
- Edit control is subclassed to intercept scroll messages
- Line number width auto-adjusts based on line count

### Large File Handling

Files up to 100MB are supported via special buffer management:
- Content stored in `TabState.pContent` for large files
- Chunked reading/writing to avoid memory exhaustion
- UTF-8 encoding with BOM detection

### Edit Control Subclassing

Edit controls are subclassed (`EditSubclassProc`) to:
- Intercept Vim key sequences
- Sync line number scrolling
- Handle custom keyboard shortcuts
- Update status bar after navigation

### Font Management

Fonts are managed globally via `g_FontSettings`:
- Applied to all edit controls and UI elements
- Font dialog accessible via Format menu
- Persisted in session data
- Tab control uses smaller font for better filename display

## Common Development Patterns

### Adding a New Menu Command

1. Add command ID to `resource.h`:
   ```c
   #define IDM_YOUR_COMMAND 601
   ```

2. Add menu item to `notepad.rc`

3. Handle in `WndProc` WM_COMMAND:
   ```c
   case IDM_YOUR_COMMAND:
       YourFunction(hwnd);
       break;
   ```

### Adding a New Vim Command

1. Add to `ProcessVimKey()` in `vim_mode.c`
2. Implement motion/edit function
3. Update mode state as needed
4. Consider repeat counts and operators

### Adding a New Theme

1. Add theme type to `ThemeType` enum in `theme.h`
2. Define colors in `g_Themes[]` array in `theme.c`
3. Theme is automatically added to View menu

### Modifying Syntax Highlighting

Colors are defined per-theme, so modify `ThemeColors` structure in the theme definition. To add support for a new language:
1. Add to `LanguageType` enum
2. Implement detection in `DetectLanguage()`
3. Add highlighting logic in `ApplySyntaxHighlighting()`

## File Encoding

- Primary: UTF-16 (UNICODE/WCHAR) internally
- Files: UTF-8 with BOM detection
- Line endings: Auto-detect CRLF/LF/CR, preserve on save

## Testing

The project previously had property-based testing specifications in `.kiro/specs/win32-notepad/design.md`. While test implementations may not be present, the design document outlines 12 correctness properties for validation.

## Known Constraints

- Maximum 32 tabs (`MAX_TABS`)
- Maximum file size: 100MB
- Command buffer: 256 characters
- Session stored in user AppData directory
- Requires Windows with RichEdit control support
