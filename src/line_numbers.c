#include "notepad.h"

/* Line number window class name */
static const TCHAR szLineNumClassName[] = TEXT("LineNumberWindow");

/* Default line number width */
#define DEFAULT_LINE_NUM_WIDTH 35
#define LINE_NUM_PADDING 4



/* Register line number window class */
static BOOL RegisterLineNumberClass(HINSTANCE hInstance) {
    static BOOL bRegistered = FALSE;
    
    if (bRegistered) return TRUE;
    
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = LineNumberWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(LONG_PTR); /* Store edit control handle */
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szLineNumClassName;
    wc.hIconSm       = NULL;
    
    if (RegisterClassEx(&wc)) {
        bRegistered = TRUE;
        return TRUE;
    }
    return FALSE;
}

/* Calculate line number width based on line count */
int CalculateLineNumberWidth(int nLineCount) {
    int nDigits = 1;
    int n = nLineCount;
    
    while (n >= 10) {
        nDigits++;
        n /= 10;
    }
    
    /* Minimum 2 digits width */
    if (nDigits < 2) nDigits = 2;
    
    /* Character width ~8 pixels for Consolas 16pt, plus padding for right margin */
    return (nDigits * 8) + 16;
}


/* Get line count from edit control */
static int GetEditLineCount(HWND hwndEdit) {
    if (!hwndEdit) return 1;
    int nLines = (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    return (nLines > 0) ? nLines : 1;
}

/* Get first visible line in edit control */
static int GetFirstVisibleLine(HWND hwndEdit) {
    if (!hwndEdit) return 0;
    return (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
}

/* Create line number window */
HWND CreateLineNumberWindow(HWND hwndParent, HINSTANCE hInstance) {
    if (!RegisterLineNumberClass(hInstance)) {
        return NULL;
    }
    
    HWND hwndLineNum = CreateWindowEx(
        0,
        szLineNumClassName,
        TEXT(""),
        WS_CHILD,  /* Initially hidden */
        0, 0, DEFAULT_LINE_NUM_WIDTH, 100,
        hwndParent,
        (HMENU)IDC_LINENUMBERS,
        hInstance,
        NULL
    );
    
    return hwndLineNum;
}

/* Count logical lines (based on newline characters) */
static int CountLogicalLines(HWND hwndEdit) {
    int nTextLen = GetWindowTextLength(hwndEdit);
    if (nTextLen == 0) return 1;
    
    /* For performance, use EM_GETLINECOUNT for non-wrapped mode */
    /* In wrapped mode, we need to count actual newlines */
    int nLines = 1;
    
    /* Get text and count newlines */
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nTextLen + 1) * sizeof(TCHAR));
    if (pText) {
        GetWindowText(hwndEdit, pText, nTextLen + 1);
        for (int i = 0; i < nTextLen; i++) {
            if (pText[i] == TEXT('\n')) {
                nLines++;
            }
        }
        HeapFree(GetProcessHeap(), 0, pText);
    }
    
    return nLines;
}

/* Get character index for logical line (counting newlines) */
static int GetLogicalLineCharIndex(HWND hwndEdit, int nLogicalLine) {
    if (nLogicalLine == 0) return 0;
    
    int nTextLen = GetWindowTextLength(hwndEdit);
    if (nTextLen == 0) return 0;
    
    TCHAR* pText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nTextLen + 1) * sizeof(TCHAR));
    if (!pText) return 0;
    
    GetWindowText(hwndEdit, pText, nTextLen + 1);
    
    int nCurrentLine = 0;
    int nCharIndex = 0;
    
    for (int i = 0; i < nTextLen && nCurrentLine < nLogicalLine; i++) {
        if (pText[i] == TEXT('\n')) {
            nCurrentLine++;
            if (nCurrentLine == nLogicalLine) {
                nCharIndex = i + 1;
                break;
            }
        }
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    return nCharIndex;
}

