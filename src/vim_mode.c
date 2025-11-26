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

/* ============ WORD Motions (whitespace-delimited) ============ */

/* Helper: check if character is whitespace */
static BOOL IsWhitespace(TCHAR ch) {
    return ch == TEXT(' ') || ch == TEXT('\t') || ch == TEXT('\r') || ch == TEXT('\n');
}

/* W - move to next WORD (whitespace-delimited) */
void VimMoveWORDForward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos < textLen; i++) {
        /* Skip non-whitespace */
        while (pos < textLen && !IsWhitespace(pText[pos])) pos++;
        /* Skip whitespace */
        while (pos < textLen && IsWhitespace(pText[pos])) pos++;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

/* B - move to previous WORD */
void VimMoveWORDBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos > 0; i++) {
        /* Skip whitespace backward */
        while (pos > 0 && IsWhitespace(pText[pos - 1])) pos--;
        /* Skip non-whitespace backward */
        while (pos > 0 && !IsWhitespace(pText[pos - 1])) pos--;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

/* E - move to end of WORD */
void VimMoveWORDEnd(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos < textLen; i++) {
        pos++;
        /* Skip whitespace */
        while (pos < textLen && IsWhitespace(pText[pos])) pos++;
        /* Skip non-whitespace */
        while (pos < textLen && !IsWhitespace(pText[pos])) pos++;
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

/* ge - move to end of previous word */
void VimMoveWordEndBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos > 0; i++) {
        if (pos > 0) pos--;
        /* Skip non-word characters backward */
        while (pos > 0 && !IsCharAlphaNumeric(pText[pos]) && pText[pos] != TEXT('_')) pos--;
        /* Skip word characters backward to find start, then go to end */
        while (pos > 0 && (IsCharAlphaNumeric(pText[pos - 1]) || pText[pos - 1] == TEXT('_'))) pos--;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

/* gE - move to end of previous WORD */
void VimMoveWORDEndBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    for (int i = 0; i < count && pos > 0; i++) {
        if (pos > 0) pos--;
        /* Skip whitespace backward */
        while (pos > 0 && IsWhitespace(pText[pos])) pos--;
        /* Skip non-whitespace backward */
        while (pos > 0 && !IsWhitespace(pText[pos - 1])) pos--;
    }
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
    g_VimState.nDesiredCol = GetCurrentCol(hwndEdit);
}

/* ============ Screen Position Motions ============ */

/* H - move to top of screen */
void VimMoveScreenTop(HWND hwndEdit) {
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    DWORD newPos = GetLineIndex(hwndEdit, firstLine);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    VimMoveFirstNonBlank(hwndEdit);
}

/* M - move to middle of screen */
void VimMoveScreenMiddle(HWND hwndEdit) {
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    RECT rc;
    GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16;
    if (visibleLines < 1) visibleLines = 20;
    
    int middleLine = firstLine + visibleLines / 2;
    int totalLines = GetLineCount(hwndEdit);
    if (middleLine >= totalLines) middleLine = totalLines - 1;
    
    DWORD newPos = GetLineIndex(hwndEdit, middleLine);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    VimMoveFirstNonBlank(hwndEdit);
}

/* L - move to bottom of screen */
void VimMoveScreenBottom(HWND hwndEdit) {
    int firstLine = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    RECT rc;
    GetClientRect(hwndEdit, &rc);
    int visibleLines = (rc.bottom - rc.top) / 16;
    if (visibleLines < 1) visibleLines = 20;
    
    int bottomLine = firstLine + visibleLines - 1;
    int totalLines = GetLineCount(hwndEdit);
    if (bottomLine >= totalLines) bottomLine = totalLines - 1;
    
    DWORD newPos = GetLineIndex(hwndEdit, bottomLine);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, newPos);
    } else {
        SetEditCursorPos(hwndEdit, newPos);
    }
    VimMoveFirstNonBlank(hwndEdit);
}

/* + or Enter - move to first non-blank of next line */
void VimMoveNextLineFirstNonBlank(HWND hwndEdit, int count) {
    VimMoveDown(hwndEdit, count);
    VimMoveFirstNonBlank(hwndEdit);
}

