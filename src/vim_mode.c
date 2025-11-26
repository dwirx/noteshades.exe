#include "vim_mode.h"
#include "notepad.h"
#include "session.h"
#include <richedit.h>

/* Global vim state */
VimState g_VimState = {0};

/* Pending find character */
static TCHAR g_chPendingFind = 0;
static BOOL g_bFindForward = TRUE;
static BOOL g_bFindTill = FALSE;

/* Initialize vim mode */
void InitVimMode(void) {
    g_VimState.bEnabled = FALSE;
    g_VimState.mode = VIM_MODE_NORMAL;
    g_VimState.nRepeatCount = 0;
    g_VimState.chPendingOp = 0;
    g_VimState.dwVisualStart = 0;
    g_VimState.nVisualStartLine = 0;
    g_VimState.szCommandBuffer[0] = TEXT('\0');
    g_VimState.nCommandLen = 0;
    g_VimState.szSearchPattern[0] = TEXT('\0');
    g_VimState.bSearchForward = TRUE;
    g_VimState.nDesiredCol = 0;
    g_VimState.hwndMain = NULL;
    g_VimState.szLastCommand[0] = TEXT('\0');
    g_VimState.szRegister[0] = TEXT('\0');
    g_VimState.bRegisterIsLine = FALSE;
}

/* Set vim mode state (for loading from session) */
void SetVimModeEnabled(BOOL bEnabled) {
    g_VimState.bEnabled = bEnabled;
    if (bEnabled) {
        g_VimState.mode = VIM_MODE_NORMAL;
    }
}

/* Toggle vim mode on/off */
void ToggleVimMode(HWND hwnd) {
    g_VimState.bEnabled = !g_VimState.bEnabled;
    if (g_VimState.bEnabled) {
        g_VimState.mode = VIM_MODE_NORMAL;
        g_VimState.hwndMain = hwnd;
    }
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_VIEW_VIMMODE,
                  g_VimState.bEnabled ? MF_CHECKED : MF_UNCHECKED);

    /* Mark session dirty to save vim mode state */
    MarkSessionDirty();
}

BOOL IsVimModeEnabled(void) { return g_VimState.bEnabled; }
VimModeState GetVimModeState(void) { return g_VimState.mode; }

const TCHAR* GetVimModeString(void) {
    if (!g_VimState.bEnabled) return TEXT("");
    switch (g_VimState.mode) {
        case VIM_MODE_NORMAL:      return TEXT("NORMAL");
        case VIM_MODE_INSERT:      return TEXT("INSERT");
        case VIM_MODE_VISUAL:      return TEXT("VISUAL");
        case VIM_MODE_VISUAL_LINE: return TEXT("V-LINE");
        case VIM_MODE_COMMAND:     return TEXT("COMMAND");
        case VIM_MODE_SEARCH:      return TEXT("SEARCH");
        default: return TEXT("");
    }
}

const TCHAR* GetVimCommandBuffer(void) {
    return g_VimState.szCommandBuffer;
}

/* ============ Helper Functions ============ */

static DWORD GetEditCursorPos(HWND hwndEdit) {
    DWORD dwStart, dwEnd;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    return dwEnd;
}

static void SetEditCursorPos(HWND hwndEdit, DWORD pos) {
    SendMessage(hwndEdit, EM_SETSEL, pos, pos);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
}

static DWORD GetTextLen(HWND hwndEdit) {
    return (DWORD)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
}

static int GetLineFromChar(HWND hwndEdit, DWORD pos) {
    return (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, pos, 0);
}

static DWORD GetLineIndex(HWND hwndEdit, int line) {
    return (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, line, 0);
}

static int GetLineLen(HWND hwndEdit, int line) {
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    return (int)SendMessage(hwndEdit, EM_LINELENGTH, lineStart, 0);
}

static int GetLineCount(HWND hwndEdit) {
    return (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
}

static int GetCurrentCol(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    return (int)(pos - lineStart);
}

/* Get text buffer - works with both Edit and RichEdit controls */
static TCHAR* GetTextBuffer(HWND hwndEdit, DWORD* pLen) {
    /* Get text length */
    DWORD textLen = (DWORD)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
    if (textLen == 0) {
        if (pLen) *pLen = 0;
        return NULL;
    }
    
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (textLen + 4) * sizeof(TCHAR));
    if (pText) {
        /* Use WM_GETTEXT which works for both Edit and RichEdit */
        SendMessage(hwndEdit, WM_GETTEXT, textLen + 1, (LPARAM)pText);
        if (pLen) *pLen = textLen;
    }
    return pText;
}

static void FreeTextBuffer(TCHAR* pText) {
    if (pText) HeapFree(GetProcessHeap(), 0, pText);
}

/* Update selection for Visual mode (character) */
static void UpdateVisualSelection(HWND hwndEdit, DWORD newPos) {
    DWORD start = g_VimState.dwVisualStart;
    DWORD end = newPos;
    if (start > end) { DWORD tmp = start; start = end; end = tmp; }
    SendMessage(hwndEdit, EM_SETSEL, start, end);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
}

/* Update selection for Visual Line mode */
static void UpdateVisualLineSelection(HWND hwndEdit, int currentLine) {
    int startLine = g_VimState.nVisualStartLine;
    int endLine = currentLine;
    if (startLine > endLine) { int tmp = startLine; startLine = endLine; endLine = tmp; }
    
    DWORD startPos = GetLineIndex(hwndEdit, startLine);
    int totalLines = GetLineCount(hwndEdit);
    DWORD endPos;
    if (endLine + 1 < totalLines) {
        endPos = GetLineIndex(hwndEdit, endLine + 1);
    } else {
        endPos = GetTextLen(hwndEdit);
    }
    SendMessage(hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
}


/* ============ Navigation Functions ============ */

void VimMoveLeft(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD lineStart = GetLineIndex(hwndEdit, GetLineFromChar(hwndEdit, pos));
    DWORD newPos = (pos > lineStart + (DWORD)count) ? pos - count : lineStart;
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, GetLineFromChar(hwndEdit, newPos));
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimMoveRight(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    DWORD newPos = (pos + count < lineEnd) ? pos + count : lineEnd;
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, GetLineFromChar(hwndEdit, newPos));
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimMoveUp(HWND hwndEdit, int count) {
    int line;
    int col;
    
    if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        line = g_VimState.nVisualCurrentLine;
        col = 0;
    } else {
        DWORD pos = GetEditCursorPos(hwndEdit);
        line = GetLineFromChar(hwndEdit, pos);
        DWORD lineStart = GetLineIndex(hwndEdit, line);
        int currentCol = (int)(pos - lineStart);
        col = (g_VimState.nDesiredCol > 0) ? g_VimState.nDesiredCol : currentCol;
    }
    
    int newLine = (line > count) ? line - count : 0;
    
    if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        g_VimState.nVisualCurrentLine = newLine;
        UpdateVisualLineSelection(hwndEdit, newLine);
    } else if (g_VimState.mode == VIM_MODE_VISUAL) {
        DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
        int newLineLen = GetLineLen(hwndEdit, newLine);
        DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
        int newLineLen = GetLineLen(hwndEdit, newLine);
        DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
        SetEditCursorPos(hwndEdit, newPos);
    }
}

