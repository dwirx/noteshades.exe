#include "notepad.h"
#include "syntax.h"
#include <richedit.h>

/* Global application state */
AppState g_AppState = {0};

/* Window class name */
static const TCHAR szClassName[] = TEXT("NotepadMainWindow");

/* Font handle for edit control */
static HFONT g_hFont = NULL;

/* RichEdit library handle */
static HMODULE g_hRichEdit = NULL;

/* Original edit control window procedure */
static WNDPROC g_OrigEditProc = NULL;

/* Subclassed edit control procedure to catch scroll events */
static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_VSCROLL:
        case WM_HSCROLL:
        case WM_MOUSEWHEEL: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Then sync line numbers */
            TabState* pTab = GetCurrentTabState();
            if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
            }
            return result;
        }
        
        case WM_KEYDOWN:
        case WM_KEYUP: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Sync line numbers for navigation keys */
            if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR || 
                wParam == VK_NEXT || wParam == VK_HOME || wParam == VK_END) {
                TabState* pTab = GetCurrentTabState();
                if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                }
            }
            return result;
        }
        
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Sync line numbers after mouse click (might cause scroll) */
            TabState* pTab = GetCurrentTabState();
            if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
            }
            return result;
        }
        
        case EM_SCROLL:
        case EM_LINESCROLL:
        case WM_SIZE: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Sync line numbers */
            TabState* pTab = GetCurrentTabState();
            if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
            }
            return result;
        }
    }
    return CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
}

/* Tab control height */
#define TAB_HEIGHT 28

/* Close button size */
#define CLOSE_BTN_SIZE 16

/* Hover tracking for close button */
static int g_nHoverTab = -1;
static BOOL g_bHoverClose = FALSE;
static BOOL g_bTrackingMouse = FALSE;

/* Original tab control window procedure */
static WNDPROC g_OrigTabProc = NULL;