/* - move to first non-blank of previous line */
void VimMovePrevLineFirstNonBlank(HWND hwndEdit, int count) {
    VimMoveUp(hwndEdit, count);
    VimMoveFirstNonBlank(hwndEdit);
}

/* ============ Additional Edit Commands ============ */

/* r - replace single character */
void VimReplaceChar(HWND hwndEdit, TCHAR ch) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen = GetTextLen(hwndEdit);
    if (pos >= textLen) return;
    
    /* Select the character and replace it */
    SendMessage(hwndEdit, EM_SETSEL, pos, pos + 1);
    TCHAR szReplace[2] = { ch, TEXT('\0') };
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)szReplace);
    /* Move cursor back to the replaced position */
    SetEditCursorPos(hwndEdit, pos);
}

/* s - substitute character (delete and enter insert mode) */
void VimSubstituteChar(HWND hwndEdit, int count) {
    VimDeleteChar(hwndEdit, count);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* S - substitute line (delete line content and enter insert mode) */
void VimSubstituteLine(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    
    /* Find first non-blank */
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    DWORD firstNonBlank = lineStart;
    if (pText) {
        for (DWORD i = lineStart; i < lineStart + lineLen; i++) {
            if (pText[i] != TEXT(' ') && pText[i] != TEXT('\t')) {
                firstNonBlank = i;
                break;
            }
        }
        FreeTextBuffer(pText);
    }
    
    /* Delete from first non-blank to end of line */
    SendMessage(hwndEdit, EM_SETSEL, firstNonBlank, lineStart + lineLen);
    SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    g_VimState.mode = VIM_MODE_INSERT;
}

/* C - change to end of line */
void VimChangeToEnd(HWND hwndEdit) {
    VimDeleteToEnd(hwndEdit);
    g_VimState.mode = VIM_MODE_INSERT;
}

/* ~ - toggle case of character under cursor */
void VimToggleCase(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || pos >= textLen) {
        if (pText) FreeTextBuffer(pText);
        return;
    }
    
    TCHAR ch = pText[pos];
    TCHAR newCh = ch;
    
    /* Toggle case using simple ASCII check for better compatibility */
    if (ch >= TEXT('a') && ch <= TEXT('z')) {
        newCh = ch - TEXT('a') + TEXT('A');
    } else if (ch >= TEXT('A') && ch <= TEXT('Z')) {
        newCh = ch - TEXT('A') + TEXT('a');
    }
    FreeTextBuffer(pText);
    
    if (newCh != ch) {
        SendMessage(hwndEdit, EM_SETSEL, pos, pos + 1);
        TCHAR szReplace[2] = { newCh, TEXT('\0') };
        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)szReplace);
    }
    
    /* Move cursor right */
    SetEditCursorPos(hwndEdit, pos + 1);
}

/* ============ Text Objects ============ */

/* Select inner word (iw) */
TextObjectRange VimSelectInnerWord(HWND hwndEdit) {
    TextObjectRange range = {0, 0, FALSE};
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return range;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos >= textLen) {
        FreeTextBuffer(pText);
        return range;
    }
    
    /* Check if on a word character */
    BOOL onWord = IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_');
    
    if (onWord) {
        /* Find word boundaries */
        DWORD start = pos, end = pos;
        while (start > 0 && (IsCharAlphaNumeric(pText[start - 1]) || pText[start - 1] == TEXT('_'))) start--;
        while (end < textLen && (IsCharAlphaNumeric(pText[end]) || pText[end] == TEXT('_'))) end++;
        range.dwStart = start;
        range.dwEnd = end;
        range.bFound = TRUE;
    } else {
        /* On whitespace - select whitespace */
        DWORD start = pos, end = pos;
        while (start > 0 && (pText[start - 1] == TEXT(' ') || pText[start - 1] == TEXT('\t'))) start--;
        while (end < textLen && (pText[end] == TEXT(' ') || pText[end] == TEXT('\t'))) end++;
        if (end > start) {
            range.dwStart = start;
            range.dwEnd = end;
            range.bFound = TRUE;
        }
    }
    
    FreeTextBuffer(pText);
    return range;
}

