#include "notepad.h"
#include "syntax.h"
#include "vim_mode.h"

/* Create status bar */
HWND CreateStatusBar(HWND hwndParent, HINSTANCE hInstance) {
    HWND hwndStatus = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hwndParent,
        (HMENU)IDC_STATUS,
        hInstance,
        NULL
    );
    
    if (hwndStatus) {
        /* Set initial parts */
        RECT rc;
        GetClientRect(hwndParent, &rc);
        SetStatusBarParts(hwndStatus, rc.right);
    }
    
    return hwndStatus;
}

/* Set status bar parts based on window width */
void SetStatusBarParts(HWND hwndStatus, int nWidth) {
    if (!hwndStatus) return;
    
    int nParts[SB_PART_COUNT];
    int nRight = nWidth;
    
    /* Calculate part positions from right to left */
    nParts[SB_PART_VIMMODE] = nRight;
    nRight -= 70; /* Width for vim mode */
    
    nParts[SB_PART_INSERTMODE] = nRight;
    nRight -= SB_WIDTH_INSERTMODE;
    
    nParts[SB_PART_ENCODING] = nRight;
    nRight -= SB_WIDTH_ENCODING;
    
    nParts[SB_PART_LINEENDING] = nRight;
    nRight -= SB_WIDTH_LINEENDING;
    
    nParts[SB_PART_POSITION] = nRight;
    nRight -= SB_WIDTH_POSITION;
    
    nParts[SB_PART_LINES] = nRight;
    nRight -= SB_WIDTH_LINES;
    
    nParts[SB_PART_LENGTH] = nRight;
    nRight -= SB_WIDTH_LENGTH;
    
    nParts[SB_PART_FILETYPE] = nRight;
    
    SendMessage(hwndStatus, SB_SETPARTS, SB_PART_COUNT, (LPARAM)nParts);
}


/* Get file type string based on extension */
const TCHAR* GetFileTypeString(const TCHAR* szFileName) {
    if (!szFileName || szFileName[0] == TEXT('\0')) {
        return TEXT("Normal text file");
    }
    
    /* Find extension */
    const TCHAR* pExt = _tcsrchr(szFileName, TEXT('.'));
    if (!pExt) {
        return TEXT("Normal text file");
    }
    
    /* Check common extensions */
    if (_tcsicmp(pExt, TEXT(".txt")) == 0) return TEXT("Normal text file");
    if (_tcsicmp(pExt, TEXT(".c")) == 0) return TEXT("C source file");
    if (_tcsicmp(pExt, TEXT(".cpp")) == 0) return TEXT("C++ source file");
    if (_tcsicmp(pExt, TEXT(".h")) == 0) return TEXT("C/C++ header file");
    if (_tcsicmp(pExt, TEXT(".hpp")) == 0) return TEXT("C++ header file");
    if (_tcsicmp(pExt, TEXT(".py")) == 0) return TEXT("Python file");
    if (_tcsicmp(pExt, TEXT(".js")) == 0) return TEXT("JavaScript file");
    if (_tcsicmp(pExt, TEXT(".ts")) == 0) return TEXT("TypeScript file");
    if (_tcsicmp(pExt, TEXT(".html")) == 0) return TEXT("HTML file");
    if (_tcsicmp(pExt, TEXT(".htm")) == 0) return TEXT("HTML file");
    if (_tcsicmp(pExt, TEXT(".css")) == 0) return TEXT("CSS file");
    if (_tcsicmp(pExt, TEXT(".json")) == 0) return TEXT("JSON file");
    if (_tcsicmp(pExt, TEXT(".xml")) == 0) return TEXT("XML file");
    if (_tcsicmp(pExt, TEXT(".md")) == 0) return TEXT("Markdown file");
    if (_tcsicmp(pExt, TEXT(".java")) == 0) return TEXT("Java file");
    if (_tcsicmp(pExt, TEXT(".cs")) == 0) return TEXT("C# file");
    if (_tcsicmp(pExt, TEXT(".php")) == 0) return TEXT("PHP file");
    if (_tcsicmp(pExt, TEXT(".rb")) == 0) return TEXT("Ruby file");
    if (_tcsicmp(pExt, TEXT(".go")) == 0) return TEXT("Go file");
    if (_tcsicmp(pExt, TEXT(".rs")) == 0) return TEXT("Rust file");
    if (_tcsicmp(pExt, TEXT(".sql")) == 0) return TEXT("SQL file");
    if (_tcsicmp(pExt, TEXT(".sh")) == 0) return TEXT("Shell script");
    if (_tcsicmp(pExt, TEXT(".bat")) == 0) return TEXT("Batch file");
    if (_tcsicmp(pExt, TEXT(".ps1")) == 0) return TEXT("PowerShell file");
    if (_tcsicmp(pExt, TEXT(".ini")) == 0) return TEXT("INI file");
    if (_tcsicmp(pExt, TEXT(".cfg")) == 0) return TEXT("Config file");
    if (_tcsicmp(pExt, TEXT(".log")) == 0) return TEXT("Log file");
    if (_tcsicmp(pExt, TEXT(".rc")) == 0) return TEXT("Resource file");
    
    return TEXT("Normal text file");
}