/* Subclassed tab control procedure to catch mouse events */
static LRESULT CALLBACK TabSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE: {
            /* Track mouse for close button hover effect */
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            
            /* Enable mouse leave tracking */
            if (!g_bTrackingMouse) {
                TRACKMOUSEEVENT tme = {0};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                g_bTrackingMouse = TRUE;
            }
            
            int nOldHoverTab = g_nHoverTab;
            BOOL bOldHoverClose = g_bHoverClose;
            g_nHoverTab = -1;
            g_bHoverClose = FALSE;
            
            TCHITTESTINFO htInfo;
            htInfo.pt = pt;
            int nTab = TabCtrl_HitTest(hwnd, &htInfo);
            
            if (nTab >= 0) {
                RECT rcItem;
                TabCtrl_GetItemRect(hwnd, nTab, &rcItem);
                int closeX = rcItem.right - CLOSE_BTN_SIZE - 4;
                
                if (pt.x >= closeX && pt.x <= rcItem.right) {
                    g_nHoverTab = nTab;
                    g_bHoverClose = TRUE;
                }
            }
            
            /* Redraw tab if hover state changed */
            if (nOldHoverTab != g_nHoverTab || bOldHoverClose != g_bHoverClose) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        
        case WM_MOUSELEAVE: {
            g_bTrackingMouse = FALSE;
            if (g_nHoverTab >= 0 || g_bHoverClose) {
                g_nHoverTab = -1;
                g_bHoverClose = FALSE;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        
        case WM_LBUTTONUP: {
            /* Check if click is on close button */
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            
            TCHITTESTINFO htInfo;
            htInfo.pt = pt;
            int nTab = TabCtrl_HitTest(hwnd, &htInfo);
            
            if (nTab >= 0) {
                RECT rcItem;
                TabCtrl_GetItemRect(hwnd, nTab, &rcItem);
                int closeX = rcItem.right - CLOSE_BTN_SIZE - 4;
                
                if (pt.x >= closeX) {
                    /* Get parent window and close tab */
                    HWND hwndParent = GetParent(hwnd);
                    CloseTab(hwndParent, nTab);
                    return 0;
                }
            }
            break;
        }
    }
    return CallWindowProc(g_OrigTabProc, hwnd, msg, wParam, lParam);
}

/* Get current edit control */
HWND GetCurrentEdit(void) {
    if (g_AppState.nCurrentTab >= 0 && g_AppState.nCurrentTab < g_AppState.nTabCount) {
        return g_AppState.tabs[g_AppState.nCurrentTab].hwndEdit;
    }
    return NULL;
}

/* Get current tab state */
TabState* GetCurrentTabState(void) {
    if (g_AppState.nCurrentTab >= 0 && g_AppState.nCurrentTab < g_AppState.nTabCount) {
        return &g_AppState.tabs[g_AppState.nCurrentTab];
    }
    return NULL;
}

/* Initialize tab state */
void InitTabState(TabState* pState) {
    pState->szFileName[0] = TEXT('\0');
    pState->bModified = FALSE;
    pState->bUntitled = TRUE;
    pState->hwndEdit = NULL;
    pState->pContent = NULL;
    pState->dwContentSize = 0;
    pState->lineNumState.bShowLineNumbers = FALSE;
    pState->lineNumState.hwndLineNumbers = NULL;
    pState->lineNumState.nLineNumberWidth = 0;
    pState->lineEnding = LINE_ENDING_CRLF;  /* Default Windows line ending */
    pState->bInsertMode = TRUE;              /* Default insert mode */
    pState->language = LANG_NONE;            /* No syntax highlighting by default */
}

/* Create edit control for a tab */
static HWND CreateTabEditControl(HWND hwndParent, BOOL bWordWrap) {
    DWORD dwStyle = WS_CHILD | WS_VSCROLL | ES_MULTILINE | 
                    ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL;
    
    if (!bWordWrap) {
        dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    HWND hwndEdit = NULL;
    
    /* Try RichEdit first (better for large files) */
    if (g_hRichEdit) {
        /* Try RICHEDIT50W first (from Msftedit.dll) */
        hwndEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            TEXT("RICHEDIT50W"),
            TEXT(""),
            dwStyle,
            0, 0, 0, 0,
            hwndParent,
            (HMENU)IDC_EDIT,
            g_AppState.hInstance,
            NULL
        );
        
        /* Fallback to RichEdit20W (from Riched20.dll) */
        if (!hwndEdit) {
            hwndEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("RichEdit20W"),
                TEXT(""),
                dwStyle,
                0, 0, 0, 0,
                hwndParent,
                (HMENU)IDC_EDIT,
                g_AppState.hInstance,
                NULL
            );
        }
    }
    
    /* Fallback to standard Edit control */
    if (!hwndEdit) {
        hwndEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            TEXT("EDIT"),
            TEXT(""),
            dwStyle,
            0, 0, 0, 0,
            hwndParent,
            (HMENU)IDC_EDIT,
            g_AppState.hInstance,
            NULL
        );
    }
    
    if (hwndEdit) {
        /* Set font */
        SendMessage(hwndEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        
        /* Set text limit to maximum */
        SendMessage(hwndEdit, EM_SETLIMITTEXT, 0, 0);
        
        /* Subclass edit control to catch scroll events */
        g_OrigEditProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
    }
    
    return hwndEdit;
}