/* Select a word including surrounding whitespace (aw) */
TextObjectRange VimSelectAWord(HWND hwndEdit) {
    TextObjectRange range = {0, 0, FALSE};
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return range;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos >= textLen) {
        FreeTextBuffer(pText);
        return range;
    }
    
    BOOL onWord = IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_');
    
    if (onWord) {
        /* Find word boundaries */
        DWORD start = pos, end = pos;
        while (start > 0 && (IsCharAlphaNumeric(pText[start - 1]) || pText[start - 1] == TEXT('_'))) start--;
        while (end < textLen && (IsCharAlphaNumeric(pText[end]) || pText[end] == TEXT('_'))) end++;
        
        /* Include trailing whitespace, or leading if at end of line */
        DWORD origEnd = end;
        while (end < textLen && (pText[end] == TEXT(' ') || pText[end] == TEXT('\t'))) end++;
        
        /* If no trailing whitespace, include leading */
        if (end == origEnd) {
            while (start > 0 && (pText[start - 1] == TEXT(' ') || pText[start - 1] == TEXT('\t'))) start--;
        }
        
        range.dwStart = start;
        range.dwEnd = end;
        range.bFound = TRUE;
    }
    
    FreeTextBuffer(pText);
    return range;
}

/* Select inner quote (i" or i') */
TextObjectRange VimSelectInnerQuote(HWND hwndEdit, TCHAR chQuote) {
    TextObjectRange range = {0, 0, FALSE};
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return range;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    
    /* Find opening quote (search backward then forward) */
    DWORD openQuote = (DWORD)-1;
    DWORD closeQuote = (DWORD)-1;
    
    /* First, check if we're inside quotes by counting quotes before cursor */
    int quoteCount = 0;
    for (DWORD i = lineStart; i < pos; i++) {
        if (pText[i] == chQuote) quoteCount++;
    }
    
    if (quoteCount % 2 == 1) {
        /* We're inside quotes - find the opening quote */
        for (DWORD i = pos; i > lineStart; i--) {
            if (pText[i - 1] == chQuote) {
                openQuote = i - 1;
                break;
            }
        }
        /* Find closing quote */
        for (DWORD i = pos; i < lineEnd; i++) {
            if (pText[i] == chQuote) {
                closeQuote = i;
                break;
            }
        }
    } else {
        /* We're outside quotes - find next pair */
        for (DWORD i = pos; i < lineEnd; i++) {
            if (pText[i] == chQuote) {
                if (openQuote == (DWORD)-1) {
                    openQuote = i;
                } else {
                    closeQuote = i;
                    break;
                }
            }
        }
    }
    
    if (openQuote != (DWORD)-1 && closeQuote != (DWORD)-1 && closeQuote > openQuote) {
        range.dwStart = openQuote + 1;
        range.dwEnd = closeQuote;
        range.bFound = TRUE;
    }
    
    FreeTextBuffer(pText);
    return range;
}

/* Select a quote including quotes (a" or a') */
TextObjectRange VimSelectAQuote(HWND hwndEdit, TCHAR chQuote) {
    TextObjectRange range = VimSelectInnerQuote(hwndEdit, chQuote);
    if (range.bFound) {
        /* Include the quotes */
        if (range.dwStart > 0) range.dwStart--;
        range.dwEnd++;
    }
    return range;
}