/* Line number window procedure */
LRESULT CALLBACK LineNumberWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
            
            /* Get associated edit control from parent's current tab */
            HWND hwndEdit = GetCurrentEdit();
            if (!hwndEdit) {
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            /* Get client rect */
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int nWidth = rcClient.right - rcClient.left;
            int nHeight = rcClient.bottom - rcClient.top;
            
            if (nWidth <= 0 || nHeight <= 0) {
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            /* Create double buffer */
            HDC hdc = CreateCompatibleDC(hdcScreen);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, nWidth, nHeight);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
            
            /* Fill background with white (like Notepad++) */
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rcClient, hBrush);
            DeleteObject(hBrush);
            
            /* Draw thin separator line on right edge */
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, rcClient.right - 1, rcClient.top, NULL);
            LineTo(hdc, rcClient.right - 1, rcClient.bottom);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            /* Use the SAME font as edit control for proper alignment */
            HFONT hEditFont = (HFONT)SendMessage(hwndEdit, WM_GETFONT, 0, 0);
            HFONT hOldFont = NULL;
            if (hEditFont) {
                hOldFont = (HFONT)SelectObject(hdc, hEditFont);
            }
            
            /* Get line height using edit control's font */
            TEXTMETRIC tm;
            GetTextMetrics(hdc, &tm);
            int nLineHeight = tm.tmHeight;
            
            /* Get total logical lines (based on newlines, not visual wrapping) */
            int nTotalLines = CountLogicalLines(hwndEdit);
            
            /* Set text properties - dark gray like Notepad++ */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(80, 80, 80));
            
            TCHAR szLineNum[16];
            RECT rcLine;
            rcLine.left = 0;
            rcLine.right = rcClient.right - 6;
            
            /* Track last Y position to avoid drawing at same position */
            int nLastY = -10000;
            
            for (int nLine = 0; nLine < nTotalLines; nLine++) {
                /* Get character index at start of this logical line */
                int nCharIndex = GetLogicalLineCharIndex(hwndEdit, nLine);
                
                /* Get Y position of this character */
                LRESULT lPos = SendMessage(hwndEdit, EM_POSFROMCHAR, nCharIndex, 0);
                
                /* Extract Y coordinate - handle as signed short for proper negative values */
                short nY = (short)HIWORD(lPos);
                
                /* Skip if line is above visible area (scrolled past) */
                if (nY < -nLineHeight) continue;
                
                /* Stop if line is below visible area */
                if (nY > nHeight) break;
                
                /* Skip if same Y position as last line */
                if (nY == nLastY) continue;
                nLastY = nY;
                
                /* Draw line number at correct Y position - align with text baseline */
                rcLine.top = (int)nY;
                rcLine.bottom = rcLine.top + nLineHeight;
                
                _sntprintf(szLineNum, 16, TEXT("%d"), nLine + 1);
                DrawText(hdc, szLineNum, -1, &rcLine, DT_RIGHT | DT_TOP | DT_SINGLELINE);
            }
            
            if (hOldFont) {
                SelectObject(hdc, hOldFont);
            }
            
            /* Copy buffer to screen */
            BitBlt(hdcScreen, 0, 0, nWidth, nHeight, hdc, 0, 0, SRCCOPY);
            
            /* Cleanup */
            SelectObject(hdc, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdc);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1; /* We handle background in WM_PAINT */
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


/* Update line numbers display */
void UpdateLineNumbers(HWND hwndLineNumbers, HWND hwndEdit) {
    (void)hwndEdit; /* Unused - we get edit from GetCurrentEdit() */
    if (!hwndLineNumbers || !IsWindowVisible(hwndLineNumbers)) return;
    
    /* Invalidate to trigger repaint */
    InvalidateRect(hwndLineNumbers, NULL, TRUE);
}

/* Sync line number scroll with edit control */
void SyncLineNumberScroll(HWND hwndLineNumbers, HWND hwndEdit) {
    (void)hwndEdit; /* Unused - we get edit from GetCurrentEdit() */
    if (!hwndLineNumbers || !IsWindowVisible(hwndLineNumbers)) return;
    
    /* Invalidate without erasing background for smoother redraw */
    InvalidateRect(hwndLineNumbers, NULL, FALSE);
}

/* Toggle line numbers visibility */
void ToggleLineNumbers(HWND hwnd) {
    g_AppState.bShowLineNumbers = !g_AppState.bShowLineNumbers;
    
    /* Update menu check mark */
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_VIEW_LINENUMBERS, 
                  g_AppState.bShowLineNumbers ? MF_CHECKED : MF_UNCHECKED);
    
    /* Update current tab's line number window */
    TabState* pTab = GetCurrentTabState();
    if (pTab) {
        if (g_AppState.bShowLineNumbers) {
            /* Create line number window if not exists */
            if (!pTab->lineNumState.hwndLineNumbers) {
                pTab->lineNumState.hwndLineNumbers = CreateLineNumberWindow(hwnd, g_AppState.hInstance);
            }
            
            /* Calculate width based on line count */
            int nLines = GetEditLineCount(pTab->hwndEdit);
            pTab->lineNumState.nLineNumberWidth = CalculateLineNumberWidth(nLines);
            pTab->lineNumState.bShowLineNumbers = TRUE;
            
            ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_SHOW);
            
            /* Start periodic sync timer - 100ms is enough for smooth sync */
            SetTimer(hwnd, 2, 100, NULL);
        } else {
            pTab->lineNumState.bShowLineNumbers = FALSE;
            if (pTab->lineNumState.hwndLineNumbers) {
                ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_HIDE);
            }
            
            /* Stop periodic sync timer */
            KillTimer(hwnd, 2);
        }
    }
    
    /* Reposition controls */
    RepositionControls(hwnd);
}