/* Add a new tab */
int AddNewTab(HWND hwnd, const TCHAR* szTitle) {
    if (g_AppState.nTabCount >= MAX_TABS) {
        ShowErrorDialog(hwnd, TEXT("Maximum number of tabs reached."));
        return -1;
    }
    
    int nNewTab = g_AppState.nTabCount;
    
    /* Initialize tab state */
    InitTabState(&g_AppState.tabs[nNewTab]);
    
    /* Create edit control for this tab */
    g_AppState.tabs[nNewTab].hwndEdit = CreateTabEditControl(hwnd, g_AppState.bWordWrap);
    
    /* Create line number window if line numbers are enabled */
    if (g_AppState.bShowLineNumbers) {
        g_AppState.tabs[nNewTab].lineNumState.hwndLineNumbers = CreateLineNumberWindow(hwnd, g_AppState.hInstance);
        g_AppState.tabs[nNewTab].lineNumState.bShowLineNumbers = TRUE;
        g_AppState.tabs[nNewTab].lineNumState.nLineNumberWidth = CalculateLineNumberWidth(1);
    }
    
    /* Add tab to tab control */
    TCITEM tie = {0};
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPTSTR)szTitle;
    TabCtrl_InsertItem(g_AppState.hwndTab, nNewTab, &tie);
    
    g_AppState.nTabCount++;
    
    /* Switch to new tab */
    SwitchToTab(hwnd, nNewTab);
    
    return nNewTab;
}

/* Close a tab */
void CloseTab(HWND hwnd, int nTabIndex) {
    if (nTabIndex < 0 || nTabIndex >= g_AppState.nTabCount) return;
    
    TabState* pTab = &g_AppState.tabs[nTabIndex];
    
    /* Check for unsaved changes */
    if (pTab->bModified) {
        g_AppState.nCurrentTab = nTabIndex;
        SwitchToTab(hwnd, nTabIndex);
        if (!PromptSaveChanges(hwnd)) {
            return; /* User cancelled */
        }
    }
    
    /* Destroy edit control */
    if (pTab->hwndEdit) {
        DestroyWindow(pTab->hwndEdit);
    }
    
    /* Destroy line number window */
    if (pTab->lineNumState.hwndLineNumbers) {
        DestroyWindow(pTab->lineNumState.hwndLineNumbers);
    }
    
    /* Free content buffer if allocated */
    if (pTab->pContent) {
        HeapFree(GetProcessHeap(), 0, pTab->pContent);
    }
    
    /* Remove tab from tab control */
    TabCtrl_DeleteItem(g_AppState.hwndTab, nTabIndex);
    
    /* Shift remaining tabs */
    for (int i = nTabIndex; i < g_AppState.nTabCount - 1; i++) {
        g_AppState.tabs[i] = g_AppState.tabs[i + 1];
    }
    
    g_AppState.nTabCount--;
    
    /* If no tabs left, create a new one */
    if (g_AppState.nTabCount == 0) {
        AddNewTab(hwnd, TEXT("Untitled"));
    } else {
        /* Switch to appropriate tab */
        int nNewCurrent = (nTabIndex >= g_AppState.nTabCount) ? 
                          g_AppState.nTabCount - 1 : nTabIndex;
        SwitchToTab(hwnd, nNewCurrent);
    }
}

/* Switch to a specific tab */
void SwitchToTab(HWND hwnd, int nTabIndex) {
    if (nTabIndex < 0 || nTabIndex >= g_AppState.nTabCount) return;
    
    /* Hide current edit control and line numbers */
    if (g_AppState.nCurrentTab >= 0 && g_AppState.nCurrentTab < g_AppState.nTabCount) {
        ShowWindow(g_AppState.tabs[g_AppState.nCurrentTab].hwndEdit, SW_HIDE);
        if (g_AppState.tabs[g_AppState.nCurrentTab].lineNumState.hwndLineNumbers) {
            ShowWindow(g_AppState.tabs[g_AppState.nCurrentTab].lineNumState.hwndLineNumbers, SW_HIDE);
        }
    }
    
    g_AppState.nCurrentTab = nTabIndex;
    
    TabState* pTab = &g_AppState.tabs[nTabIndex];
    
    /* Show line number window if enabled */
    if (g_AppState.bShowLineNumbers) {
        /* Create line number window if not exists */
        if (!pTab->lineNumState.hwndLineNumbers) {
            pTab->lineNumState.hwndLineNumbers = CreateLineNumberWindow(hwnd, g_AppState.hInstance);
            pTab->lineNumState.bShowLineNumbers = TRUE;
            int nLines = (int)SendMessage(pTab->hwndEdit, EM_GETLINECOUNT, 0, 0);
            pTab->lineNumState.nLineNumberWidth = CalculateLineNumberWidth(nLines);
        }
        ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_SHOW);
    }
    
    /* Show edit control */
    if (pTab->hwndEdit) {
        ShowWindow(pTab->hwndEdit, SW_SHOW);
        SetFocus(pTab->hwndEdit);
    }
    
    /* Reposition controls */
    RepositionControls(hwnd);
    
    /* Update tab control selection */
    TabCtrl_SetCurSel(g_AppState.hwndTab, nTabIndex);
    
    /* Update window title */
    UpdateWindowTitle(hwnd);
}

