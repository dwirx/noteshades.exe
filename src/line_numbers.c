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
    
    /* Minimum 3 digits width for better appearance */
    if (nDigits < 3) nDigits = 3;
    
    /* Character width ~9 pixels for Consolas 16pt, plus padding */
    return (nDigits * 9) + 20;
}


/* Get line count from edit control */
static int GetEditLineCount(HWND hwndEdit) {
    if (!hwndEdit) return 1;
    int nLines = (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    return (nLines > 0) ? nLines : 1;
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



/* Line number window procedure - OPTIMIZED for large files */
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
            
            /* Fill background with light gray */
            HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
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
            if (nLineHeight <= 0) nLineHeight = 16;
            
            /* Get first visible line and total line count */
            int nFirstVisible = (int)SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
            int nTotalLines = (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
            if (nTotalLines < 1) nTotalLines = 1;
            
            /* Calculate how many lines can fit in visible area */
            int nVisibleLines = (nHeight / nLineHeight) + 2;
            
            /* Set text properties - dark gray */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(100, 100, 100));
            
            /* Get current line for relative numbering */
            DWORD dwCursorPos = 0, dwCursorEnd = 0;
            SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwCursorPos, (LPARAM)&dwCursorEnd);
            int nCurrentLine = (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, dwCursorPos, 0);
            
            TCHAR szLineNum[16];
            RECT rcLine;
            rcLine.left = 4;
            rcLine.right = rcClient.right - 8;
            
            /* Draw line numbers at EXACT Y positions from edit control */
            for (int i = 0; i < nVisibleLines; i++) {
                int nLine = nFirstVisible + i;
                int nLineNum = nLine + 1; /* 1-based line number */
                
                /* Stop if we've passed the last line */
                if (nLineNum > nTotalLines) break;
                
                /* Get character index at start of this line */
                int nCharIndex = (int)SendMessage(hwndEdit, EM_LINEINDEX, nLine, 0);
                if (nCharIndex < 0) continue;
                
                /* Get EXACT Y position from edit control */
                LRESULT lPos = SendMessage(hwndEdit, EM_POSFROMCHAR, nCharIndex, 0);
                if (lPos == -1) continue;
                
                int nY = (short)HIWORD(lPos);
                
                /* Skip if above visible area */
                if (nY < -nLineHeight) continue;
                
                /* Stop if below visible area */
                if (nY > nHeight + nLineHeight) break;
                
                /* Draw line number at exact Y position */
                rcLine.top = nY;
                rcLine.bottom = nY + nLineHeight;
                
                /* Calculate display number based on relative mode */
                int nDisplayNum;
                if (g_AppState.bRelativeLineNumbers) {
                    if (nLine == nCurrentLine) {
                        /* Current line shows absolute number */
                        nDisplayNum = nLineNum;
                        /* Highlight current line number */
                        SetTextColor(hdc, RGB(0, 0, 0)); /* Black for current */
                    } else {
                        /* Other lines show relative distance */
                        nDisplayNum = abs(nLine - nCurrentLine);
                        SetTextColor(hdc, RGB(100, 100, 100)); /* Gray for relative */
                    }
                } else {
                    nDisplayNum = nLineNum;
                }
                
                _sntprintf(szLineNum, 16, TEXT("%d"), nDisplayNum);
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

/* Toggle relative line numbers */
void ToggleRelativeLineNumbers(HWND hwnd) {
    g_AppState.bRelativeLineNumbers = !g_AppState.bRelativeLineNumbers;
    
    /* Update menu check mark */
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_VIEW_RELATIVENUM, 
                  g_AppState.bRelativeLineNumbers ? MF_CHECKED : MF_UNCHECKED);
    
    /* Refresh line numbers display */
    TabState* pTab = GetCurrentTabState();
    if (pTab && pTab->lineNumState.hwndLineNumbers && g_AppState.bShowLineNumbers) {
        InvalidateRect(pTab->lineNumState.hwndLineNumbers, NULL, TRUE);
    }
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
            
            /* Start periodic sync timer - 50ms for smooth sync */
            SetTimer(hwnd, 2, 50, NULL);
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
                /* Position at same Y as edit control - we'll handle alignment in paint */
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
        
        /* Position at same Y as edit control - alignment handled in paint */
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