/* Select inner bracket (i(, i), i{, i}, i[, i], i<, i>) */
TextObjectRange VimSelectInnerBracket(HWND hwndEdit, TCHAR chOpen, TCHAR chClose) {
    TextObjectRange range = {0, 0, FALSE};
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return range;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    
    /* Find matching brackets */
    DWORD openBracket = (DWORD)-1;
    DWORD closeBracket = (DWORD)-1;
    int depth = 0;
    
    /* Search backward for opening bracket */
    for (DWORD i = pos; ; i--) {
        if (pText[i] == chClose) depth++;
        else if (pText[i] == chOpen) {
            if (depth == 0) {
                openBracket = i;
                break;
            }
            depth--;
        }
        if (i == 0) break;
    }
    
    /* Search forward for closing bracket */
    if (openBracket != (DWORD)-1) {
        depth = 0;
        for (DWORD i = openBracket + 1; i < textLen; i++) {
            if (pText[i] == chOpen) depth++;
            else if (pText[i] == chClose) {
                if (depth == 0) {
                    closeBracket = i;
                    break;
                }
                depth--;
            }
        }
    }
    
    if (openBracket != (DWORD)-1 && closeBracket != (DWORD)-1) {
        range.dwStart = openBracket + 1;
        range.dwEnd = closeBracket;
        range.bFound = TRUE;
    }
    
    FreeTextBuffer(pText);
    return range;
}

/* Select a bracket including brackets (a(, a), a{, a}, a[, a], a<, a>) */
TextObjectRange VimSelectABracket(HWND hwndEdit, TCHAR chOpen, TCHAR chClose) {
    TextObjectRange range = VimSelectInnerBracket(hwndEdit, chOpen, chClose);
    if (range.bFound) {
        /* Include the brackets */
        if (range.dwStart > 0) range.dwStart--;
        range.dwEnd++;
    }
    return range;
}

/* Apply operator to text object range */
void VimApplyTextObject(HWND hwndEdit, TCHAR chOp, TextObjectRange range) {
    if (!range.bFound) return;
    
    /* Select the range */
    SendMessage(hwndEdit, EM_SETSEL, range.dwStart, range.dwEnd);
    
    switch (chOp) {
        case TEXT('d'):
        case TEXT('x'):
            /* Delete */
            SendMessage(hwndEdit, WM_COPY, 0, 0);
            SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            g_VimState.bRegisterIsLine = FALSE;
            break;
        case TEXT('y'):
            /* Yank */
            SendMessage(hwndEdit, WM_COPY, 0, 0);
            SetEditCursorPos(hwndEdit, range.dwStart);
            g_VimState.bRegisterIsLine = FALSE;
            break;
        case TEXT('c'):
            /* Change */
            SendMessage(hwndEdit, WM_COPY, 0, 0);
            SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            g_VimState.bRegisterIsLine = FALSE;
            g_VimState.mode = VIM_MODE_INSERT;
            break;
    }
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
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD origPos = pos;
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
    
    /* Only move if found, and stop one character before target */
    if (found > 0 && pos > origPos) {
        DWORD newPos = pos - 1;
        if (newPos > origPos) {
            if (g_VimState.mode == VIM_MODE_VISUAL) {
                UpdateVisualSelection(hwndEdit, newPos);
            } else {
                SetEditCursorPos(hwndEdit, newPos);
            }
        }
    }
}

void VimFindCharTillBackward(HWND hwndEdit, TCHAR ch, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    DWORD origPos = pos;
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
    
    /* Only move if found, and stop one character after target */
    if (found > 0 && pos < origPos) {
        DWORD newPos = pos + 1;
        if (newPos < origPos) {
            if (g_VimState.mode == VIM_MODE_VISUAL) {
                UpdateVisualSelection(hwndEdit, newPos);
            } else {
                SetEditCursorPos(hwndEdit, newPos);
            }
        }
    }
}

/* ============ Additional Motions ============ */