/* Update tab title */
void UpdateTabTitle(int nTabIndex) {
    if (nTabIndex < 0 || nTabIndex >= g_AppState.nTabCount) return;
    
    TabState* pTab = &g_AppState.tabs[nTabIndex];
    TCHAR szTitle[MAX_PATH + 4];
    
    if (pTab->bUntitled) {
        _sntprintf(szTitle, MAX_PATH + 4, TEXT("Untitled%s"), 
                   pTab->bModified ? TEXT(" *") : TEXT(""));
    } else {
        /* Extract filename from full path */
        TCHAR* pFileName = _tcsrchr(pTab->szFileName, TEXT('\\'));
        if (pFileName) {
            pFileName++;
        } else {
            pFileName = pTab->szFileName;
        }
        _sntprintf(szTitle, MAX_PATH + 4, TEXT("%s%s"), 
                   pFileName, pTab->bModified ? TEXT(" *") : TEXT(""));
    }
    
    TCITEM tie = {0};
    tie.mask = TCIF_TEXT;
    tie.pszText = szTitle;
    TabCtrl_SetItem(g_AppState.hwndTab, nTabIndex, &tie);
}

/* Register main window class */
ATOM RegisterMainWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {0};
    
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    
    return RegisterClassEx(&wc);
}

/* Create main window */
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
    HWND hwnd = CreateWindowEx(
        0,
        szClassName,
        TEXT("Untitled - XNote"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 800,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd) {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }
    
    return hwnd;
}

