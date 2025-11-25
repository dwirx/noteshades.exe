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
    
    /* Character width ~8 pixels for Consolas 16pt, plus small padding */
    return (nDigits * 8) + LINE_NUM_PADDING + 10;
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

/* Line number window procedure */
LRESULT CALLBACK LineNumberWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            /* Get associated edit control from parent's current tab */
            HWND hwndEdit = GetCurrentEdit();
            if (!hwndEdit) {
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            /* Get client rect */
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            /* Fill background with light gray */
            HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
            FillRect(hdc, &rcClient, hBrush);
            DeleteObject(hBrush);
            
            /* Draw separator line */
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
            
            /* Get first visible line and total lines */
            int nFirstLine = GetFirstVisibleLine(hwndEdit);
            int nTotalLines = GetEditLineCount(hwndEdit);
            
            /* Calculate visible lines */
            int nVisibleLines = (rcClient.bottom - rcClient.top) / nLineHeight + 2;
            
            /* Set text properties */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(100, 100, 100));
            
            /* 
             * Calculate top offset to match edit control's text position
             * Edit control has WS_EX_CLIENTEDGE which adds a 2-pixel border
             * Plus internal padding of about 1 pixel
             */
            int nTopOffset = 3;  /* Border (2) + internal padding (1) */
            
            /* Draw line numbers */
            TCHAR szLineNum[16];
            RECT rcLine;
            rcLine.left = 2;
            rcLine.right = rcClient.right - 4;
            
            for (int i = 0; i < nVisibleLines && (nFirstLine + i) < nTotalLines; i++) {
                rcLine.top = nTopOffset + (i * nLineHeight);
                rcLine.bottom = rcLine.top + nLineHeight;
                
                _sntprintf(szLineNum, 16, TEXT("%d"), nFirstLine + i + 1);
                DrawText(hdc, szLineNum, -1, &rcLine, DT_RIGHT | DT_TOP | DT_SINGLELINE);
            }
            
            if (hOldFont) {
                SelectObject(hdc, hOldFont);
            }
            
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
    
    /* Just invalidate - the paint handler will get the current scroll position */
    InvalidateRect(hwndLineNumbers, NULL, TRUE);
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
        } else {
            pTab->lineNumState.bShowLineNumbers = FALSE;
            if (pTab->lineNumState.hwndLineNumbers) {
                ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_HIDE);
            }
        }
    }
    
    /* Reposition controls */
    RepositionControls(hwnd);
}

/* Tab control height - must match main.c */
#define TAB_HEIGHT 28

/* Reposition controls based on line number visibility */
void RepositionControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    /* Tab control at top */
    int nTabHeight = TAB_HEIGHT;
    MoveWindow(g_AppState.hwndTab, 0, 0, rc.right, nTabHeight, TRUE);
    
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return;
    
    int nEditLeft = 0;
    int nEditWidth = rc.right;
    
    if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
        int nLineNumWidth = pTab->lineNumState.nLineNumberWidth;
        if (nLineNumWidth <= 0) nLineNumWidth = DEFAULT_LINE_NUM_WIDTH;
        
        /* Position line number window */
        MoveWindow(pTab->lineNumState.hwndLineNumbers, 
                   0, nTabHeight, 
                   nLineNumWidth, rc.bottom - nTabHeight, 
                   TRUE);
        
        nEditLeft = nLineNumWidth;
        nEditWidth = rc.right - nLineNumWidth;
    }
    
    /* Position edit control */
    if (pTab->hwndEdit) {
        MoveWindow(pTab->hwndEdit, 
                   nEditLeft, nTabHeight, 
                   nEditWidth, rc.bottom - nTabHeight, 
                   TRUE);
    }
    
    /* Refresh line numbers */
    if (pTab->lineNumState.hwndLineNumbers && g_AppState.bShowLineNumbers) {
        InvalidateRect(pTab->lineNumState.hwndLineNumbers, NULL, TRUE);
    }
}
