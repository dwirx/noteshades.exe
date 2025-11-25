#include "notepad.h"
#include "syntax.h"
#include <richedit.h>

/* Global zoom level */
static int g_nZoomLevel = 100;
static int g_nBaseFontSize = 16;

/* Undo last edit operation */
void EditUndo(HWND hEdit) {
    SendMessage(hEdit, EM_UNDO, 0, 0);
}

/* Cut selected text to clipboard */
void EditCut(HWND hEdit) {
    SendMessage(hEdit, WM_CUT, 0, 0);
}

/* Copy selected text to clipboard */
void EditCopy(HWND hEdit) {
    SendMessage(hEdit, WM_COPY, 0, 0);
}

/* Paste clipboard content at cursor position */
void EditPaste(HWND hEdit) {
    SendMessage(hEdit, WM_PASTE, 0, 0);
}

/* Select all text in edit control */
void EditSelectAll(HWND hEdit) {
    SendMessage(hEdit, EM_SETSEL, 0, -1);
}

/* Duplicate current line */
void EditDuplicateLine(HWND hEdit) {
    if (!hEdit) return;
    
    /* Get current cursor position */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* Get current line */
    int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
    int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
    
    /* Get line text */
    TCHAR* szLine = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (!szLine) return;
    
    *(WORD*)szLine = (WORD)(nLineLen + 1);
    SendMessage(hEdit, EM_GETLINE, nLine, (LPARAM)szLine);
    szLine[nLineLen] = TEXT('\0');
    
    /* Move to end of line and insert newline + duplicate */
    DWORD dwLineEnd = dwLineStart + nLineLen;
    SendMessage(hEdit, EM_SETSEL, dwLineEnd, dwLineEnd);
    
    /* Create duplicate string with newline */
    TCHAR* szDup = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (szDup) {
        _sntprintf(szDup, nLineLen + 4, TEXT("\r\n%s"), szLine);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDup);
        HeapFree(GetProcessHeap(), 0, szDup);
    }
    
    /* Restore cursor position on new line */
    int nCol = dwStart - dwLineStart;
    DWORD dwNewLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine + 1, 0);
    SendMessage(hEdit, EM_SETSEL, dwNewLineStart + nCol, dwNewLineStart + nCol);
    
    HeapFree(GetProcessHeap(), 0, szLine);
}

/* Delete current line */
void EditDeleteLine(HWND hEdit) {
    if (!hEdit) return;
    
    /* Get current cursor position */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* Get current line */
    int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    int nTotalLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
    
    /* Calculate selection range */
    DWORD dwDeleteEnd;
    if (nLine + 1 < nTotalLines) {
        dwDeleteEnd = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine + 1, 0);
    } else {
        /* Last line - delete from previous newline if exists */
        int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
        dwDeleteEnd = dwLineStart + nLineLen;
        if (nLine > 0) {
            DWORD dwPrevLineEnd = dwLineStart;
            dwLineStart = dwPrevLineEnd - 2; /* Include \r\n */
            if ((int)dwLineStart < 0) dwLineStart = 0;
        }
    }
    
    /* Delete the line */
    SendMessage(hEdit, EM_SETSEL, dwLineStart, dwDeleteEnd);
    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
}

/* Move current line up */
void EditMoveLineUp(HWND hEdit) {
    if (!hEdit) return;
    
    /* Get current cursor position */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* Get current line */
    int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    if (nLine == 0) return; /* Already at top */
    
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
    int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
    int nTotalLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
    
    /* Get current line text */
    TCHAR* szLine = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (!szLine) return;
    
    *(WORD*)szLine = (WORD)(nLineLen + 1);
    SendMessage(hEdit, EM_GETLINE, nLine, (LPARAM)szLine);
    szLine[nLineLen] = TEXT('\0');
    
    /* Delete current line (including newline before it) */
    DWORD dwDeleteStart = dwLineStart - 2; /* \r\n before line */
    if ((int)dwDeleteStart < 0) dwDeleteStart = 0;
    
    DWORD dwDeleteEnd;
    if (nLine + 1 < nTotalLines) {
        dwDeleteEnd = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine + 1, 0);
    } else {
        dwDeleteEnd = dwLineStart + nLineLen;
    }
    
    /* Delete current line */
    SendMessage(hEdit, EM_SETSEL, dwLineStart, dwDeleteEnd);
    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    
    /* Insert at previous line position */
    DWORD dwPrevLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine - 1, 0);
    SendMessage(hEdit, EM_SETSEL, dwPrevLineStart, dwPrevLineStart);
    
    TCHAR* szInsert = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (szInsert) {
        _sntprintf(szInsert, nLineLen + 4, TEXT("%s\r\n"), szLine);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szInsert);
        HeapFree(GetProcessHeap(), 0, szInsert);
    }
    
    /* Restore cursor position */
    int nCol = dwStart - dwLineStart;
    SendMessage(hEdit, EM_SETSEL, dwPrevLineStart + nCol, dwPrevLineStart + nCol);
    
    HeapFree(GetProcessHeap(), 0, szLine);
}

