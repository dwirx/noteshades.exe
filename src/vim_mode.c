#include "vim_mode.h"
#include "notepad.h"
#include <richedit.h>

/* Global vim state */
VimState g_VimState = {0};

/* Initialize vim mode */
void InitVimMode(void) {
    g_VimState.bEnabled = FALSE;
    g_VimState.mode = VIM_MODE_NORMAL;
    g_VimState.nRepeatCount = 0;
    g_VimState.chPendingOp = 0;
    g_VimState.dwVisualStart = 0;
    g_VimState.szCommandBuffer[0] = TEXT('\0');
    g_VimState.nCommandLen = 0;
    g_VimState.szSearchPattern[0] = TEXT('\0');
    g_VimState.bSearchForward = TRUE;
}

/* Toggle vim mode on/off */
void ToggleVimMode(HWND hwnd) {
    g_VimState.bEnabled = !g_VimState.bEnabled;
    if (g_VimState.bEnabled) {
        g_VimState.mode = VIM_MODE_NORMAL;
    }
    /* Update menu check mark */
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_VIEW_VIMMODE, 
                  g_VimState.bEnabled ? MF_CHECKED : MF_UNCHECKED);
}

/* Check if vim mode is enabled */
BOOL IsVimModeEnabled(void) {
    return g_VimState.bEnabled;
}

/* Get current vim mode state */
VimModeState GetVimModeState(void) {
    return g_VimState.mode;
}

/* Get vim mode string for status bar */
const TCHAR* GetVimModeString(void) {
    if (!g_VimState.bEnabled) {
        return TEXT("");
    }
    switch (g_VimState.mode) {
        case VIM_MODE_NORMAL:  return TEXT("NORMAL");
        case VIM_MODE_INSERT:  return TEXT("INSERT");
        case VIM_MODE_VISUAL:  return TEXT("VISUAL");
        case VIM_MODE_COMMAND: return TEXT("COMMAND");
        default: return TEXT("");
    }
}

/* Helper: Get current cursor position in edit control */
static DWORD GetEditCursorPos(HWND hwndEdit) {
    DWORD dwStart, dwEnd;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    return dwEnd;
}

/* Helper: Set cursor position in edit control */
static void SetEditCursorPos(HWND hwndEdit, DWORD pos) {
    SendMessage(hwndEdit, EM_SETSEL, pos, pos);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
}

/* Helper: Get text length */
static DWORD GetTextLen(HWND hwndEdit) {
    return (DWORD)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
}

/* Helper: Get line from char position */
static int GetLineFromChar(HWND hwndEdit, DWORD pos) {
    return (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, pos, 0);
}

/* Helper: Get char index at line start */
static DWORD GetLineIndex(HWND hwndEdit, int line) {
    return (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, line, 0);
}

/* Helper: Get line length */
static int GetLineLen(HWND hwndEdit, int line) {
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    return (int)SendMessage(hwndEdit, EM_LINELENGTH, lineStart, 0);
}