void VimMoveDown(HWND hwndEdit, int count) {
    int line;
    int col;
    int totalLines = GetLineCount(hwndEdit);
    
    if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        line = g_VimState.nVisualCurrentLine;
        col = 0;
    } else {
        DWORD pos = GetEditCursorPos(hwndEdit);
        line = GetLineFromChar(hwndEdit, pos);
        DWORD lineStart = GetLineIndex(hwndEdit, line);
        int currentCol = (int)(pos - lineStart);
        col = (g_VimState.nDesiredCol > 0) ? g_VimState.nDesiredCol : currentCol;
    }
    
    int newLine = (line + count < totalLines) ? line + count : totalLines - 1;
    
    if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        g_VimState.nVisualCurrentLine = newLine;
        UpdateVisualLineSelection(hwndEdit, newLine);
    } else if (g_VimState.mode == VIM_MODE_VISUAL) {
        DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
        int newLineLen = GetLineLen(hwndEdit, newLine);
        DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        DWORD newLineStart = GetLineIndex(hwndEdit, newLine);
        int newLineLen = GetLineLen(hwndEdit, newLine);
        DWORD newPos = newLineStart + ((col < newLineLen) ? col : newLineLen);
        SetEditCursorPos(hwndEdit, newPos);
    }
}

void VimMoveLineStart(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD newPos = GetLineIndex(hwndEdit, line);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, line);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = 0;
}

void VimMoveLineEnd(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD newPos = lineStart + lineLen;
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, line);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = 9999;
}

void VimMoveFirstNonBlank(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD newPos = lineStart;
    for (int i = 0; i < lineLen; i++) {
        TCHAR ch = pText[lineStart + i];
        if (ch != TEXT(' ') && ch != TEXT('\t')) {
            newPos = lineStart + i;
            break;
        }
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimMoveFirstLine(HWND hwndEdit) {
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, 0);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        g_VimState.nVisualCurrentLine = 0;
        UpdateVisualLineSelection(hwndEdit, 0);
    } else {
        SetEditCursorPos(hwndEdit, 0);
    }
    g_VimState.nDesiredCol = 0;
}

void VimMoveLastLine(HWND hwndEdit) {
    int totalLines = GetLineCount(hwndEdit);
    DWORD newPos = GetLineIndex(hwndEdit, totalLines - 1);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        g_VimState.nVisualCurrentLine = totalLines - 1;
        UpdateVisualLineSelection(hwndEdit, totalLines - 1);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = 0;
}

void VimMoveToLine(HWND hwndEdit, int line) {
    int totalLines = GetLineCount(hwndEdit);
    if (line < 1) line = 1;
    if (line > totalLines) line = totalLines;
    DWORD newPos = GetLineIndex(hwndEdit, line - 1);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        g_VimState.nVisualCurrentLine = line - 1;
        UpdateVisualLineSelection(hwndEdit, line - 1);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    g_VimState.nDesiredCol = 0;
}

void VimMoveWordForward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos < textLen; i++) {
        while (pos < textLen && (IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_'))) pos++;
        while (pos < textLen && !IsCharAlphaNumeric(pText[pos]) && pText[pos] != TEXT('_')) pos++;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, GetLineFromChar(hwndEdit, pos));
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimMoveWordBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos > 0; i++) {
        while (pos > 0 && !IsCharAlphaNumeric(pText[pos - 1]) && pText[pos - 1] != TEXT('_')) pos--;
        while (pos > 0 && (IsCharAlphaNumeric(pText[pos - 1]) || pText[pos - 1] == TEXT('_'))) pos--;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, GetLineFromChar(hwndEdit, pos));
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimMoveWordEnd(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos < textLen; i++) {
        pos++;
        while (pos < textLen && !IsCharAlphaNumeric(pText[pos]) && pText[pos] != TEXT('_')) pos++;
        while (pos < textLen && (IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_'))) pos++;
        if (pos > 0) pos--;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

void VimPageDown(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) SendMessage(hwndEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    DWORD newPos = GetLineIndex(hwndEdit, firstLine);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, firstLine);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
}

void VimPageUp(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) SendMessage(hwndEdit, EM_SCROLL, SB_PAGEUP, 0);
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    DWORD newPos = GetLineIndex(hwndEdit, firstLine);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
        UpdateVisualLineSelection(hwndEdit, firstLine);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
}

void VimHalfPageDown(HWND hwndEdit, int count) {
    RECT rc; GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16 / 2;
    if (visibleLines < 1) visibleLines = 10;
    VimMoveDown(hwndEdit, visibleLines * count);
}

void VimHalfPageUp(HWND hwndEdit, int count) {
    RECT rc; GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16 / 2;
    if (visibleLines < 1) visibleLines = 10;
    VimMoveUp(hwndEdit, visibleLines * count);
}


/* ============ Find Character Functions ============ */

void VimFindCharForward(HWND hwndEdit, TCHAR ch, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    
    int found = 0;
    for (DWORD i = pos + 1; i < lineEnd && found < count; i++) {
        if (pText[i] == ch) {
            pos = i;
            found++;
        }
    }
    FreeTextBuffer(pText);
    
    if (found > 0) {
        if (g_VimState.mode == VIM_MODE_VISUAL) {
            UpdateVisualSelection(hwndEdit, pos);
        } else {
            SetEditCursorPos(hwndEdit, pos);
        }
    }
}

void VimFindCharBackward(HWND hwndEdit, TCHAR ch, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    
    int found = 0;
    for (DWORD i = pos; i > lineStart && found < count; i--) {
        if (pText[i - 1] == ch) {
            pos = i - 1;
            found++;
        }
    }
    FreeTextBuffer(pText);
    
    if (found > 0) {
        if (g_VimState.mode == VIM_MODE_VISUAL) {
            UpdateVisualSelection(hwndEdit, pos);
        } else {
            SetEditCursorPos(hwndEdit, pos);
        }
    }
}

void VimFindCharTillForward(HWND hwndEdit, TCHAR ch, int count) {
    VimFindCharForward(hwndEdit, ch, count);
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos > 0) SetEditCursorPos(hwndEdit, pos - 1);
}

void VimFindCharTillBackward(HWND hwndEdit, TCHAR ch, int count) {
    VimFindCharBackward(hwndEdit, ch, count);
    DWORD pos = GetEditCursorPos(hwndEdit);
    SetEditCursorPos(hwndEdit, pos + 1);
}

/* ============ Additional Motions ============ */

void VimMoveMatchingBracket(HWND hwndEdit) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    TCHAR ch = pText[pos];
    TCHAR match = 0;
    int dir = 0;
    
    switch (ch) {
        case TEXT('('): match = TEXT(')'); dir = 1; break;
        case TEXT(')'): match = TEXT('('); dir = -1; break;
        case TEXT('['): match = TEXT(']'); dir = 1; break;
        case TEXT(']'): match = TEXT('['); dir = -1; break;
        case TEXT('{'): match = TEXT('}'); dir = 1; break;
        case TEXT('}'): match = TEXT('{'); dir = -1; break;
    }
    
    if (dir != 0) {
        int depth = 1;
        DWORD i = pos + dir;
        while (i > 0 && i < textLen && depth > 0) {
            if (pText[i] == ch) depth++;
            else if (pText[i] == match) depth--;
            if (depth == 0) {
                SetEditCursorPos(hwndEdit, i);
                break;
            }
            i += dir;
        }
    }
    FreeTextBuffer(pText);
}

