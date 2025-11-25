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
    g_VimState.nVisualStartLine = 0;
    g_VimState.szCommandBuffer[0] = TEXT('\0');
    g_VimState.nCommandLen = 0;
    g_VimState.szSearchPattern[0] = TEXT('\0');
    g_VimState.bSearchForward = TRUE;
    g_VimState.nDesiredCol = 0;
}

/* Toggle vim mode on/off */
void ToggleVimMode(HWND hwnd) {
    g_VimState.bEnabled = !g_VimState.bEnabled;
    if (g_VimState.bEnabled) {
        g_VimState.mode = VIM_MODE_NORMAL;
    }
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_VIEW_VIMMODE, 
                  g_VimState.bEnabled ? MF_CHECKED : MF_UNCHECKED);
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
        default: return TEXT("");
    }
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

/* Get text buffer */
static TCHAR* GetTextBuffer(HWND hwndEdit, DWORD* pLen) {
    DWORD textLen = GetTextLen(hwndEdit);
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (textLen + 1) * sizeof(TCHAR));
    if (pText) {
        GetWindowText(hwndEdit, pText, textLen + 1);
        if (pLen) *pLen = textLen;
    }
    return pText;
}

static void FreeTextBuffer(TCHAR* pText) {
    if (pText) HeapFree(GetProcessHeap(), 0, pText);
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
    
    /* In Visual Line mode, use tracked current line */
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
    
    /* In Visual Line mode, use tracked current line */
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
    g_VimState.nDesiredCol = 9999; /* Large number for end of line */
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
        /* Skip current word */
        while (pos < textLen && (IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_'))) pos++;
        /* Skip whitespace/punctuation */
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
        /* Skip whitespace before cursor */
        while (pos > 0 && !IsCharAlphaNumeric(pText[pos - 1]) && pText[pos - 1] != TEXT('_')) pos--;
        /* Skip word */
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
        pos++; /* Move at least one char */
        /* Skip whitespace */
        while (pos < textLen && !IsCharAlphaNumeric(pText[pos]) && pText[pos] != TEXT('_')) pos++;
        /* Move to end of word */
        while (pos < textLen && (IsCharAlphaNumeric(pText[pos]) || pText[pos] == TEXT('_'))) pos++;
        if (pos > 0) pos--; /* Back to last char of word */
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

void VimRedo(HWND hwndEdit) { 
    /* RichEdit supports redo with EM_REDO */
    SendMessage(hwndEdit, EM_REDO, 0, 0); 
}

void VimJoinLines(HWND hwndEdit) {
    DWORD pos = GetEditCursorPos(hwndEdit);
    int line = GetLineFromChar(hwndEdit, pos);
    int totalLines = GetLineCount(hwndEdit);
    if (line >= totalLines - 1) return;
    
    /* Go to end of current line */
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    int lineLen = GetLineLen(hwndEdit, line);
    DWORD lineEnd = lineStart + lineLen;
    
    /* Select newline and leading whitespace of next line */
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
                /* Remove up to 4 spaces */
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
    DWORD pos = GetEditCursorPos(hwndEdit);
    SetEditCursorPos(hwndEdit, pos);
    
    /* Set desired column to current position */
    int line = GetLineFromChar(hwndEdit, pos);
    DWORD lineStart = GetLineIndex(hwndEdit, line);
    g_VimState.nDesiredCol = (int)(pos - lineStart);
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

void VimSearchNext(HWND hwndEdit) {
    if (g_VimState.szSearchPattern[0] == TEXT('\0')) return;
    
    DWORD textLen;
    TCHAR* pText = GetTextBuffer(hwndEdit, &textLen);
    if (!pText) return;
    
    DWORD pos = GetEditCursorPos(hwndEdit);
    TCHAR* found = NULL;
    
    if (g_VimState.bSearchForward) {
        if (pos + 1 < textLen) {
            found = _tcsstr(pText + pos + 1, g_VimState.szSearchPattern);
        }
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
                lastFound = search; search++;
            }
            found = lastFound;
        }
    }
    
    if (found) SetEditCursorPos(hwndEdit, (DWORD)(found - pText));
    FreeTextBuffer(pText);
}

void VimSearchPrev(HWND hwndEdit) {
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
    VimSearchNext(hwndEdit);
    g_VimState.bSearchForward = !g_VimState.bSearchForward;
}


/* ============ Key Processing ============ */

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
    BOOL bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    BOOL bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    
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
            
            /* Ctrl+D = half page down, Ctrl+U = half page up */
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
                /* Navigation - all work in V-LINE mode */
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
                /* Edit operations */
                case TEXT('d'):
                case TEXT('x'): VimVisualDelete(hwndEdit); return TRUE;
                case TEXT('y'): VimVisualYank(hwndEdit); return TRUE;
                case TEXT('c'): VimVisualChange(hwndEdit); return TRUE;
                case TEXT('>'): VimIndentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('<'): VimUnindentLine(hwndEdit, count); VimEnterNormalMode(hwndEdit); return TRUE;
                case TEXT('v'): /* Switch to character visual */
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
                case TEXT('V'): /* Switch to line visual */
                    VimEnterVisualLineMode(hwndEdit);
                    return TRUE;
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
            case TEXT('W'): VimMoveWordForward(hwndEdit, count); break; /* Same as w for now */
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
            
            /* Mode transitions */
            case TEXT('i'): VimEnterInsertMode(hwndEdit); break;
            case TEXT('a'): VimEnterInsertModeAfter(hwndEdit); break;
            case TEXT('I'): VimEnterInsertModeLineStart(hwndEdit); break;
            case TEXT('A'): VimEnterInsertModeLineEnd(hwndEdit); break;
            case TEXT('o'): VimEnterInsertModeNewLineBelow(hwndEdit); break;
            case TEXT('O'): VimEnterInsertModeNewLineAbove(hwndEdit); break;
            case TEXT('v'): VimEnterVisualMode(hwndEdit); break;
            case TEXT('V'): VimEnterVisualLineMode(hwndEdit); break;
            
            /* Search */
            case TEXT('n'): VimSearchNext(hwndEdit); break;
            case TEXT('N'): VimSearchPrev(hwndEdit); break;
            
            default: g_VimState.nRepeatCount = 0; return FALSE;
        }
        g_VimState.nRepeatCount = 0;
        return TRUE;
    }
    return FALSE;
}