/* Window procedure */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            /* Initialize word wrap to OFF by default */
            g_AppState.bWordWrap = FALSE;
            g_AppState.bShowLineNumbers = TRUE;  /* Line numbers ON by default */
            g_AppState.nTabCount = 0;
            g_AppState.nCurrentTab = -1;
            
            /* Create font for edit controls */
            g_hFont = CreateFont(
                16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("Consolas")
            );
            
            /* Create status bar */
            g_AppState.hwndStatus = CreateStatusBar(hwnd, g_AppState.hInstance);
            
            /* Create tab control with owner draw for close buttons */
            g_AppState.hwndTab = CreateWindowEx(
                0,
                WC_TABCONTROL,
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED,
                0, 0, 0, TAB_HEIGHT,
                hwnd,
                (HMENU)IDC_TAB,
                g_AppState.hInstance,
                NULL
            );
            SendMessage(g_AppState.hwndTab, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            
            /* Subclass tab control to handle mouse events for close button */
            g_OrigTabProc = (WNDPROC)SetWindowLongPtr(g_AppState.hwndTab, GWLP_WNDPROC, (LONG_PTR)TabSubclassProc);
            
            /* Set tab item size for close button */
            TabCtrl_SetItemSize(g_AppState.hwndTab, 120, TAB_HEIGHT - 4);
            
            /* Create first tab */
            AddNewTab(hwnd, TEXT("Untitled"));
            
            /* Initialize menu check marks */
            HMENU hMenu = GetMenu(hwnd);
            CheckMenuItem(hMenu, IDM_VIEW_LINENUMBERS, 
                          g_AppState.bShowLineNumbers ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_FORMAT_WORDWRAP, 
                          g_AppState.bWordWrap ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_VIEW_SYNTAX, 
                          g_bSyntaxHighlight ? MF_CHECKED : MF_UNCHECKED);
            
            /* Start periodic sync timer for line numbers if enabled */
            if (g_AppState.bShowLineNumbers) {
                SetTimer(hwnd, 2, 100, NULL);
            }
            
            /* Start status bar update timer */
            SetTimer(hwnd, TIMER_STATUSBAR, 100, NULL);
            
            /* Initial status bar update */
            UpdateStatusBar(hwnd);
            
            return 0;
        }
        
        case WM_SIZE: {
            /* Debounce resize - use timer to avoid too many redraws */
            SetTimer(hwnd, 3, 16, NULL); /* ~60fps */
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == 1) {
                /* Scroll sync timer */
                KillTimer(hwnd, 1);
                TabState* pTab = GetCurrentTabState();
                if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    InvalidateRect(pTab->lineNumState.hwndLineNumbers, NULL, FALSE);
                }
            } else if (wParam == 2) {
                /* Periodic sync timer for line numbers - only sync if scroll position changed */
                TabState* pTab = GetCurrentTabState();
                if (pTab && pTab->hwndEdit && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    static int s_nLastFirstLine = -1;
                    static int s_nLastLineCount = -1;
                    int nFirstLine = (int)SendMessage(pTab->hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
                    int nLineCount = (int)SendMessage(pTab->hwndEdit, EM_GETLINECOUNT, 0, 0);
                    if (nFirstLine != s_nLastFirstLine || nLineCount != s_nLastLineCount) {
                        s_nLastFirstLine = nFirstLine;
                        s_nLastLineCount = nLineCount;
                        InvalidateRect(pTab->lineNumState.hwndLineNumbers, NULL, FALSE);
                    }
                }
            } else if (wParam == 3) {
                /* Resize debounce timer */
                KillTimer(hwnd, 3);
                RepositionControls(hwnd);
            } else if (wParam == TIMER_STATUSBAR) {
                /* Update status bar periodically */
                UpdateStatusBar(hwnd);
            }
            return 0;
        }
        
        case WM_VSCROLL:
        case WM_MOUSEWHEEL: {
            /* Sync line numbers when scrolling */
            TabState* pTab = GetCurrentTabState();
            if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                SetTimer(hwnd, 1, 16, NULL);
            }
            break;
        }
        
        case WM_NOTIFY: {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->hwndFrom == g_AppState.hwndTab) {
                if (pnmh->code == TCN_SELCHANGE) {
                    int nNewTab = TabCtrl_GetCurSel(g_AppState.hwndTab);
                    SwitchToTab(hwnd, nNewTab);
                }
            }
            return 0;
        }
        
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
            if (pDIS->CtlID == IDC_TAB) {
                /* Draw tab with close button */
                RECT rc = pDIS->rcItem;
                BOOL bSelected = (pDIS->itemState & ODS_SELECTED);
                
                /* Fill background */
                FillRect(pDIS->hDC, &rc, (HBRUSH)(bSelected ? COLOR_WINDOW + 1 : COLOR_BTNFACE + 1));
                
                /* Get tab text */
                TCHAR szText[MAX_PATH];
                TCITEM tci = {0};
                tci.mask = TCIF_TEXT;
                tci.pszText = szText;
                tci.cchTextMax = MAX_PATH;
                TabCtrl_GetItem(g_AppState.hwndTab, pDIS->itemID, &tci);
                
                /* Draw text */
                rc.left += 4;
                rc.right -= CLOSE_BTN_SIZE + 4;
                SetBkMode(pDIS->hDC, TRANSPARENT);
                DrawText(pDIS->hDC, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                
                /* Draw close button (X) */
                RECT rcClose;
                rcClose.right = pDIS->rcItem.right - 4;
                rcClose.left = rcClose.right - CLOSE_BTN_SIZE;
                rcClose.top = pDIS->rcItem.top + (pDIS->rcItem.bottom - pDIS->rcItem.top - CLOSE_BTN_SIZE) / 2;
                rcClose.bottom = rcClose.top + CLOSE_BTN_SIZE;
                
                /* Check if hovering over this tab's close button */
                BOOL bHoverThisClose = (g_nHoverTab == (int)pDIS->itemID && g_bHoverClose);
                
                /* Draw close button background if hovering */
                if (bHoverThisClose) {
                    HBRUSH hCloseBrush = CreateSolidBrush(RGB(232, 17, 35));
                    RECT rcCloseBg = rcClose;
                    InflateRect(&rcCloseBg, 2, 2);
                    FillRect(pDIS->hDC, &rcCloseBg, hCloseBrush);
                    DeleteObject(hCloseBrush);
                }
                
                /* Draw X with appropriate color */
                COLORREF closeColor = bHoverThisClose ? RGB(255, 255, 255) : RGB(100, 100, 100);
                HPEN hPen = CreatePen(PS_SOLID, 2, closeColor);
                HPEN hOldPen = (HPEN)SelectObject(pDIS->hDC, hPen);
                MoveToEx(pDIS->hDC, rcClose.left + 3, rcClose.top + 3, NULL);
                LineTo(pDIS->hDC, rcClose.right - 3, rcClose.bottom - 3);
                MoveToEx(pDIS->hDC, rcClose.right - 3, rcClose.top + 3, NULL);
                LineTo(pDIS->hDC, rcClose.left + 3, rcClose.bottom - 3);
                SelectObject(pDIS->hDC, hOldPen);
                DeleteObject(hPen);
                
                return TRUE;
            }
            break;
        }
        
        case WM_LBUTTONUP: {
            /* Check if click is on tab close button (backup handler) */
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            HWND hwndTab = g_AppState.hwndTab;
            
            /* Convert to tab control coordinates */
            RECT rcTab;
            GetWindowRect(hwndTab, &rcTab);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcTab, 2);
            
            if (PtInRect(&rcTab, pt)) {
                /* Find which tab was clicked */
                TCHITTESTINFO htInfo;
                htInfo.pt.x = pt.x - rcTab.left;
                htInfo.pt.y = pt.y - rcTab.top;
                int nTab = TabCtrl_HitTest(hwndTab, &htInfo);
                
                if (nTab >= 0) {
                    /* Check if click is on close button area */
                    RECT rcItem;
                    TabCtrl_GetItemRect(hwndTab, nTab, &rcItem);
                    
                    int closeX = rcItem.right - CLOSE_BTN_SIZE - 4;
                    if (htInfo.pt.x >= closeX) {
                        CloseTab(hwnd, nTab);
                        return 0;
                    }
                }
            }
            break;
        }

        case WM_COMMAND: {
            HWND hwndEdit = GetCurrentEdit();
            TabState* pTab = GetCurrentTabState();
            
            switch (LOWORD(wParam)) {
                /* File menu */
                case IDM_FILE_NEW:
                    FileNew(hwnd);
                    break;
                case IDM_FILE_NEWTAB:
                    AddNewTab(hwnd, TEXT("Untitled"));
                    break;
                case IDM_FILE_OPEN:
                    FileOpen(hwnd);
                    break;
                case IDM_FILE_SAVE:
                    FileSave(hwnd);
                    break;
                case IDM_FILE_SAVEAS:
                    FileSaveAs(hwnd);
                    break;
                case IDM_FILE_CLOSETAB:
                    CloseTab(hwnd, g_AppState.nCurrentTab);
                    break;
                case IDM_FILE_EXIT:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;

                /* Edit menu */
                case IDM_EDIT_UNDO:
                    if (hwndEdit) EditUndo(hwndEdit);
                    break;
                case IDM_EDIT_CUT:
                    if (hwndEdit) EditCut(hwndEdit);
                    break;
                case IDM_EDIT_COPY:
                    if (hwndEdit) EditCopy(hwndEdit);
                    break;
                case IDM_EDIT_PASTE:
                    if (hwndEdit) EditPaste(hwndEdit);
                    break;
                case IDM_EDIT_SELECTALL:
                    if (hwndEdit) EditSelectAll(hwndEdit);
                    break;
                
                /* Find/Replace */
                case IDM_EDIT_FIND:
                    ShowFindDialog(hwnd);
                    break;
                case IDM_EDIT_REPLACE:
                    ShowReplaceDialog(hwnd);
                    break;
                case IDM_EDIT_FINDNEXT:
                    FindNext(hwnd);
                    break;
                
                /* Format menu */
                case IDM_FORMAT_WORDWRAP:
                    ToggleWordWrap(hwnd);
                    break;
                
                /* View menu */
                case IDM_VIEW_LINENUMBERS:
                    ToggleLineNumbers(hwnd);
                    break;
                
                case IDM_VIEW_SYNTAX: {
                    /* Toggle syntax highlighting */
                    g_bSyntaxHighlight = !g_bSyntaxHighlight;
                    HMENU hMenu = GetMenu(hwnd);
                    CheckMenuItem(hMenu, IDM_VIEW_SYNTAX, 
                                  g_bSyntaxHighlight ? MF_CHECKED : MF_UNCHECKED);
                    /* Re-apply highlighting */
                    if (pTab && pTab->hwndEdit) {
                        if (g_bSyntaxHighlight) {
                            pTab->language = DetectLanguage(pTab->szFileName);
                            ApplySyntaxHighlighting(pTab->hwndEdit, pTab->language);
                        } else {
                            /* Reset to default color */
                            SetupSyntaxHighlighting(pTab->hwndEdit, LANG_NONE);
                        }
                    }
                    break;
                }
                
                /* Help menu */
                case IDM_HELP_CONTENTS:
                    ShowHelpDialog(hwnd);
                    break;
                case IDM_HELP_ABOUT:
                    ShowAboutDialog(hwnd);
                    break;
                
                /* Edit control notifications */
                default:
                    if (HIWORD(wParam) == EN_CHANGE && pTab) {
                        pTab->bModified = TRUE;
                        UpdateTabTitle(g_AppState.nCurrentTab);
                        
                        /* Update line numbers if visible */
                        if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                            /* Recalculate width if line count changed significantly */
                            int nLines = (int)SendMessage(pTab->hwndEdit, EM_GETLINECOUNT, 0, 0);
                            int nNewWidth = CalculateLineNumberWidth(nLines);
                            if (nNewWidth != pTab->lineNumState.nLineNumberWidth) {
                                pTab->lineNumState.nLineNumberWidth = nNewWidth;
                                RepositionControls(hwnd);
                            }
                            UpdateLineNumbers(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                        }
                    }
                    break;
            }
            return 0;
        }

        case WM_CLOSE: {
            /* Check all tabs for unsaved changes */
            for (int i = 0; i < g_AppState.nTabCount; i++) {
                if (g_AppState.tabs[i].bModified) {
                    SwitchToTab(hwnd, i);
                    if (!PromptSaveChanges(hwnd)) {
                        return 0; /* User cancelled */
                    }
                }
            }
            DestroyWindow(hwnd);
            return 0;
        }
        
        case WM_DESTROY:
            /* Cleanup all tabs */
            for (int i = 0; i < g_AppState.nTabCount; i++) {
                if (g_AppState.tabs[i].hwndEdit) {
                    DestroyWindow(g_AppState.tabs[i].hwndEdit);
                }
                if (g_AppState.tabs[i].lineNumState.hwndLineNumbers) {
                    DestroyWindow(g_AppState.tabs[i].lineNumState.hwndLineNumbers);
                }
                if (g_AppState.tabs[i].pContent) {
                    HeapFree(GetProcessHeap(), 0, g_AppState.tabs[i].pContent);
                }
            }
            
            if (g_hFont) {
                DeleteObject(g_hFont);
                g_hFont = NULL;
            }
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Toggle word wrap on/off */
void ToggleWordWrap(HWND hwnd) {
    g_AppState.bWordWrap = !g_AppState.bWordWrap;
    
    /* Update menu check mark */
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_FORMAT_WORDWRAP, 
                  g_AppState.bWordWrap ? MF_CHECKED : MF_UNCHECKED);
    
    /* Only recreate current tab's edit control for better performance */
    if (g_AppState.nCurrentTab >= 0) {
        RecreateEditControl(hwnd, g_AppState.nCurrentTab, g_AppState.bWordWrap);
    }
}