/* Tab control height - must match main.c */
#define TAB_HEIGHT 28

/* Status bar height */
#define STATUS_HEIGHT 22

/* Reposition controls based on line number visibility */
void RepositionControls(HWND hwnd) {
    if (!hwnd || !g_AppState.hwndTab) return;
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    /* Minimum size check */
    if (rc.right < 100 || rc.bottom < 100) return;
    
    /* Tab control at top */
    int nTabHeight = TAB_HEIGHT;
    
    /* Get status bar height */
    int nStatusHeight = 0;
    if (g_AppState.hwndStatus) {
        RECT rcStatus;
        GetWindowRect(g_AppState.hwndStatus, &rcStatus);
        nStatusHeight = rcStatus.bottom - rcStatus.top;
        
        /* Update status bar parts for new width */
        SetStatusBarParts(g_AppState.hwndStatus, rc.right);
    }
    
    /* Calculate edit area height */
    int nEditAreaHeight = rc.bottom - nTabHeight - nStatusHeight;
    if (nEditAreaHeight < 50) nEditAreaHeight = 50;
    
    /* Use DeferWindowPos for smoother resize */
    HDWP hdwp = BeginDeferWindowPos(4);
    if (!hdwp) {
        /* Fallback to regular MoveWindow */
        MoveWindow(g_AppState.hwndTab, 0, 0, rc.right, nTabHeight, FALSE);
        
        if (g_AppState.hwndStatus) {
            SendMessage(g_AppState.hwndStatus, WM_SIZE, 0, 0);
        }
        
        TabState* pTab = GetCurrentTabState();
        if (pTab && pTab->hwndEdit) {
            int nEditLeft = 0;
            int nEditWidth = rc.right;
            
            if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                int nLineNumWidth = pTab->lineNumState.nLineNumberWidth;
                if (nLineNumWidth <= 0) nLineNumWidth = DEFAULT_LINE_NUM_WIDTH;
                MoveWindow(pTab->lineNumState.hwndLineNumbers, 0, nTabHeight, nLineNumWidth, nEditAreaHeight, TRUE);
                ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_SHOW);
                nEditLeft = nLineNumWidth;
                nEditWidth = rc.right - nLineNumWidth;
            }
            MoveWindow(pTab->hwndEdit, nEditLeft, nTabHeight, nEditWidth, nEditAreaHeight, TRUE);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }
    
    hdwp = DeferWindowPos(hdwp, g_AppState.hwndTab, NULL, 0, 0, rc.right, nTabHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    
    /* Status bar auto-positions itself, just send WM_SIZE */
    if (g_AppState.hwndStatus) {
        SendMessage(g_AppState.hwndStatus, WM_SIZE, 0, 0);
    }
    
    TabState* pTab = GetCurrentTabState();
    if (!pTab) {
        EndDeferWindowPos(hdwp);
        return;
    }
    
    int nEditLeft = 0;
    int nEditWidth = rc.right;
    
    if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
        int nLineNumWidth = pTab->lineNumState.nLineNumberWidth;
        if (nLineNumWidth <= 0) nLineNumWidth = DEFAULT_LINE_NUM_WIDTH;
        
        /* Position and show line number window */
        hdwp = DeferWindowPos(hdwp, pTab->lineNumState.hwndLineNumbers, NULL,
                   0, nTabHeight, 
                   nLineNumWidth, nEditAreaHeight, 
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        
        nEditLeft = nLineNumWidth;
        nEditWidth = rc.right - nLineNumWidth;
    }
    
    /* Position edit control */
    if (pTab->hwndEdit) {
        hdwp = DeferWindowPos(hdwp, pTab->hwndEdit, NULL,
                   nEditLeft, nTabHeight, 
                   nEditWidth, nEditAreaHeight, 
                   SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    EndDeferWindowPos(hdwp);
    
    /* Refresh line numbers */
    if (pTab->lineNumState.hwndLineNumbers && g_AppState.bShowLineNumbers) {
        InvalidateRect(pTab->lineNumState.hwndLineNumbers, NULL, TRUE);
    }
}