void VimMoveParagraphForward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int found = 0;
    
    while (pos < textLen && found < count) {
        /* Skip non-empty lines */
        while (pos < textLen && pText[pos] != TEXT('\r') && pText[pos] != TEXT('\n')) pos++;
        /* Skip newlines */
        while (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) pos++;
        /* Check if next line is empty (paragraph boundary) */
        if (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) {
            found++;
        }
    }
    FreeTextBuffer(pText);
    SetEditCursorPos(hwndEdit, pos);
}

void VimMoveParagraphBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int found = 0;
    
    while (pos > 0 && found < count) {
        pos--;
        /* Skip newlines */
        while (pos > 0 && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) pos--;
        /* Skip non-empty lines */
        while (pos > 0 && pText[pos] != TEXT('\r') && pText[pos] != TEXT('\n')) pos--;
        /* Check if previous line is empty */
        if (pos > 0 && (pText[pos - 1] == TEXT('\r') || pText[pos - 1] == TEXT('\n'))) {
            found++;
        }
    }
    FreeTextBuffer(pText);
    SetEditCursorPos(hwndEdit, pos);
}

/* ============ Edit Commands ============ */

void VimDeleteChar(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen = GetTextLen(hwndEdit);
    DWORD endPos = (pos + count < textLen) ? pos + count : textLen;
    SendMessage(hwndEdit, EM_SETSEL, pos, endPos);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

void VimDeleteCharBefore(HWND hwndEdit, int count) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD startPos = (pos > (DWORD)count) ? pos - count : 0;
    SendMessage(hwndEdit, EM_SETSEL, startPos, pos);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

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

void VimPaste(HWND hwndEdit) { SendMessage(hwndEdit, WM_PASTE, 0, 0); }

void VimPasteBefore(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos > 0) SetEditCursorPos(hwndEdit, pos - 1);
    SendMessage(hwndEdit, WM_PASTE, 0, 0);
}

void VimUndo(HWND hwndEdit) { SendMessage(hwndEdit, EM_UNDO, 0, 0); }
void VimRedo(HWND hwndEdit) { SendMessage(hwndEdit, EM_REDO, 0, 0); }

void VimJoinLines(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    int totalLines = GetLineCount(hwndEdit);
    if (line >= totalLines - 1) return;
    
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    
    DWORD nextLineStart = GetLineIndex(hwndEdit, line + 1);
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD deleteEnd = nextLineStart;
    while (deleteEnd < textLen && (pText[deleteEnd] == TEXT(' ') || pText[deleteEnd] == TEXT('\t'))) {
        deleteEnd++;
    }
    FreeTextBuffer(pText);
    
    SendMessage(hwndEdit, EM_SETSEL, lineEnd, deleteEnd);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(" "));
}

void VimIndentLine(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) {
        DWORD pos = GetEditCursorPos(hwndEdit);
        int line = GetLineFromChar(hwndEdit, pos);
        DWORD lineStart = GetLineIndex(hwndEdit, line);
        SetEditCursorPos(hwndEdit, lineStart);
        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\t"));
    }
}

void VimUnindentLine(HWND hwndEdit, int count) {
    for (int i = 0; i < count; i++) {
        DWORD pos = GetEditCursorPos(hwndEdit);
        int line = GetLineFromChar(hwndEdit, pos);
        DWORD lineStart = GetLineIndex(hwndEdit, line);
        
        DWORD textLen;
        TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
        if (!pText) return;
        
        if (lineStart < textLen) {
            TCHAR ch = pText[lineStart];
            if (ch == TEXT('\t')) {
                SendMessage(hwndEdit, EM_SETSEL, lineStart, lineStart + 1);
                SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            } else if (ch == TEXT(' ')) {
                int spaces = 0;
                while (spaces < 4 && lineStart + spaces < textLen && pText[lineStart + spaces] == TEXT(' ')) {
                    spaces++;
                }
                SendMessage(hwndEdit, EM_SETSEL, lineStart, lineStart + spaces);
                SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            }
        }
        FreeTextBuffer(pText);
    }
}


/* ============ Visual Mode Operations ============ */

static void VimVisualDelete(HWND hwndEdit) {
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    g_VimState.mode = VIM_MODE_NORMAL;
}

static void VimVisualYank(HWND hwndEdit) {
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    DWORD start = g_VimState.dwVisualStart;
    SetEditCursorPos(hwndEdit, start);
    g_VimState.mode = VIM_MODE_NORMAL;
}

static void VimVisualChange(HWND hwndEdit) {
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    g_VimState.mode = VIM_MODE_INSERT;
}

/* ============ Mode Transitions ============ */

void VimEnterInsertMode(HWND hwndEdit) { 
    g_VimState.mode = VIM_MODE_INSERT; 
    (void)hwndEdit; 
}

void VimEnterInsertModeAfter(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    if (pos < lineStart + lineLen) {
        SetEditCursorPos(hwndEdit, pos + 1);
    }
    g_VimState.mode = VIM_MODE_INSERT;
}

void VimEnterInsertModeLineStart(HWND hwndEdit) {
    VimMoveFirstNonBlank(hwndEdit);
    g_VimState.mode = VIM_MODE_INSERT;
}

void VimEnterInsertModeLineEnd(HWND hwndEdit) {
    VimMoveLineEnd(hwndEdit);
    g_VimState.mode = VIM_MODE_INSERT;
}

void VimEnterInsertModeNewLineBelow(HWND hwndEdit) {
    VimMoveLineEnd(hwndEdit);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
    g_VimState.mode = VIM_MODE_INSERT;
}

void VimEnterInsertModeNewLineAbove(HWND hwndEdit) {
    VimMoveLineStart(hwndEdit);
    DWORD pos = GetEditCursorPos(hwndEdit);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
    SetEditCursorPos(hwndEdit, pos);
    g_VimState.mode = VIM_MODE_INSERT;
}

void VimEnterVisualMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_VISUAL;
    g_VimState.dwVisualStart = GetEditCursorPos(hwndEdit);
}

void VimEnterVisualLineMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_VISUAL_LINE;
    DWORD pos = GetEditCursorPos(hwndEdit);
    g_VimState.nVisualStartLine = GetLineFromChar(hwndEdit, pos);
    g_VimState.nVisualCurrentLine = g_VimState.nVisualStartLine;
    g_VimState.dwVisualStart = GetLineIndex(hwndEdit, g_VimState.nVisualStartLine);
    UpdateVisualLineSelection(hwndEdit, g_VimState.nVisualCurrentLine);
}