void VimMoveMatchingBracket(HWND hwndEdit) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText || textLen == 0) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    if (pos >= textLen) {
        FreeTextBuffer(pText);
        return;
    }
    
    TCHAR ch = pText[pos];
    TCHAR openBracket = 0;
    TCHAR closeBracket = 0;
    int dir = 0;
    
    /* Determine bracket type and direction */
    switch (ch) {
        case TEXT('('): openBracket = TEXT('('); closeBracket = TEXT(')'); dir = 1; break;
        case TEXT(')'): openBracket = TEXT('('); closeBracket = TEXT(')'); dir = -1; break;
        case TEXT('['): openBracket = TEXT('['); closeBracket = TEXT(']'); dir = 1; break;
        case TEXT(']'): openBracket = TEXT('['); closeBracket = TEXT(']'); dir = -1; break;
        case TEXT('{'): openBracket = TEXT('{'); closeBracket = TEXT('}'); dir = 1; break;
        case TEXT('}'): openBracket = TEXT('{'); closeBracket = TEXT('}'); dir = -1; break;
        default:
            /* If not on a bracket, search forward for one on this line */
            {
                int line = GetLineFromChar(hwndEdit, pos);
                DWORD lineStart = GetLineIndex(hwndEdit, line);
                int lineLen = GetLineLen(hwndEdit, line);
                DWORD lineEnd = lineStart + lineLen;
                
                for (DWORD i = pos; i < lineEnd; i++) {
                    TCHAR c = pText[i];
                    if (c == TEXT('(') || c == TEXT('[') || c == TEXT('{') ||
                        c == TEXT(')') || c == TEXT(']') || c == TEXT('}')) {
                        pos = i;
                        ch = c;
                        switch (ch) {
                            case TEXT('('): openBracket = TEXT('('); closeBracket = TEXT(')'); dir = 1; break;
                            case TEXT(')'): openBracket = TEXT('('); closeBracket = TEXT(')'); dir = -1; break;
                            case TEXT('['): openBracket = TEXT('['); closeBracket = TEXT(']'); dir = 1; break;
                            case TEXT(']'): openBracket = TEXT('['); closeBracket = TEXT(']'); dir = -1; break;
                            case TEXT('{'): openBracket = TEXT('{'); closeBracket = TEXT('}'); dir = 1; break;
                            case TEXT('}'): openBracket = TEXT('{'); closeBracket = TEXT('}'); dir = -1; break;
                        }
                        break;
                    }
                }
            }
            break;
    }
    
    if (dir != 0) {
        int depth = 1;
        DWORD i = pos;
        
        /* Move in the specified direction */
        while (TRUE) {
            if (dir > 0) {
                i++;
                if (i >= textLen) break;
            } else {
                if (i == 0) break;
                i--;
            }
            
            TCHAR c = pText[i];
            if (c == openBracket) {
                if (dir > 0) depth++;
                else depth--;
            } else if (c == closeBracket) {
                if (dir > 0) depth--;
                else depth++;
            }
            
            if (depth == 0) {
                if (g_VimState.mode == VIM_MODE_VISUAL) {
                    UpdateVisualSelection(hwndEdit, i);
                } else {
                    SetEditCursorPos(hwndEdit, i);
                }
                break;
            }
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
    BOOL wasOnEmptyLine = FALSE;
    
    /* Check if we're starting on an empty line */
    if (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) {
        wasOnEmptyLine = TRUE;
    }
    
    while (found < count && pos < textLen) {
        if (wasOnEmptyLine) {
            /* Skip empty lines first */
            while (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) pos++;
            wasOnEmptyLine = FALSE;
        }
        
        /* Skip non-empty content until we hit an empty line */
        while (pos < textLen && pText[pos] != TEXT('\r') && pText[pos] != TEXT('\n')) pos++;
        
        /* Skip the newline characters */
        while (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) {
            pos++;
            /* Check if this is an empty line (paragraph boundary) */
            if (pos < textLen && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) {
                found++;
                break;
            }
            /* Also count reaching end of file as a paragraph boundary */
            if (pos >= textLen) {
                found++;
                break;
            }
        }
        
        /* If we reached end of file, count it */
        if (pos >= textLen && found < count) {
            found++;
        }
    }
    
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
}

void VimMoveParagraphBackward(HWND hwndEdit, int count) {
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    int found = 0;
    
    while (found < count && pos > 0) {
        /* Move back one character */
        if (pos > 0) pos--;
        
        /* Skip any newlines we're on */
        while (pos > 0 && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) pos--;
        
        /* Skip non-empty content backward */
        while (pos > 0 && pText[pos] != TEXT('\r') && pText[pos] != TEXT('\n')) pos--;
        
        /* Now we're at a newline or start of file */
        if (pos == 0) {
            found++;
        } else {
            /* Check if the previous character is also a newline (empty line = paragraph boundary) */
            if (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n')) {
                /* Move to start of empty line */
                while (pos > 0 && (pText[pos - 1] == TEXT('\r') || pText[pos - 1] == TEXT('\n'))) {
                    pos--;
                }
                found++;
            }
        }
    }
    
    /* Position at start of line after empty line */
    if (pos > 0 && (pText[pos] == TEXT('\r') || pText[pos] == TEXT('\n'))) {
        pos++;
        if (pos < textLen && pText[pos] == TEXT('\n') && pText[pos-1] == TEXT('\r')) {
            pos++;
        }
    }
    
    FreeTextBuffer(pText);
    
    if (g_VimState.mode == VIM_MODE_VISUAL) {
        UpdateVisualSelection(hwndEdit, pos);
    } else {
        SetEditCursorPos(hwndEdit, pos);
    }
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
    
    /* Store deleted content in register */
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (pText && endPos > startPos) {
        DWORD copyLen = endPos - startPos;
        if (copyLen >= sizeof(g_VimState.szRegister) / sizeof(TCHAR)) {
            copyLen = sizeof(g_VimState.szRegister) / sizeof(TCHAR) - 1;
        }
        _tcsncpy_s(g_VimState.szRegister, sizeof(g_VimState.szRegister) / sizeof(TCHAR), 
                   pText + startPos, copyLen);
        g_VimState.bRegisterIsLine = TRUE;  /* Mark as line-mode content */
        FreeTextBuffer(pText);
    }
    
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
    
    /* Store yanked content in register */
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (pText && endPos > startPos) {
        DWORD copyLen = endPos - startPos;
        if (copyLen >= sizeof(g_VimState.szRegister) / sizeof(TCHAR)) {
            copyLen = sizeof(g_VimState.szRegister) / sizeof(TCHAR) - 1;
        }
        _tcsncpy_s(g_VimState.szRegister, sizeof(g_VimState.szRegister) / sizeof(TCHAR), 
                   pText + startPos, copyLen);
        g_VimState.bRegisterIsLine = TRUE;  /* Mark as line-mode content */
        FreeTextBuffer(pText);
    }
    
    SendMessage(hwndEdit, EM_SETSEL, startPos, endPos);
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_SETSEL, pos, pos);
}