/* Helper: Get total line count */
static int GetLineCount(HWND hwndEdit) {
    return (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
}

/* Vim navigation: Move left */
void VimMoveLeft(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD lineStart = GetLineIndex(hwndEdit, GetLineFromChar(hwndEdit, pos));
    DWORD newPos = (pos > lineStart + (DWORD)count) ? pos - count : lineStart;
    SetEditCursorPos(hwndEdit, newPos);
}

/* Vim navigation: Move right */
void VimMoveRight(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    DWORD newPos = (pos + count < lineEnd) ? pos + count : lineEnd;
    SetEditCursorPos(hwndEdit, newPos);
}

/* Vim navigation: Move up */
void VimMoveUp(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int col = pos - lineStart;
    
    int newLine = (line > count) ? line - count : 0;
    DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
    int newLineLen = GetLineLen(hwndEdit, newLine);
    DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
    SetEditCursorPos(hwndEdit, newPos);
}

/* Vim navigation: Move down */
void VimMoveDown(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int col = pos - lineStart;
    int totalLines = GetLineCount(hwndEdit);
    
    int newLine = (line + count < totalLines) ? line + count : totalLines - 1;
    DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
    int newLineLen = GetLineLen(hwndEdit, newLine);
    DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
    SetEditCursorPos(hwndEdit, newPos);
}

/* Vim navigation: Move to line start */
void VimMoveLineStart(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    SetEditCursorPos(hwndEdit, GetLineIndex(hwndEdit, line));
}

/* Vim navigation: Move to line end */
void VimMoveLineEnd(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    SetEditCursorPos(hwndEdit, lineStart + lineLen);
}

/* Vim navigation: Move to first line */
void VimMoveFirstLine(HWND hwndEdit) {
    SetEditCursorPos(hwndEdit, 0);
}

/* Vim navigation: Move to last line */
void VimMoveLastLine(HWND hwndEdit) {
    int totalLines = GetLineCount(hwndEdit);
    DWORD lineStart = GetLineIndex(hwndEdit, totalLines - 1);
    SetEditCursorPos(hwndEdit, lineStart);
}

/* Vim navigation: Move to specific line */
void VimMoveToLine(HWND hwndEdit, int line) {
    int totalLines = GetLineCount(hwndEdit);
    if (line < 1) line = 1;
    if (line > totalLines) line = totalLines;
    DWORD lineStart = GetLineIndex(hwndEdit, line - 1);
    SetEditCursorPos(hwndEdit, lineStart);
}


/* Vim navigation: Word forward */
void VimMoveWordForward(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen = GetTextLen(hwndEdit);
    
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (textLen + 1) * sizeof(TCHAR));
    if (!pText) return;
    GetWindowText(hwndEdit, pText, textLen + 1);
    
    for (int i = 0; i < count && pos < textLen; i++) {
        while (pos < textLen && (IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_'))) pos++;
        while (pos < textLen && !IsCharAlphaNumeric(pText[pos]) && pText[pos] != TEXT('_')) pos++;
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    SetEditCursorPos(hwndEdit, pos);
}

/* Vim navigation: Word backward */
void VimMoveWordBackward(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen = GetTextLen(hwndEdit);
    
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (textLen + 1) * sizeof(TCHAR));
    if (!pText) return;
    GetWindowText(hwndEdit, pText, textLen + 1);
    
    for (int i = 0; i < count && pos > 0; i++) {
        while (pos > 0 && !IsCharAlphaNumeric(pText[pos - 1]) && pText[pos - 1] != TEXT('_')) pos--;
        while (pos > 0 && (IsCharAlphaNumeric(pText[pos - 1]) || pText[pos - 1] == TEXT('_'))) pos--;
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    SetEditCursorPos(hwndEdit, pos);
}

/* Vim navigation: Page down */
void VimPageDown(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) {
        SendMessage(hwndEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    }
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    SetEditCursorPos(hwndEdit, GetLineIndex(hwndEdit, firstLine));
}

/* Vim navigation: Page up */
void VimPageUp(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) {
        SendMessage(hwndEdit, EM_SCROLL, SB_PAGEUP, 0);
    }
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    SetEditCursorPos(hwndEdit, GetLineIndex(hwndEdit, firstLine));
}

/* Vim navigation: Half page down */
void VimHalfPageDown(HWND hwndEdit, int count) {
    RECT rc;
    GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16 / 2;
    VimMoveDown(hwndEdit, visibleLines * count);
}

/* Vim navigation: Half page up */
void VimHalfPageUp(HWND hwndEdit, int count) {
    RECT rc;
    GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16 / 2;
    VimMoveUp(hwndEdit, visibleLines * count);
}

/* Vim edit: Delete character under cursor */
void VimDeleteChar(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen = GetTextLen(hwndEdit);
    DWORD endPos = (pos + count < textLen) ? pos + count : textLen;
    SendMessage(hwndEdit, EM_SETSEL, pos, endPos);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

/* Vim edit: Delete character before cursor */
void VimDeleteCharBefore(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD startPos = (pos > (DWORD)count) ? pos - count : 0;
    SendMessage(hwndEdit, EM_SETSEL, startPos, pos);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

/* Vim edit: Delete line */
void VimDeleteLine(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    int totalLines = GetLineCount(hwndEdit);
    int endLine = (line + count < totalLines) ? line + count : totalLines;
    
    DWORD startPos = GetLineIndex(hwndEdit, line);
    DWORD endPos = (endLine < totalLines) ? GetLineIndex(hwndEdit, endLine) : GetTextLen(hwndEdit);
    
    SendMessage(hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

/* Vim edit: Delete to end of line */
void VimDeleteToEnd(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    
    SendMessage(hwndEdit, EM_SETSEL, pos, lineEnd);
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

/* Vim edit: Yank (copy) line */
void VimYankLine(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    int totalLines = GetLineCount(hwndEdit);
    int endLine = (line + count < totalLines) ? line + count : totalLines;
    
    DWORD startPos = GetLineIndex(hwndEdit, line);
    DWORD endPos = (endLine < totalLines) ? GetLineIndex(hwndEdit, endLine) : GetTextLen(hwndEdit);
    
    SendMessage(hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_SETSEL, pos, pos);
}

/* Vim edit: Paste */
void VimPaste(HWND hwndEdit) {
    SendMessage(hwndEdit, WM_PASTE, 0, 0);
}

/* Vim edit: Paste before cursor */
void VimPasteBefore(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos > 0) SetEditCursorPos(hwndEdit, pos - 1);
    SendMessage(hwndEdit, WM_PASTE, 0, 0);
}

/* Vim edit: Undo */
void VimUndo(HWND hwndEdit) {
    SendMessage(hwndEdit, EM_UNDO, 0, 0);
}

/* Vim mode: Enter insert mode */
void VimEnterInsertMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_INSERT;
    (void)hwndEdit;
}

/* Vim mode: Enter insert mode after cursor */
void VimEnterInsertModeAfter(HWND hwndEdit) {
    VimMoveRight(hwndEdit, 1);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* Vim mode: Enter insert mode at line start */
void VimEnterInsertModeLineStart(HWND hwndEdit) {
    VimMoveLineStart(hwndEdit);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* Vim mode: Enter insert mode at line end */
void VimEnterInsertModeLineEnd(HWND hwndEdit) {
    VimMoveLineEnd(hwndEdit);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* Vim mode: Enter insert mode with new line below */
void VimEnterInsertModeNewLineBelow(HWND hwndEdit) {
    VimMoveLineEnd(hwndEdit);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
    g_VimState.mode = VIM_MODE_INSERT;
}

/* Vim mode: Enter insert mode with new line above */
void VimEnterInsertModeNewLineAbove(HWND hwndEdit) {
    VimMoveLineStart(hwndEdit);
    DWORD pos = GetEditCursorPos(hwndEdit);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
    SetEditCursorPos(hwndEdit, pos);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* Vim mode: Enter visual mode */
void VimEnterVisualMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_VISUAL;
    g_VimState.dwVisualStart = GetEditCursorPos(hwndEdit);
}

/* Vim mode: Enter normal mode */
void VimEnterNormalMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_NORMAL;
    g_VimState.nRepeatCount = 0;
    g_VimState.chPendingOp = 0;
    DWORD pos = GetEditCursorPos(hwndEdit);
    SetEditCursorPos(hwndEdit, pos);
}

/* Search forward */
void VimSearchForward(HWND hwndEdit, const TCHAR* pattern) {
    _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
    g_VimState.bSearchForward = TRUE;
    VimSearchNext(hwndEdit);
}

/* Search backward */
void VimSearchBackward(HWND hwndEdit, const TCHAR* pattern) {
    _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
    g_VimState.bSearchForward = FALSE;
    VimSearchNext(hwndEdit);
}

/* Search next occurrence */
void VimSearchNext(HWND hwndEdit) {
    if (g_VimState.szSearchPattern[0] == TEXT('\0')) return;
    
    DWORD textLen = GetTextLen(hwndEdit);
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (textLen + 1) * sizeof(TCHAR));
    if (!pText) return;
    GetWindowText(hwndEdit, pText, textLen + 1);
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    TCHAR* found = NULL;
    
    if (g_VimState.bSearchForward) {
        found = _tcsstr(pText + pos + 1, g_VimState.szSearchPattern);
        if (!found) found = _tcsstr(pText, g_VimState.szSearchPattern);
    } else {
        TCHAR* search = pText;
        TCHAR* lastFound = NULL;
        while ((search = _tcsstr(search, g_VimState.szSearchPattern)) != NULL) {
            if ((DWORD)(search - pText) < pos) lastFound = search;
            search++;
        }
        found = lastFound;
        if (!found) {
            search = pText;
            while ((search = _tcsstr(search, g_VimState.szSearchPattern)) != NULL) {
                lastFound = search;
                search++;
            }
            found = lastFound;
        }
    }
    
    if (found) SetEditCursorPos(hwndEdit, (DWORD)(found - pText));
    HeapFree(GetProcessHeap(), 0, pText);
}

/* Search previous occurrence */
void VimSearchPrev(HWND hwndEdit) {
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
    VimSearchNext(hwndEdit);
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
}


/* Process key input in vim mode */
BOOL ProcessVimKey(HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    
    if (!g_VimState.bEnabled) return FALSE;
    
    /* In insert mode, only handle Escape */
    if (g_VimState.mode == VIM_MODE_INSERT) {
        if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
            VimEnterNormalMode(hwndEdit);
            return TRUE;
        }
        return FALSE;
    }
    
    if (msg != WM_KEYDOWN && msg != WM_CHAR) return FALSE;
    
    int count = (g_VimState.nRepeatCount > 0) ? g_VimState.nRepeatCount : 1;
    
    /* Handle WM_KEYDOWN for special keys */
    if (msg == WM_KEYDOWN) {
        switch (wParam) {
            case VK_ESCAPE: VimEnterNormalMode(hwndEdit); return TRUE;
            case VK_LEFT: VimMoveLeft(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
            case VK_RIGHT: VimMoveRight(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
            case VK_UP: VimMoveUp(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
            case VK_DOWN: VimMoveDown(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
            case VK_HOME: VimMoveLineStart(hwndEdit); return TRUE;
            case VK_END: VimMoveLineEnd(hwndEdit); return TRUE;
            case VK_PRIOR: VimPageUp(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
            case VK_NEXT: VimPageDown(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE;
        }
    }
    
    /* Handle WM_CHAR for character keys */
    if (msg == WM_CHAR) {
        TCHAR ch = (TCHAR)wParam;
        
        /* Handle digit for repeat count */
        if (ch >= TEXT('1') && ch <= TEXT('9')) {
            g_VimState.nRepeatCount = g_VimState.nRepeatCount * 10 + (ch - TEXT('0'));
            return TRUE;
        }
        if (ch == TEXT('0') && g_VimState.nRepeatCount > 0) {
            g_VimState.nRepeatCount = g_VimState.nRepeatCount * 10;
            return TRUE;
        }
        
        /* Handle pending operator */
        if (g_VimState.chPendingOp) {
            switch (g_VimState.chPendingOp) {
                case TEXT('d'):
                    if (ch == TEXT('d')) VimDeleteLine(hwndEdit, count);
                    else if (ch == TEXT('w')) {
                        DWORD start = GetEditCursorPos(hwndEdit);
                        VimMoveWordForward(hwndEdit, count);
                        DWORD end = GetEditCursorPos(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, start, end);
                        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                    }
                    else if (ch == TEXT('$')) VimDeleteToEnd(hwndEdit);
                    break;
                case TEXT('y'):
                    if (ch == TEXT('y')) VimYankLine(hwndEdit, count);
                    break;
                case TEXT('g'):
                    if (ch == TEXT('g')) VimMoveFirstLine(hwndEdit);
                    break;
            }
            g_VimState.chPendingOp = 0;
            g_VimState.nRepeatCount = 0;
            return TRUE;
        }
        
        /* Normal mode commands */
        switch (ch) {
            case TEXT('h'): VimMoveLeft(hwndEdit, count); break;
            case TEXT('l'): VimMoveRight(hwndEdit, count); break;
            case TEXT('j'): VimMoveDown(hwndEdit, count); break;
            case TEXT('k'): VimMoveUp(hwndEdit, count); break;
            case TEXT('w'): VimMoveWordForward(hwndEdit, count); break;
            case TEXT('b'): VimMoveWordBackward(hwndEdit, count); break;
            case TEXT('0'): VimMoveLineStart(hwndEdit); break;
            case TEXT('$'): VimMoveLineEnd(hwndEdit); break;
            case TEXT('^'): VimMoveLineStart(hwndEdit); break;
            case TEXT('G'):
                if (g_VimState.nRepeatCount > 0) VimMoveToLine(hwndEdit, g_VimState.nRepeatCount);
                else VimMoveLastLine(hwndEdit);
                break;
            case TEXT('g'): g_VimState.chPendingOp = TEXT('g'); return TRUE;
            case TEXT('x'): VimDeleteChar(hwndEdit, count); break;
            case TEXT('X'): VimDeleteCharBefore(hwndEdit, count); break;
            case TEXT('d'): g_VimState.chPendingOp = TEXT('d'); return TRUE;
            case TEXT('D'): VimDeleteToEnd(hwndEdit); break;
            case TEXT('y'): g_VimState.chPendingOp = TEXT('y'); return TRUE;
            case TEXT('Y'): VimYankLine(hwndEdit, count); break;
            case TEXT('p'): VimPaste(hwndEdit); break;
            case TEXT('P'): VimPasteBefore(hwndEdit); break;
            case TEXT('u'): VimUndo(hwndEdit); break;
            case TEXT('i'): VimEnterInsertMode(hwndEdit); break;
            case TEXT('a'): VimEnterInsertModeAfter(hwndEdit); break;
            case TEXT('I'): VimEnterInsertModeLineStart(hwndEdit); break;
            case TEXT('A'): VimEnterInsertModeLineEnd(hwndEdit); break;
            case TEXT('o'): VimEnterInsertModeNewLineBelow(hwndEdit); break;
            case TEXT('O'): VimEnterInsertModeNewLineAbove(hwndEdit); break;
            case TEXT('v'): VimEnterVisualMode(hwndEdit); break;
            case TEXT('n'): VimSearchNext(hwndEdit); break;
            case TEXT('N'): VimSearchPrev(hwndEdit); break;
            default: g_VimState.nRepeatCount = 0; return FALSE;
        }
        
        g_VimState.nRepeatCount = 0;
        return TRUE;
    }
    
    return FALSE;
}