void VimEnterNormalMode(HWND hwndEdit) {
    g_VimState.mode = VIM_MODE_NORMAL;
    g_VimState.nRepeatCount = 0;
    g_VimState.chPendingOp = 0;
    g_VimState.szCommandBuffer[0] = TEXT('\0');
    g_VimState.nCommandLen = 0;
    DWORD pos = GetEditCursorPos(hwndEdit);
    SetEditCursorPos(hwndEdit, pos);
    
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    g_VimState.nDesiredCol = (int)(pos - lineStart);
}

void VimEnterCommandMode(HWND hwndEdit) {
    (void)hwndEdit;
    g_VimState.mode = VIM_MODE_COMMAND;
    g_VimState.szCommandBuffer[0] = TEXT(':');
    g_VimState.szCommandBuffer[1] = TEXT('\0');
    g_VimState.nCommandLen = 1;
}

/* Store search start position for realtime search */
static DWORD g_dwSearchStartPos = 0;

void VimEnterSearchMode(HWND hwndEdit, BOOL bForward) {
    g_VimState.mode = VIM_MODE_SEARCH;
    g_VimState.bSearchForward = bForward;
    g_VimState.szCommandBuffer[0] = bForward ? TEXT('/') : TEXT('?');
    g_VimState.szCommandBuffer[1] = TEXT('\0');
    g_VimState.nCommandLen = 1;
    /* Save current position for realtime search */
    g_dwSearchStartPos = GetEditCursorPos(hwndEdit);
}

/* ============ Search Functions ============ */

void VimSearchForward(HWND hwndEdit, const TCHAR* pattern) {
    _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
    g_VimState.bSearchForward = TRUE;
    VimSearchNext(hwndEdit);
}

void VimSearchBackward(HWND hwndEdit, const TCHAR* pattern) {
    _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
    g_VimState.bSearchForward = FALSE;
    VimSearchNext(hwndEdit);
}