/* Paste after cursor (p) - for line-mode paste, paste on new line below */
void VimPaste(HWND hwndEdit) {
    if (g_VimState.bRegisterIsLine) {
        /* Line-mode paste: paste on new line below current line */
        VimMoveLineEnd(hwndEdit);
        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
        SendMessage(hwndEdit, WM_PASTE, 0, 0);
    } else {
        /* Character-mode paste: paste after cursor */
        DWORD pos = GetEditCursorPos(hwndEdit);
        int line = GetLineFromChar(hwndEdit, pos);
        DWORD lineStart = GetLineIndex(hwndEdit, line);
        int lineLen = GetLineLen(hwndEdit, line);
        /* Only move right if not at end of line */
        if (pos < lineStart + lineLen) {
            SetEditCursorPos(hwndEdit, pos + 1);
        }
        SendMessage(hwndEdit, WM_PASTE, 0, 0);
    }
}

/* Paste before cursor (P) - for line-mode paste, paste on new line above */
void VimPasteBefore(HWND hwndEdit) {
    if (g_VimState.bRegisterIsLine) {
        /* Line-mode paste: paste on new line above current line */
        VimMoveLineStart(hwndEdit);
        DWORD pos = GetEditCursorPos(hwndEdit);
        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\r\n"));
        SetEditCursorPos(hwndEdit, pos);
        SendMessage(hwndEdit, WM_PASTE, 0, 0);
    } else {
        /* Character-mode paste: paste at cursor position (before cursor) */
        SendMessage(hwndEdit, WM_PASTE, 0, 0);
    }
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
    /* Buffer navigation commands */
    else if (_tcscmp(cmd, TEXT("bn")) == 0 || _tcscmp(cmd, TEXT("bnext")) == 0) {
        if (g_AppState.nTabCount > 1) {
            int nNext = (g_AppState.nCurrentTab + 1) % g_AppState.nTabCount;
            SwitchToTab(hwndMain, nNext);
        }
    }
    else if (_tcscmp(cmd, TEXT("bp")) == 0 || _tcscmp(cmd, TEXT("bprev")) == 0 || 
             _tcscmp(cmd, TEXT("bN")) == 0 || _tcscmp(cmd, TEXT("bNext")) == 0) {
        if (g_AppState.nTabCount > 1) {
            int nPrev = (g_AppState.nCurrentTab - 1 + g_AppState.nTabCount) % g_AppState.nTabCount;
            SwitchToTab(hwndMain, nPrev);
        }
    }
    else if (_tcscmp(cmd, TEXT("bd")) == 0 || _tcscmp(cmd, TEXT("bdelete")) == 0) {
        CloseTab(hwndMain, g_AppState.nCurrentTab);
    }
    else if (_tcscmp(cmd, TEXT("bd!")) == 0 || _tcscmp(cmd, TEXT("bdelete!")) == 0) {
        TabState* pTab = GetCurrentTabState();
        if (pTab) pTab->bModified = FALSE;
        CloseTab(hwndMain, g_AppState.nCurrentTab);
    }
    /* Utility commands */
    else if (_tcscmp(cmd, TEXT("noh")) == 0 || _tcscmp(cmd, TEXT("nohlsearch")) == 0) {
        /* Clear search highlighting - just clear the pattern */
        g_VimState.szSearchPattern[0] = TEXT('\0');
    }
    else if (_tcscmp(cmd, TEXT("sp")) == 0 || _tcscmp(cmd, TEXT("split")) == 0) {
        MessageBox(hwndMain, TEXT("Split windows are not supported in XNote.\nUse tabs instead (:tabnew)"), 
                   TEXT("Vim"), MB_OK | MB_ICONINFORMATION);
    }
    else if (_tcscmp(cmd, TEXT("vs")) == 0 || _tcscmp(cmd, TEXT("vsplit")) == 0) {
        MessageBox(hwndMain, TEXT("Vertical split is not supported in XNote.\nUse tabs instead (:tabnew)"), 
                   TEXT("Vim"), MB_OK | MB_ICONINFORMATION);
    }
    else if (_tcscmp(cmd, TEXT("help")) == 0 || _tcscmp(cmd, TEXT("h")) == 0) {
        MessageBox(hwndMain, 
            TEXT("XNote Vim Mode Commands:\n\n")
            TEXT("Navigation: h j k l w b e W B E 0 $ ^ gg G H M L { } % f F t T\n")
            TEXT("Edit: i a I A o O x X d c y p P u J r s S C D Y ~\n")
            TEXT("Visual: v V\n")
            TEXT("Search: / ? n N * #\n")
            TEXT("Scroll: Ctrl+D Ctrl+U Ctrl+F Ctrl+B zz zt zb\n\n")
            TEXT("Ex Commands:\n")
            TEXT(":w :q :wq :x :e :tabnew :tabn :tabp :bn :bp :bd\n")
            TEXT(":set nu :set nonu :set rnu :set nornu :set wrap :set nowrap\n")
            TEXT(":{number} - go to line"),
            TEXT("Vim Help"), MB_OK | MB_ICONINFORMATION);
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
            /* Handle text objects: i{obj} or a{obj} */
            if (g_VimState.chPendingOp == TEXT('d') || g_VimState.chPendingOp == TEXT('y') || 
                g_VimState.chPendingOp == TEXT('c')) {
                if (ch == TEXT('i') || ch == TEXT('a')) {
                    /* Store the inner/around modifier and wait for object type */
                    g_VimState.szLastCommand[0] = g_VimState.chPendingOp;
                    g_VimState.szLastCommand[1] = ch;
                    g_VimState.szLastCommand[2] = TEXT('\0');
                    g_VimState.chPendingOp = (ch == TEXT('i')) ? TEXT('I') : TEXT('A'); /* Use I/A as markers */
                    return TRUE;
                }
            }
            
            /* Handle text object type after i or a */
            if (g_VimState.chPendingOp == TEXT('I') || g_VimState.chPendingOp == TEXT('A')) {
                TCHAR chOp = g_VimState.szLastCommand[0];
                BOOL bInner = (g_VimState.chPendingOp == TEXT('I'));
                TextObjectRange range = {0, 0, FALSE};
                
                switch (ch) {
                    case TEXT('w'):
                        range = bInner ? VimSelectInnerWord(hwndEdit) : VimSelectAWord(hwndEdit);
                        break;
                    case TEXT('"'):
                        range = bInner ? VimSelectInnerQuote(hwndEdit, TEXT('"')) : VimSelectAQuote(hwndEdit, TEXT('"'));
                        break;
                    case TEXT('\''):
                        range = bInner ? VimSelectInnerQuote(hwndEdit, TEXT('\'')) : VimSelectAQuote(hwndEdit, TEXT('\''));
                        break;
                    case TEXT('('):
                    case TEXT(')'):
                    case TEXT('b'):
                        range = bInner ? VimSelectInnerBracket(hwndEdit, TEXT('('), TEXT(')')) : VimSelectABracket(hwndEdit, TEXT('('), TEXT(')'));
                        break;
                    case TEXT('{'):
                    case TEXT('}'):
                    case TEXT('B'):
                        range = bInner ? VimSelectInnerBracket(hwndEdit, TEXT('{'), TEXT('}')) : VimSelectABracket(hwndEdit, TEXT('{'), TEXT('}'));
                        break;
                    case TEXT('['):
                    case TEXT(']'):
                        range = bInner ? VimSelectInnerBracket(hwndEdit, TEXT('['), TEXT(']')) : VimSelectABracket(hwndEdit, TEXT('['), TEXT(']'));
                        break;
                    case TEXT('<'):
                    case TEXT('>'):
                        range = bInner ? VimSelectInnerBracket(hwndEdit, TEXT('<'), TEXT('>')) : VimSelectABracket(hwndEdit, TEXT('<'), TEXT('>'));
                        break;
                }
                
                if (range.bFound) {
                    VimApplyTextObject(hwndEdit, chOp, range);
                }
                g_VimState.chPendingOp = 0;
                g_VimState.nRepeatCount = 0;
                return TRUE;
            }
            
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
                    else if (ch == TEXT('e')) {
                        /* ge - move to end of previous word */
                        VimMoveWordEndBackward(hwndEdit, count);
                    }
                    else if (ch == TEXT('E')) {
                        /* gE - move to end of previous WORD */
                        VimMoveWORDEndBackward(hwndEdit, count);
                    }
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
                case TEXT('r'):
                    /* r{char} - replace character under cursor */
                    VimReplaceChar(hwndEdit, ch);
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
            case TEXT('W'): VimMoveWORDForward(hwndEdit, count); break;
            case TEXT('b'): VimMoveWordBackward(hwndEdit, count); break;
            case TEXT('B'): VimMoveWORDBackward(hwndEdit, count); break;
            case TEXT('e'): VimMoveWordEnd(hwndEdit, count); break;
            case TEXT('E'): VimMoveWORDEnd(hwndEdit, count); break;
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
            case TEXT('H'): VimMoveScreenTop(hwndEdit); break;
            case TEXT('M'): VimMoveScreenMiddle(hwndEdit); break;
            case TEXT('L'): VimMoveScreenBottom(hwndEdit); break;
            case TEXT('+'): VimMoveNextLineFirstNonBlank(hwndEdit, count); break;
            case TEXT('-'): VimMovePrevLineFirstNonBlank(hwndEdit, count); break;
            case 13: /* Enter key */ VimMoveNextLineFirstNonBlank(hwndEdit, count); break;
            
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
            case TEXT('r'): g_VimState.chPendingOp = TEXT('r'); return TRUE;
            case TEXT('~'): VimToggleCase(hwndEdit); break;
            
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