/* Move current line down */
void EditMoveLineDown(HWND hEdit) {
    if (!hEdit) return;
    
    /* Get current cursor position */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* Get current line */
    int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    int nTotalLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
    if (nLine >= nTotalLines - 1) return; /* Already at bottom */
    
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
    int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
    
    /* Get current line text */
    TCHAR* szLine = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (!szLine) return;
    
    *(WORD*)szLine = (WORD)(nLineLen + 1);
    SendMessage(hEdit, EM_GETLINE, nLine, (LPARAM)szLine);
    szLine[nLineLen] = TEXT('\0');
    
    /* Delete current line */
    DWORD dwNextLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine + 1, 0);
    SendMessage(hEdit, EM_SETSEL, dwLineStart, dwNextLineStart);
    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    
    /* Insert after next line */
    int nNextLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
    DWORD dwInsertPos = dwLineStart + nNextLineLen;
    SendMessage(hEdit, EM_SETSEL, dwInsertPos, dwInsertPos);
    
    TCHAR* szInsert = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (szInsert) {
        _sntprintf(szInsert, nLineLen + 4, TEXT("\r\n%s"), szLine);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szInsert);
        HeapFree(GetProcessHeap(), 0, szInsert);
    }
    
    /* Restore cursor position */
    int nCol = dwStart - dwLineStart;
    DWORD dwNewLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine + 1, 0);
    SendMessage(hEdit, EM_SETSEL, dwNewLineStart + nCol, dwNewLineStart + nCol);
    
    HeapFree(GetProcessHeap(), 0, szLine);
}

/* Toggle comment on current line */
void EditToggleComment(HWND hEdit) {
    if (!hEdit) return;
    
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return;
    
    /* Determine comment prefix based on language */
    const TCHAR* szComment = TEXT("// ");
    int nCommentLen = 3;
    
    switch (pTab->language) {
        case LANG_PYTHON:
        case LANG_RUBY:
        case LANG_SHELL:
        case LANG_POWERSHELL:
        case LANG_YAML:
            szComment = TEXT("# ");
            nCommentLen = 2;
            break;
        case LANG_HTML:
        case LANG_XML:
            szComment = TEXT("<!-- ");
            nCommentLen = 5;
            break;
        case LANG_CSS:
            szComment = TEXT("/* ");
            nCommentLen = 3;
            break;
        case LANG_BATCH:
            szComment = TEXT("REM ");
            nCommentLen = 4;
            break;
        case LANG_SQL:
            szComment = TEXT("-- ");
            nCommentLen = 3;
            break;
        default:
            break;
    }
    
    /* Get current line */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
    int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
    
    /* Get line text */
    TCHAR* szLine = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLineLen + 4) * sizeof(TCHAR));
    if (!szLine) return;
    
    *(WORD*)szLine = (WORD)(nLineLen + 1);
    SendMessage(hEdit, EM_GETLINE, nLine, (LPARAM)szLine);
    szLine[nLineLen] = TEXT('\0');
    
    /* Find first non-whitespace */
    int nFirstChar = 0;
    while (nFirstChar < nLineLen && (szLine[nFirstChar] == TEXT(' ') || szLine[nFirstChar] == TEXT('\t'))) {
        nFirstChar++;
    }
    
    /* Check if line starts with comment */
    BOOL bHasComment = (_tcsncmp(szLine + nFirstChar, szComment, nCommentLen) == 0);
    
    if (bHasComment) {
        /* Remove comment */
        SendMessage(hEdit, EM_SETSEL, dwLineStart + nFirstChar, dwLineStart + nFirstChar + nCommentLen);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
    } else {
        /* Add comment */
        SendMessage(hEdit, EM_SETSEL, dwLineStart + nFirstChar, dwLineStart + nFirstChar);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szComment);
    }
    
    HeapFree(GetProcessHeap(), 0, szLine);
}