/* Manual case-insensitive search as fallback */
static BOOL ManualSearch(HWND hwndEdit, DWORD startPos, BOOL bWrap) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return FALSE;
    
    int patternLen = (int)_tcslen(g_VimState.szSearchPattern);
    if (patternLen == 0 || (DWORD)patternLen > textLen) {
        FreeTextBuffer(pText);
        return FALSE;
    }
    
    DWORD foundPos = (DWORD)-1;
    
    if (g_VimState.bSearchForward) {
        /* Search forward from startPos */
        for (DWORD i = startPos; i + patternLen <= textLen; i++) {
            if (_tcsnicmp(pText + i, g_VimState.szSearchPattern, patternLen) == 0) {
                foundPos = i;
                break;
            }
        }
        /* Wrap if not found */
        if (foundPos == (DWORD)-1 && bWrap && startPos > 0) {
            for (DWORD i = 0; i < startPos && i + patternLen <= textLen; i++) {
                if (_tcsnicmp(pText + i, g_VimState.szSearchPattern, patternLen) == 0) {
                    foundPos = i;
                    break;
                }
            }
        }
    } else {
        /* Search backward from startPos */
        if (startPos > textLen) startPos = textLen;
        for (DWORD i = (startPos > 0 ? startPos - 1 : 0); ; i--) {
            if (i + patternLen <= textLen) {
                if (_tcsnicmp(pText + i, g_VimState.szSearchPattern, patternLen) == 0) {
                    foundPos = i;
                    break;
                }
            }
            if (i == 0) break;
        }
        /* Wrap if not found */
        if (foundPos == (DWORD)-1 && bWrap) {
            for (DWORD i = textLen - patternLen; i > startPos; i--) {
                if (_tcsnicmp(pText + i, g_VimState.szSearchPattern, patternLen) == 0) {
                    foundPos = i;
                    break;
                }
            }
        }
    }
    
    FreeTextBuffer(pText);
    
    if (foundPos != (DWORD)-1) {
        SendMessage(hwndEdit, EM_SETSEL, foundPos, foundPos + patternLen);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* Search function - tries RichEdit API first, falls back to manual */
static BOOL VimDoSearch(HWND hwndEdit, DWORD startPos, BOOL bWrap) {
    if (g_VimState.szSearchPattern[0] == TEXT('\0')) return FALSE;
    
    int patternLen = (int)_tcslen(g_VimState.szSearchPattern);
    DWORD textLen = (DWORD)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
    
    if (textLen == 0 || patternLen == 0) return FALSE;
    
    /* Try EM_FINDTEXTEX first (RichEdit) */
    FINDTEXTEX ft = {0};
    ft.lpstrText = g_VimState.szSearchPattern;
    
    LRESULT result = -1;
    
    if (g_VimState.bSearchForward) {
        ft.chrg.cpMin = startPos;
        ft.chrg.cpMax = textLen;
        result = SendMessage(hwndEdit, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
        
        if (result == -1 && bWrap && startPos > 0) {
            ft.chrg.cpMin = 0;
            ft.chrg.cpMax = startPos + patternLen;
            result = SendMessage(hwndEdit, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
        }
    } else {
        ft.chrg.cpMin = (startPos > 0) ? startPos - 1 : 0;
        ft.chrg.cpMax = 0;
        result = SendMessage(hwndEdit, EM_FINDTEXTEX, 0, (LPARAM)&ft);
        
        if (result == -1 && bWrap) {
            ft.chrg.cpMin = textLen;
            ft.chrg.cpMax = startPos;
            result = SendMessage(hwndEdit, EM_FINDTEXTEX, 0, (LPARAM)&ft);
        }
    }
    
    if (result != -1 && ft.chrgText.cpMin >= 0) {
        SendMessage(hwndEdit, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
        return TRUE;
    }
    
    /* Fallback to manual search */
    return ManualSearch(hwndEdit, startPos, bWrap);
}

/* Realtime search from saved start position */
void VimSearchRealtime(HWND hwndEdit) {
    if (g_VimState.szSearchPattern[0] == TEXT('\0')) return;
    VimDoSearch(hwndEdit, g_dwSearchStartPos, TRUE);
}

void VimSearchNext(HWND hwndEdit) {
    if (g_VimState.szSearchPattern[0] == TEXT('\0')) return;
    
    /* Get current selection end as start point */
    DWORD dwStart, dwEnd;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* For 'n' command, start search after current match */
    DWORD searchStart;
    if (g_VimState.bSearchForward) {
        searchStart = dwEnd; /* Start after current selection */
    } else {
        searchStart = dwStart; /* Start before current selection */
    }
    
    VimDoSearch(hwndEdit, searchStart, TRUE);
}

void VimSearchPrev(HWND hwndEdit) {
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
    VimSearchNext(hwndEdit);
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
}

/* ============ Command Execution ============ */

void VimExecuteCommand(HWND hwndMain, HWND hwndEdit) {
    TCHAR* cmd = g_VimState.szCommandBuffer + 1; /* Skip ':' */
    
    /* Trim whitespace */
    while (*cmd == TEXT(' ')) cmd++;
    
    /* Parse command */
    if (_tcscmp(cmd, TEXT("q")) == 0 || _tcscmp(cmd, TEXT("quit")) == 0) {
        /* Check for unsaved changes */
        TabState* pTab = GetCurrentTabState();
        if (pTab && pTab->bModified) {
            MessageBox(hwndMain, TEXT("No write since last change (add ! to override)"), 
                       TEXT("Vim"), MB_OK | MB_ICONWARNING);
        } else {
            CloseTab(hwndMain, g_AppState.nCurrentTab);
        }
    }
    else if (_tcscmp(cmd, TEXT("q!")) == 0 || _tcscmp(cmd, TEXT("quit!")) == 0) {
        /* Force close without saving */
        TabState* pTab = GetCurrentTabState();
        if (pTab) pTab->bModified = FALSE;
        CloseTab(hwndMain, g_AppState.nCurrentTab);
    }
    else if (_tcscmp(cmd, TEXT("w")) == 0 || _tcscmp(cmd, TEXT("write")) == 0) {
        FileSave(hwndMain);
    }
    else if (_tcscmp(cmd, TEXT("wq")) == 0 || _tcscmp(cmd, TEXT("x")) == 0) {
        if (FileSave(hwndMain)) {
            TabState* pTab = GetCurrentTabState();
            if (pTab) pTab->bModified = FALSE;
            CloseTab(hwndMain, g_AppState.nCurrentTab);
        }
    }
    else if (_tcscmp(cmd, TEXT("wq!")) == 0) {
        FileSave(hwndMain);
        TabState* pTab = GetCurrentTabState();
        if (pTab) pTab->bModified = FALSE;
        CloseTab(hwndMain, g_AppState.nCurrentTab);
    }
    else if (_tcscmp(cmd, TEXT("qa")) == 0 || _tcscmp(cmd, TEXT("qall")) == 0) {
        /* Quit all - check for unsaved */
        BOOL bHasUnsaved = FALSE;
        for (int i = 0; i < g_AppState.nTabCount; i++) {
            if (g_AppState.tabs[i].bModified) {
                bHasUnsaved = TRUE;
                break;
            }
        }
        if (bHasUnsaved) {
            MessageBox(hwndMain, TEXT("No write since last change (add ! to override)"), 
                       TEXT("Vim"), MB_OK | MB_ICONWARNING);
        } else {
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
        }
    }
    else if (_tcscmp(cmd, TEXT("qa!")) == 0 || _tcscmp(cmd, TEXT("qall!")) == 0) {
        /* Force quit all */
        for (int i = 0; i < g_AppState.nTabCount; i++) {
            g_AppState.tabs[i].bModified = FALSE;
        }
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
    }
    else if (_tcscmp(cmd, TEXT("e")) == 0 || _tcscmp(cmd, TEXT("edit")) == 0) {
        FileOpen(hwndMain);
    }
    else if (_tcscmp(cmd, TEXT("new")) == 0 || _tcscmp(cmd, TEXT("enew")) == 0) {
        AddNewTab(hwndMain, TEXT("Untitled"));
    }
    else if (_tcscmp(cmd, TEXT("tabnew")) == 0 || _tcscmp(cmd, TEXT("tabe")) == 0) {
        AddNewTab(hwndMain, TEXT("Untitled"));
    }
    else if (_tcscmp(cmd, TEXT("tabn")) == 0 || _tcscmp(cmd, TEXT("tabnext")) == 0) {
        if (g_AppState.nTabCount > 1) {
            int nNext = (g_AppState.nCurrentTab + 1) % g_AppState.nTabCount;
            SwitchToTab(hwndMain, nNext);
        }
    }
    else if (_tcscmp(cmd, TEXT("tabp")) == 0 || _tcscmp(cmd, TEXT("tabprev")) == 0) {
        if (g_AppState.nTabCount > 1) {
            int nPrev = (g_AppState.nCurrentTab - 1 + g_AppState.nTabCount) % g_AppState.nTabCount;
            SwitchToTab(hwndMain, nPrev);
        }
    }
    else if (_tcscmp(cmd, TEXT("tabclose")) == 0 || _tcscmp(cmd, TEXT("tabc")) == 0) {
        CloseTab(hwndMain, g_AppState.nCurrentTab);
    }
    else if (_tcscmp(cmd, TEXT("set number")) == 0 || _tcscmp(cmd, TEXT("set nu")) == 0) {
        if (!g_AppState.bShowLineNumbers) {
            ToggleLineNumbers(hwndMain);
        }
    }
    else if (_tcscmp(cmd, TEXT("set nonumber")) == 0 || _tcscmp(cmd, TEXT("set nonu")) == 0) {
        if (g_AppState.bShowLineNumbers) {
            ToggleLineNumbers(hwndMain);
        }
    }
    else if (_tcscmp(cmd, TEXT("set relativenumber")) == 0 || _tcscmp(cmd, TEXT("set rnu")) == 0) {
        if (!g_AppState.bRelativeLineNumbers) {
            ToggleRelativeLineNumbers(hwndMain);
        }
    }
    else if (_tcscmp(cmd, TEXT("set norelativenumber")) == 0 || _tcscmp(cmd, TEXT("set nornu")) == 0) {
        if (g_AppState.bRelativeLineNumbers) {
            ToggleRelativeLineNumbers(hwndMain);
        }
    }
    else if (_tcscmp(cmd, TEXT("set wrap")) == 0) {
        if (!g_AppState.bWordWrap) {
            ToggleWordWrap(hwndMain);
        }
    }
    else if (_tcscmp(cmd, TEXT("set nowrap")) == 0) {
        if (g_AppState.bWordWrap) {
            ToggleWordWrap(hwndMain);
        }
    }
    else if (_tcsncmp(cmd, TEXT("e "), 2) == 0 || _tcsncmp(cmd, TEXT("edit "), 5) == 0) {
        /* Open specific file - not implemented yet */
    }
    else if (cmd[0] >= TEXT('0') && cmd[0] <= TEXT('9')) {
        /* Go to line number */
        int lineNum = _ttoi(cmd);
        if (lineNum > 0) {
            EditGoToLine(hwndEdit, lineNum);
        }
    }
    else if (_tcscmp(cmd, TEXT("$")) == 0) {
        /* Go to last line */
        VimMoveLastLine(hwndEdit);
    }
    else if (_tcslen(cmd) > 0) {
        /* Unknown command */
        TCHAR szMsg[300];
        _sntprintf(szMsg, 300, TEXT("Not an editor command: %s"), cmd);
        MessageBox(hwndMain, szMsg, TEXT("Vim"), MB_OK | MB_ICONWARNING);
    }
    
    /* Return to normal mode */
    g_VimState.mode = VIM_MODE_NORMAL;
    g_VimState.szCommandBuffer[0] = TEXT('\0');
    g_VimState.nCommandLen = 0;
}


/* ============ Key Processing ============ */

BOOL ProcessVimKey(HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (!g_VimState.bEnabled) return FALSE;
    
    /* Command mode - handle typing */
    if (g_VimState.mode == VIM_MODE_COMMAND || g_VimState.mode == VIM_MODE_SEARCH) {
        if (msg == WM_KEYDOWN) {
            if (wParam == VK_ESCAPE) {
                /* Return to start position when canceling search */
                if (g_VimState.mode == VIM_MODE_SEARCH) {
                    SetEditCursorPos(hwndEdit, g_dwSearchStartPos);
                }
                VimEnterNormalMode(hwndEdit);
                return TRUE;
            }
            if (wParam == VK_RETURN) {
                if (g_VimState.mode == VIM_MODE_SEARCH) {
                    /* Confirm search - pattern already saved, cursor at found position */
                    TCHAR* pattern = g_VimState.szCommandBuffer + 1;
                    if (_tcslen(pattern) > 0) {
                        _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
                        /* Move cursor to start of selection (not end) */
                        DWORD dwStart, dwEnd;
                        SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
                        SetEditCursorPos(hwndEdit, dwStart);
                    } else {
                        /* Empty pattern, return to start */
                        SetEditCursorPos(hwndEdit, g_dwSearchStartPos);
                    }
                    g_VimState.mode = VIM_MODE_NORMAL;
                    g_VimState.szCommandBuffer[0] = TEXT('\0');
                    g_VimState.nCommandLen = 0;
                } else {
                    /* Execute command */
                    VimExecuteCommand(g_VimState.hwndMain, hwndEdit);
                }
                return TRUE;
            }
            if (wParam == VK_BACK) {
                if (g_VimState.nCommandLen > 1) {
                    g_VimState.nCommandLen--;
                    g_VimState.szCommandBuffer[g_VimState.nCommandLen] = TEXT('\0');
                    /* Realtime search update */
                    if (g_VimState.mode == VIM_MODE_SEARCH) {
                        if (g_VimState.nCommandLen > 1) {
                            TCHAR* pattern = g_VimState.szCommandBuffer + 1;
                            _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
                            VimSearchRealtime(hwndEdit);
                        } else {
                            /* Pattern empty, return to start position */
                            g_VimState.szSearchPattern[0] = TEXT('\0');
                            SetEditCursorPos(hwndEdit, g_dwSearchStartPos);
                        }
                    }
                } else {
                    /* Return to start position when canceling search */
                    if (g_VimState.mode == VIM_MODE_SEARCH) {
                        SetEditCursorPos(hwndEdit, g_dwSearchStartPos);
                    }
                    VimEnterNormalMode(hwndEdit);
                }
                return TRUE;
            }
            /* Navigate search history with up/down */
            if (wParam == VK_UP || wParam == VK_DOWN) {
                /* Could implement search history here */
                return TRUE;
            }
        }
        if (msg == WM_CHAR) {
            TCHAR ch = (TCHAR)wParam;
            if (ch >= TEXT(' ') && g_VimState.nCommandLen < 254) {
                g_VimState.szCommandBuffer[g_VimState.nCommandLen++] = ch;
                g_VimState.szCommandBuffer[g_VimState.nCommandLen] = TEXT('\0');
                
                /* Realtime search - search as you type from original position */
                if (g_VimState.mode == VIM_MODE_SEARCH) {
                    TCHAR* pattern = g_VimState.szCommandBuffer + 1;
                    if (_tcslen(pattern) > 0) {
                        _tcscpy_s(g_VimState.szSearchPattern, 256, pattern);
                        VimSearchRealtime(hwndEdit);
                    }
                }
            }
            return TRUE;
        }
        return TRUE;
    }
    
    /* Insert mode - only handle Escape */
    if (g_VimState.mode == VIM_MODE_INSERT) {
        if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
            VimEnterNormalMode(hwndEdit);
            return TRUE;
        }
        return FALSE;
    }
    
    if (msg != WM_KEYDOWN && msg != WM_CHAR) return FALSE;
    
    int count = (g_VimState.nRepeatCount > 0) ? g_VimState.nRepeatCount : 1;
    BOOL bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    
    /* Handle pending find character */
    if (g_chPendingFind != 0 && msg == WM_CHAR) {
        TCHAR ch = (TCHAR)wParam;
        if (g_bFindForward) {
            if (g_bFindTill) VimFindCharTillForward(hwndEdit, ch, count);
            else VimFindCharForward(hwndEdit, ch, count);
        } else {
            if (g_bFindTill) VimFindCharTillBackward(hwndEdit, ch, count);
            else VimFindCharBackward(hwndEdit, ch, count);
        }
        g_chPendingFind = 0;
        g_VimState.nRepeatCount = 0;
        return TRUE;
    }
    
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
            
            case 'D':
                if (bCtrl) { VimHalfPageDown(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE; }
                break;
            case 'U':
                if (bCtrl) { VimHalfPageUp(hwndEdit, count); g_VimState.nRepeatCount = 0; return TRUE; }
                break;
            case 'R':
                if (bCtrl) { VimRedo(hwndEdit); return TRUE; }
                break;
        }
    }
    
    /* Handle WM_CHAR for character keys */
    if (msg == WM_CHAR) {
        TCHAR ch = (TCHAR)wParam;
        
        /* Handle pending operator in visual modes */
        if (g_VimState.chPendingOp && (g_VimState.mode == VIM_MODE_VISUAL || g_VimState.mode == VIM_MODE_VISUAL_LINE)) {
            if (g_VimState.chPendingOp == TEXT('g') && ch == TEXT('g')) {
                VimMoveFirstLine(hwndEdit);
                g_VimState.chPendingOp = 0;
                return TRUE;
            }
            g_VimState.chPendingOp = 0;
        }
        
        /* Visual Line mode commands */
        if (g_VimState.mode == VIM_MODE_VISUAL_LINE) {
            switch (ch) {
                case TEXT('h'): VimMoveLeft(hwndEdit, count); return TRUE;
                case TEXT('l'): VimMoveRight(hwndEdit, count); return TRUE;
                case TEXT('j'): VimMoveDown(hwndEdit, count); return TRUE;
                case TEXT('k'): VimMoveUp(hwndEdit, count); return TRUE;
                case TEXT('w'): VimMoveWordForward(hwndEdit, count); return TRUE;
                case TEXT('b'): VimMoveWordBackward(hwndEdit, count); return TRUE;
                case TEXT('e'): VimMoveWordEnd(hwndEdit, count); return TRUE;
                case TEXT('0'): VimMoveLineStart(hwndEdit); return TRUE;
                case TEXT('$'): VimMoveLineEnd(hwndEdit); return TRUE;
                case TEXT('^'): VimMoveFirstNonBlank(hwndEdit); return TRUE;
                case TEXT('G'): VimMoveLastLine(hwndEdit); return TRUE;
                case TEXT('g'): g_VimState.chPendingOp = TEXT('g'); return TRUE;
                case TEXT('d'):
                case TEXT('x'): VimVisualDelete(hwndEdit); return TRUE;
                case TEXT('y'): VimVisualYank(hwndEdit); return TRUE;
                case TEXT('c'): VimVisualChange(hwndEdit); return TRUE;
                case TEXT('>'): VimIndentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('<'): VimUnindentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('v'):
                    g_VimState.mode = VIM_MODE_VISUAL;
                    g_VimState.dwVisualStart = GetLineIndex(hwndEdit, g_VimState.nVisualStartLine);
                    return TRUE;
                case 27: VimEnterNormalMode(hwndEdit); return TRUE;
                default: return TRUE;
            }
        }
        
        /* Visual (character) mode commands */
        if (g_VimState.mode == VIM_MODE_VISUAL) {
            switch (ch) {
                case TEXT('h'): VimMoveLeft(hwndEdit, count); return TRUE;
                case TEXT('l'): VimMoveRight(hwndEdit, count); return TRUE;
                case TEXT('j'): VimMoveDown(hwndEdit, count); return TRUE;
                case TEXT('k'): VimMoveUp(hwndEdit, count); return TRUE;
                case TEXT('w'): VimMoveWordForward(hwndEdit, count); return TRUE;
                case TEXT('b'): VimMoveWordBackward(hwndEdit, count); return TRUE;
                case TEXT('e'): VimMoveWordEnd(hwndEdit, count); return TRUE;
                case TEXT('0'): VimMoveLineStart(hwndEdit); return TRUE;
                case TEXT('$'): VimMoveLineEnd(hwndEdit); return TRUE;
                case TEXT('^'): VimMoveFirstNonBlank(hwndEdit); return TRUE;
                case TEXT('G'): VimMoveLastLine(hwndEdit); return TRUE;
                case TEXT('g'): g_VimState.chPendingOp = TEXT('g'); return TRUE;
                case TEXT('d'):
                case TEXT('x'): VimVisualDelete(hwndEdit); return TRUE;
                case TEXT('y'): VimVisualYank(hwndEdit); return TRUE;
                case TEXT('c'): VimVisualChange(hwndEdit); return TRUE;
                case TEXT('>'): VimIndentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('<'): VimUnindentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('V'): VimEnterVisualLineMode(hwndEdit); return TRUE;
                case 27: VimEnterNormalMode(hwndEdit); return TRUE;
                default: return TRUE;
            }
        }
        
        /* Normal mode - handle digit for repeat count */
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
                        SendMessage(hwndEdit, WM_COPY, 0, 0);
                        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                    }
                    else if (ch == TEXT('$')) VimDeleteToEnd(hwndEdit);
                    else if (ch == TEXT('0')) {
                        DWORD pos = GetEditCursorPos(hwndEdit);
                        VimMoveLineStart(hwndEdit);
                        DWORD start = GetEditCursorPos(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, start, pos);
                        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                    }
                    break;
                case TEXT('y'):
                    if (ch == TEXT('y')) VimYankLine(hwndEdit, count);
                    else if (ch == TEXT('w')) {
                        DWORD start = GetEditCursorPos(hwndEdit);
                        VimMoveWordForward(hwndEdit, count);
                        DWORD end = GetEditCursorPos(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, start, end);
                        SendMessage(hwndEdit, WM_COPY, 0, 0);
                        SetEditCursorPos(hwndEdit, start);
                    }
                    break;
                case TEXT('c'):
                    if (ch == TEXT('c')) {
                        VimDeleteLine(hwndEdit, count);
                        g_VimState.mode = VIM_MODE_INSERT;
                    }
                    else if (ch == TEXT('w')) {
                        DWORD start = GetEditCursorPos(hwndEdit);
                        VimMoveWordForward(hwndEdit, count);
                        DWORD end = GetEditCursorPos(hwndEdit);
                        SendMessage(hwndEdit, EM_SETSEL, start, end);
                        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                        g_VimState.mode = VIM_MODE_INSERT;
                    }
                    break;
                case TEXT('g'):
                    if (ch == TEXT('g')) VimMoveFirstLine(hwndEdit);
                    else if (ch == TEXT('t')) {
                        /* gt - next tab */
                        if (g_AppState.nTabCount > 1) {
                            int nNext;
                            if (g_VimState.nRepeatCount > 0) {
                                /* {count}gt goes to tab {count} */
                                nNext = (g_VimState.nRepeatCount - 1) % g_AppState.nTabCount;
                            } else {
                                nNext = (g_AppState.nCurrentTab + 1) % g_AppState.nTabCount;
                            }
                            SwitchToTab(g_VimState.hwndMain, nNext);
                        }
                    }
                    else if (ch == TEXT('T')) {
                        /* gT - previous tab */
                        if (g_AppState.nTabCount > 1) {
                            int nPrev = (g_AppState.nCurrentTab - 1 + g_AppState.nTabCount) % g_AppState.nTabCount;
                            SwitchToTab(g_VimState.hwndMain, nPrev);
                        }
                    }
                    else if (ch == TEXT('0')) {
                        /* g0 - first tab */
                        SwitchToTab(g_VimState.hwndMain, 0);
                    }
                    else if (ch == TEXT('$')) {
                        /* g$ - last tab */
                        SwitchToTab(g_VimState.hwndMain, g_AppState.nTabCount - 1);
                    }
                    break;
                case TEXT('z'):
                    if (ch == TEXT('z')) {
                        /* zz - center current line on screen */
                        DWORD pos = GetEditCursorPos(hwndEdit);
                        int line = GetLineFromChar(hwndEdit, pos);
                        RECT rc; GetClientRect(hwndEdit, &rc);
                        int visibleLines = (rc.bottom - rc.top) / 16;
                        int targetFirst = line - visibleLines / 2;
                        if (targetFirst < 0) targetFirst = 0;
                        int currentFirst = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                        SendMessage(hwndEdit, EM_LINESCROLL, 0, targetFirst - currentFirst);
                    }
                    else if (ch == TEXT('t')) {
                        /* zt - scroll current line to top */
                        DWORD pos = GetEditCursorPos(hwndEdit);
                        int line = GetLineFromChar(hwndEdit, pos);
                        int currentFirst = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                        SendMessage(hwndEdit, EM_LINESCROLL, 0, line - currentFirst);
                    }
                    else if (ch == TEXT('b')) {
                        /* zb - scroll current line to bottom */
                        DWORD pos = GetEditCursorPos(hwndEdit);
                        int line = GetLineFromChar(hwndEdit, pos);
                        RECT rc; GetClientRect(hwndEdit, &rc);
                        int visibleLines = (rc.bottom - rc.top) / 16;
                        int targetFirst = line - visibleLines + 1;
                        if (targetFirst < 0) targetFirst = 0;
                        int currentFirst = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                        SendMessage(hwndEdit, EM_LINESCROLL, 0, targetFirst - currentFirst);
                    }
                    break;
                case TEXT('>'):
                    if (ch == TEXT('>')) VimIndentLine(hwndEdit, count);
                    break;
                case TEXT('<'):
                    if (ch == TEXT('<')) VimUnindentLine(hwndEdit, count);
                    break;
            }
            g_VimState.chPendingOp = 0;
            g_VimState.nRepeatCount = 0;
            return TRUE;
        }
        
        /* Normal mode commands */
        switch (ch) {
            /* Navigation */
            case TEXT('h'): VimMoveLeft(hwndEdit, count); break;
            case TEXT('l'): VimMoveRight(hwndEdit, count); break;
            case TEXT('j'): VimMoveDown(hwndEdit, count); break;
            case TEXT('k'): VimMoveUp(hwndEdit, count); break;
            case TEXT('w'): VimMoveWordForward(hwndEdit, count); break;
            case TEXT('W'): VimMoveWordForward(hwndEdit, count); break;
            case TEXT('b'): VimMoveWordBackward(hwndEdit, count); break;
            case TEXT('B'): VimMoveWordBackward(hwndEdit, count); break;
            case TEXT('e'): VimMoveWordEnd(hwndEdit, count); break;
            case TEXT('0'): VimMoveLineStart(hwndEdit); break;
            case TEXT('$'): VimMoveLineEnd(hwndEdit); break;
            case TEXT('^'): VimMoveFirstNonBlank(hwndEdit); break;
            case TEXT('G'):
                if (g_VimState.nRepeatCount > 0) VimMoveToLine(hwndEdit, g_VimState.nRepeatCount);
                else VimMoveLastLine(hwndEdit);
                break;
            case TEXT('g'): g_VimState.chPendingOp = TEXT('g'); return TRUE;
            case TEXT('%'): VimMoveMatchingBracket(hwndEdit); break;
            case TEXT('{'): VimMoveParagraphBackward(hwndEdit, count); break;
            case TEXT('}'): VimMoveParagraphForward(hwndEdit, count); break;
            case TEXT('H'): {
                /* Move to top of screen */
                int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                VimMoveToLine(hwndEdit, firstLine + 1);
                break;
            }
            case TEXT('M'): {
                /* Move to middle of screen */
                RECT rc; GetClientRect(hwndEdit, &rc);
                int visibleLines = (rc.bottom - rc.top) / 16;
                int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                VimMoveToLine(hwndEdit, firstLine + visibleLines / 2 + 1);
                break;
            }
            case TEXT('L'): {
                /* Move to bottom of screen */
                RECT rc; GetClientRect(hwndEdit, &rc);
                int visibleLines = (rc.bottom - rc.top) / 16;
                int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                int totalLines = GetLineCount(hwndEdit);
                int targetLine = firstLine + visibleLines;
                if (targetLine > totalLines) targetLine = totalLines;
                VimMoveToLine(hwndEdit, targetLine);
                break;
            }
            
            /* Find character on line */
            case TEXT('f'): g_chPendingFind = TEXT('f'); g_bFindForward = TRUE; g_bFindTill = FALSE; return TRUE;
            case TEXT('F'): g_chPendingFind = TEXT('F'); g_bFindForward = FALSE; g_bFindTill = FALSE; return TRUE;
            case TEXT('t'): g_chPendingFind = TEXT('t'); g_bFindForward = TRUE; g_bFindTill = TRUE; return TRUE;
            case TEXT('T'): g_chPendingFind = TEXT('T'); g_bFindForward = FALSE; g_bFindTill = TRUE; return TRUE;
            
            /* Edit commands */
            case TEXT('x'): VimDeleteChar(hwndEdit, count); break;
            case TEXT('X'): VimDeleteCharBefore(hwndEdit, count); break;
            case TEXT('d'): g_VimState.chPendingOp = TEXT('d'); return TRUE;
            case TEXT('D'): VimDeleteToEnd(hwndEdit); break;
            case TEXT('c'): g_VimState.chPendingOp = TEXT('c'); return TRUE;
            case TEXT('C'): VimDeleteToEnd(hwndEdit); g_VimState.mode = VIM_MODE_INSERT; break;
            case TEXT('s'): VimDeleteChar(hwndEdit, count); g_VimState.mode = VIM_MODE_INSERT; break;
            case TEXT('S'): VimDeleteLine(hwndEdit, 1); g_VimState.mode = VIM_MODE_INSERT; break;
            case TEXT('y'): g_VimState.chPendingOp = TEXT('y'); return TRUE;
            case TEXT('Y'): VimYankLine(hwndEdit, count); break;
            case TEXT('p'): VimPaste(hwndEdit); break;
            case TEXT('P'): VimPasteBefore(hwndEdit); break;
            case TEXT('u'): VimUndo(hwndEdit); break;
            case TEXT('J'): VimJoinLines(hwndEdit); break;
            case TEXT('>'): g_VimState.chPendingOp = TEXT('>'); return TRUE;
            case TEXT('<'): g_VimState.chPendingOp = TEXT('<'); return TRUE;
            case TEXT('z'): g_VimState.chPendingOp = TEXT('z'); return TRUE;
            case TEXT('r'): /* Replace char - need pending */ return TRUE;
            
            /* Mode transitions */
            case TEXT('i'): VimEnterInsertMode(hwndEdit); break;
            case TEXT('a'): VimEnterInsertModeAfter(hwndEdit); break;
            case TEXT('I'): VimEnterInsertModeLineStart(hwndEdit); break;
            case TEXT('A'): VimEnterInsertModeLineEnd(hwndEdit); break;
            case TEXT('o'): VimEnterInsertModeNewLineBelow(hwndEdit); break;
            case TEXT('O'): VimEnterInsertModeNewLineAbove(hwndEdit); break;
            case TEXT('v'): VimEnterVisualMode(hwndEdit); break;
            case TEXT('V'): VimEnterVisualLineMode(hwndEdit); break;
            
            /* Command and Search */
            case TEXT(':'): VimEnterCommandMode(hwndEdit); break;
            case TEXT('/'): VimEnterSearchMode(hwndEdit, TRUE); break;
            case TEXT('?'): VimEnterSearchMode(hwndEdit, FALSE); break;
            case TEXT('n'): VimSearchNext(hwndEdit); break;
            case TEXT('N'): VimSearchPrev(hwndEdit); break;
            case TEXT('*'): {
                /* Search word under cursor */
                DWORD textLen;
                TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
                if (pText) {
                    DWORD pos = GetEditCursorPos(hwndEdit);
                    DWORD start = pos, end = pos;
                    while (start > 0 && (IsCharAlphaNumeric(pText[start-1]) || pText[start-1] == TEXT('_'))) start--;
                    while (end < textLen && (IsCharAlphaNumeric(pText[end]) || pText[end] == TEXT('_'))) end++;
                    if (end > start && end - start < 256) {
                        _tcsncpy_s(g_VimState.szSearchPattern, 256, pText + start, end - start);
                        g_VimState.bSearchForward = TRUE;
                        VimSearchNext(hwndEdit);
                    }
                    FreeTextBuffer(pText);
                }
                break;
            }
            case TEXT('#'): {
                /* Search word under cursor backward */
                DWORD textLen;
                TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
                if (pText) {
                    DWORD pos = GetEditCursorPos(hwndEdit);
                    DWORD start = pos, end = pos;
                    while (start > 0 && (IsCharAlphaNumeric(pText[start-1]) || pText[start-1] == TEXT('_'))) start--;
                    while (end < textLen && (IsCharAlphaNumeric(pText[end]) || pText[end] == TEXT('_'))) end++;
                    if (end > start && end - start < 256) {
                        _tcsncpy_s(g_VimState.szSearchPattern, 256, pText + start, end - start);
                        g_VimState.bSearchForward = FALSE;
                        VimSearchNext(hwndEdit);
                    }
                    FreeTextBuffer(pText);
                }
                break;
            }
            
            default: g_VimState.nRepeatCount = 0; return FALSE;
        }
        g_VimState.nRepeatCount = 0;
        return TRUE;
    }
    return FALSE;
}