/* Count words in edit control */
int CountWords(HWND hwndEdit) {
    if (!hwndEdit) return 0;
    
    int nLen = GetWindowTextLength(hwndEdit);
    if (nLen == 0) return 0;
    
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLen + 1) * sizeof(TCHAR));
    if (!pText) return 0;
    
    GetWindowText(hwndEdit, pText, nLen + 1);
    
    int nWords = 0;
    BOOL bInWord = FALSE;
    
    for (int i = 0; i < nLen; i++) {
        TCHAR ch = pText[i];
        BOOL bIsSpace = (ch == TEXT(' ') || ch == TEXT('\t') || 
                         ch == TEXT('\r') || ch == TEXT('\n'));
        
        if (bIsSpace) {
            bInWord = FALSE;
        } else if (!bInWord) {
            bInWord = TRUE;
            nWords++;
        }
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    return nWords;
}


/* Update status bar with current document info */
void UpdateStatusBar(HWND hwnd) {
    if (!g_AppState.hwndStatus) return;
    
    TabState* pTab = GetCurrentTabState();
    HWND hwndEdit = GetCurrentEdit();
    
    TCHAR szText[256];
    
    /* Part 0: File type / Language */
    if (pTab) {
        if (pTab->language != LANG_NONE && g_bSyntaxHighlight) {
            /* Show detected language */
            const TCHAR* szLang = GetLanguageName(pTab->language);
            SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_FILETYPE, (LPARAM)szLang);
        } else {
            const TCHAR* szFileType = GetFileTypeString(pTab->szFileName);
            SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_FILETYPE, (LPARAM)szFileType);
        }
    } else {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_FILETYPE, (LPARAM)TEXT("Plain Text"));
    }
    
    if (hwndEdit) {
        /* Part 1: Length (character count) */
        int nLength = GetWindowTextLength(hwndEdit);
        _sntprintf(szText, 256, TEXT("length: %d"), nLength);
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LENGTH, (LPARAM)szText);
        
        /* Part 2: Lines count */
        int nLines = (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
        _sntprintf(szText, 256, TEXT("lines: %d"), nLines);
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LINES, (LPARAM)szText);
        
        /* Part 3: Current position (Ln, Col, Pos) */
        DWORD dwStart = 0, dwEnd = 0;
        SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
        
        /* Get current line number */
        int nCurrentLine = (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, dwStart, 0);
        
        /* Get character index of line start */
        int nLineStart = (int)SendMessage(hwndEdit, EM_LINEINDEX, nCurrentLine, 0);
        
        /* Calculate column (1-based) */
        int nCol = dwStart - nLineStart + 1;
        
        _sntprintf(szText, 256, TEXT("Ln: %d  Col: %d  Pos: %d"), 
                   nCurrentLine + 1, nCol, dwStart);
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_POSITION, (LPARAM)szText);
    } else {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LENGTH, (LPARAM)TEXT("length: 0"));
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LINES, (LPARAM)TEXT("lines: 0"));
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_POSITION, (LPARAM)TEXT("Ln: 1  Col: 1  Pos: 0"));
    }
    
    /* Part 4: Line ending type */
    if (pTab) {
        const TCHAR* szLineEnding;
        switch (pTab->lineEnding) {
            case LINE_ENDING_LF:
                szLineEnding = TEXT("Unix (LF)");
                break;
            case LINE_ENDING_CR:
                szLineEnding = TEXT("Mac (CR)");
                break;
            case LINE_ENDING_CRLF:
            default:
                szLineEnding = TEXT("Windows (CRLF)");
                break;
        }
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LINEENDING, (LPARAM)szLineEnding);
    } else {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_LINEENDING, (LPARAM)TEXT("Windows (CRLF)"));
    }
    
    /* Part 5: Encoding (always UTF-8 for now) */
    SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_ENCODING, (LPARAM)TEXT("UTF-8"));
    
    /* Part 6: Insert/Overwrite mode */
    if (pTab) {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_INSERTMODE, 
                    (LPARAM)(pTab->bInsertMode ? TEXT("INS") : TEXT("OVR")));
    } else {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_INSERTMODE, (LPARAM)TEXT("INS"));
    }
    
    /* Part 7: Vim mode */
    if (IsVimModeEnabled()) {
        VimModeState vimMode = GetVimModeState();
        if (vimMode == VIM_MODE_COMMAND || vimMode == VIM_MODE_SEARCH) {
            /* Show command buffer */
            SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_VIMMODE, (LPARAM)GetVimCommandBuffer());
        } else {
            SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_VIMMODE, (LPARAM)GetVimModeString());
        }
    } else {
        SendMessage(g_AppState.hwndStatus, SB_SETTEXT, SB_PART_VIMMODE, (LPARAM)TEXT(""));
    }
}