/* Indent current line or selection */
void EditIndent(HWND hEdit) {
    if (!hEdit) return;
    
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    int nStartLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    int nEndLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwEnd, 0);
    
    /* Indent each line */
    for (int i = nStartLine; i <= nEndLine; i++) {
        DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, i, 0);
        SendMessage(hEdit, EM_SETSEL, dwLineStart, dwLineStart);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT("\t"));
    }
    
    /* Restore selection */
    DWORD dwNewStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nStartLine, 0);
    DWORD dwNewEnd = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nEndLine, 0);
    int nEndLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwNewEnd, 0);
    SendMessage(hEdit, EM_SETSEL, dwNewStart, dwNewEnd + nEndLineLen);
}

/* Unindent current line or selection */
void EditUnindent(HWND hEdit) {
    if (!hEdit) return;
    
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    int nStartLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwStart, 0);
    int nEndLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwEnd, 0);
    
    /* Unindent each line */
    for (int i = nStartLine; i <= nEndLine; i++) {
        DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, i, 0);
        int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
        
        if (nLineLen > 0) {
            TCHAR szFirst[2] = {0};
            *(WORD*)szFirst = 2;
            SendMessage(hEdit, EM_GETLINE, i, (LPARAM)szFirst);
            
            if (szFirst[0] == TEXT('\t')) {
                SendMessage(hEdit, EM_SETSEL, dwLineStart, dwLineStart + 1);
                SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            } else if (szFirst[0] == TEXT(' ')) {
                /* Remove up to 4 spaces */
                TCHAR szLine[8] = {0};
                *(WORD*)szLine = 5;
                SendMessage(hEdit, EM_GETLINE, i, (LPARAM)szLine);
                int nSpaces = 0;
                while (nSpaces < 4 && szLine[nSpaces] == TEXT(' ')) nSpaces++;
                if (nSpaces > 0) {
                    SendMessage(hEdit, EM_SETSEL, dwLineStart, dwLineStart + nSpaces);
                    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                }
            }
        }
    }
}

/* Go to specific line */
void EditGoToLine(HWND hEdit, int nLine) {
    if (!hEdit || nLine < 1) return;
    
    int nTotalLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
    if (nLine > nTotalLines) nLine = nTotalLines;
    
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine - 1, 0);
    SendMessage(hEdit, EM_SETSEL, dwLineStart, dwLineStart);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    SetFocus(hEdit);
}

/* Zoom functions */
int GetZoomLevel(void) {
    return g_nZoomLevel;
}

void SetZoomLevel(HWND hwnd, int nLevel) {
    if (nLevel < 50) nLevel = 50;
    if (nLevel > 300) nLevel = 300;
    g_nZoomLevel = nLevel;
    
    /* Update font size for all tabs */
    int nFontSize = (g_nBaseFontSize * g_nZoomLevel) / 100;
    
    HFONT hNewFont = CreateFont(
        nFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Consolas")
    );
    
    if (hNewFont) {
        for (int i = 0; i < g_AppState.nTabCount; i++) {
            if (g_AppState.tabs[i].hwndEdit) {
                SendMessage(g_AppState.tabs[i].hwndEdit, WM_SETFONT, (WPARAM)hNewFont, TRUE);
            }
        }
    }
    
    /* Update status bar */
    UpdateStatusBar(hwnd);
}

void ZoomIn(HWND hwnd) {
    SetZoomLevel(hwnd, g_nZoomLevel + 10);
}

void ZoomOut(HWND hwnd) {
    SetZoomLevel(hwnd, g_nZoomLevel - 10);
}

void ZoomReset(HWND hwnd) {
    SetZoomLevel(hwnd, 100);
}