/* Recreate edit control with word wrap setting */
void RecreateEditControl(HWND hwnd, int nTabIndex, BOOL bWordWrap) {
    if (nTabIndex < 0 || nTabIndex >= g_AppState.nTabCount) return;
    
    TabState* pTab = &g_AppState.tabs[nTabIndex];
    HWND hwndOldEdit = pTab->hwndEdit;
    
    if (!hwndOldEdit) return;
    
    /* Stop timers during recreation to prevent crashes */
    KillTimer(hwnd, 1);
    KillTimer(hwnd, 2);
    KillTimer(hwnd, 3);
    
    /* Disable redraw during recreation */
    SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    
    /* Get current text using wide char for better performance */
    int nLen = GetWindowTextLengthW(hwndOldEdit);
    WCHAR* pText = NULL;
    BOOL bWasModified = pTab->bModified;
    
    if (nLen > 0) {
        pText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLen + 1) * sizeof(WCHAR));
        if (pText) {
            GetWindowTextW(hwndOldEdit, pText, nLen + 1);
        }
    }
    
    /* Destroy old edit control */
    DestroyWindow(hwndOldEdit);
    pTab->hwndEdit = NULL;
    
    /* Create new edit control */
    pTab->hwndEdit = CreateTabEditControl(hwnd, bWordWrap);
    
    if (!pTab->hwndEdit) {
        /* Failed to create - cleanup and return */
        if (pText) HeapFree(GetProcessHeap(), 0, pText);
        SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
        return;
    }
    
    /* Restore text */
    if (pText) {
        SetWindowTextW(pTab->hwndEdit, pText);
        HeapFree(GetProcessHeap(), 0, pText);
    }
    
    /* Restore modified flag */
    pTab->bModified = bWasModified;
    
    /* Reposition using RepositionControls for proper line number handling */
    if (nTabIndex == g_AppState.nCurrentTab) {
        RepositionControls(hwnd);
        ShowWindow(pTab->hwndEdit, SW_SHOW);
        SetFocus(pTab->hwndEdit);
    } else {
        ShowWindow(pTab->hwndEdit, SW_HIDE);
    }
    
    /* Re-enable redraw */
    SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, NULL, TRUE);
    
    /* Restart periodic timer if line numbers are visible */
    if (g_AppState.bShowLineNumbers) {
        SetTimer(hwnd, 2, 100, NULL);
    }
}

/* WinMain entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    /* Load RichEdit library */
    g_hRichEdit = LoadLibrary(TEXT("Msftedit.dll"));
    if (!g_hRichEdit) {
        g_hRichEdit = LoadLibrary(TEXT("Riched20.dll"));
    }
    
    /* Initialize common controls */
    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icc);
    
    /* Store instance handle */
    g_AppState.hInstance = hInstance;
    
    /* Register window class */
    if (!RegisterMainWindowClass(hInstance)) {
        MessageBox(NULL, TEXT("Failed to register window class"), 
                   TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* Create main window */
    g_AppState.hwndMain = CreateMainWindow(hInstance, nCmdShow);
    if (!g_AppState.hwndMain) {
        MessageBox(NULL, TEXT("Failed to create window"), 
                   TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* Load accelerator table */
    g_AppState.hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
    
    /* Message loop with accelerator handling */
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(g_AppState.hwndMain, g_AppState.hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    /* Unload RichEdit library */
    if (g_hRichEdit) {
        FreeLibrary(g_hRichEdit);
    }
    
    return (int)msg.wParam;
}
