#include "notepad.h"
#include "syntax.h"
#include "vim_mode.h"
#include "session.h"
#include "theme.h"
#include <richedit.h>

/* Global application state */
AppState g_AppState = {0};

/* Window class name */
static const TCHAR szClassName[] = TEXT("NotepadMainWindow");

/* Font handle for edit control */
static HFONT g_hFont = NULL;

/* Font handle for tab control - smaller font for better fit */
static HFONT g_hTabFont = NULL;

/* Set global font handle */
void SetGlobalFont(HFONT hFont) {
    if (g_hFont && g_hFont != hFont) {
        DeleteObject(g_hFont);
    }
    g_hFont = hFont;
}

/* Get global font handle */
HFONT GetGlobalFont(void) {
    return g_hFont;
}

/* RichEdit library handle */
static HMODULE g_hRichEdit = NULL;

/* Original edit control window procedure */
static WNDPROC g_OrigEditProc = NULL;

/* Scroll debouncing timer ID and state for large file optimization */
#define TIMER_SCROLL_DEBOUNCE 100
static BOOL g_bScrollPending = FALSE;
static DWORD g_dwLastScrollTime = 0;
#define SCROLL_DEBOUNCE_MS 16  /* ~60 FPS for smooth scrolling */

/* Threshold for line count based debouncing (files with many lines need debouncing regardless of size) */
#define LINE_COUNT_DEBOUNCE_THRESHOLD 5000

/* Subclassed edit control procedure to catch scroll events and vim keys */
static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    /* Process vim keys first if vim mode is enabled */
    if (IsVimModeEnabled()) {
        if (msg == WM_KEYDOWN || msg == WM_CHAR) {
            if (ProcessVimKey(hwnd, msg, wParam, lParam)) {
                /* Sync line numbers after vim navigation */
                TabState* pTab = GetCurrentTabState();
                if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                }
                return 0; /* Key was handled by vim */
            }
        }
    }
    
    switch (msg) {
        case WM_TIMER:
            if (wParam == TIMER_SCROLL_DEBOUNCE) {
                /* Debounced scroll update */
                KillTimer(hwnd, TIMER_SCROLL_DEBOUNCE);
                g_bScrollPending = FALSE;
                TabState* pTab = GetCurrentTabState();
                if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                }
                return 0;
            }
            break;
            
        case WM_VSCROLL:
        case WM_HSCROLL:
        case WM_MOUSEWHEEL: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Use debouncing for large files OR files with many lines (Requirement 4.3, 4.4) */
            TabState* pTab = GetCurrentTabState();
            if (pTab) {
                /* Check both file size AND line count for debouncing decision */
                int nLineCount = (int)SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
                BOOL bNeedsDebouncing = (pTab->dwTotalFileSize > THRESHOLD_PARTIAL) || 
                                        (nLineCount > LINE_COUNT_DEBOUNCE_THRESHOLD);
                
                if (bNeedsDebouncing) {
                    /* Large file or many lines - use debounced scroll */
                    DWORD dwNow = GetTickCount();
                    if (dwNow - g_dwLastScrollTime < SCROLL_DEBOUNCE_MS) {
                        /* Too soon - set timer for delayed update */
                        if (!g_bScrollPending) {
                            SetTimer(hwnd, TIMER_SCROLL_DEBOUNCE, SCROLL_DEBOUNCE_MS, NULL);
                            g_bScrollPending = TRUE;
                        }
                    } else {
                        /* Enough time passed - update immediately */
                        g_dwLastScrollTime = dwNow;
                        if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                            SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                        }
                    }
                } else {
                    /* Small file with few lines - sync immediately */
                    if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                        SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                    }
                }
            }
            return result;
        }
        
        case WM_KEYDOWN:
        case WM_KEYUP: {
            /* Call original proc first */
            LRESULT result = CallWindowProc(g_OrigEditProc, hwnd, msg, wParam, lParam);
            
            /* Sync line numbers for navigation keys - with debouncing for large files */
            if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR || 
                wParam == VK_NEXT || wParam == VK_HOME || wParam == VK_END) {
                TabState* pTab = GetCurrentTabState();
                if (pTab && g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
                    /* Check if debouncing needed for large files */
                    int nLineCount = (int)SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
                    BOOL bNeedsDebouncing = (pTab->dwTotalFileSize > THRESHOLD_PARTIAL) || 
                                            (nLineCount > LINE_COUNT_DEBOUNCE_THRESHOLD);
                    
                    if (bNeedsDebouncing && (wParam == VK_UP || wParam == VK_DOWN)) {
                        /* For rapid up/down keys, use debouncing */
                        DWORD dwNow = GetTickCount();
                        if (dwNow - g_dwLastScrollTime < SCROLL_DEBOUNCE_MS) {
                            if (!g_bScrollPending) {
                                SetTimer(hwnd, TIMER_SCROLL_DEBOUNCE, SCROLL_DEBOUNCE_MS, NULL);
                                g_bScrollPending = TRUE;
                            }
                        } else {
                            g_dwLastScrollTime = dwNow;
                            SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                        }
                    } else {
                        /* Small file or page keys - sync immediately */
                        SyncLineNumberScroll(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
                    }
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

/* Tab control height - increased for better visual appearance */
#define TAB_HEIGHT 32

/* Close button size */
#define CLOSE_BTN_SIZE 16

/* Tab padding for better spacing */
#define TAB_PADDING 8

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
    pState->bNeedsSyntaxRefresh = FALSE;     /* No pending syntax refresh */

    /* Initialize large file support fields */
    pState->fileMode = FILEMODE_NORMAL;      /* Normal mode by default */
    pState->hFileMapping = NULL;             /* No memory mapping */
    pState->pMappedView = NULL;              /* No mapped view */
    pState->dwTotalFileSize = 0;             /* No file loaded */
    pState->dwLoadedSize = 0;                /* Nothing loaded yet */
    pState->dwChunkSize = 20 * 1024 * 1024;  /* Default 20MB chunks */
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

        /* Set text limit to maximum for large file support */
        /* EM_SETLIMITTEXT with 0 = default limit (~32KB for EDIT, ~1GB for RichEdit) */
        /* EM_EXLIMITTEXT allows setting explicit limit up to 2GB for RichEdit */
        SendMessage(hwndEdit, EM_SETLIMITTEXT, 0, 0);

        /* For RichEdit controls, set extended limit to support files up to 1GB */
        /* This is a RichEdit-specific message that supports larger limits */
        if (g_hRichEdit) {
            /* Set limit to 1GB (1073741824 bytes) for text capacity */
            SendMessage(hwndEdit, EM_EXLIMITTEXT, 0, 1073741824);
        }

        /* Apply current theme colors */
        ApplyThemeToEdit(hwndEdit);
        
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
    
    /* Check if edit control was created successfully */
    if (!g_AppState.tabs[nNewTab].hwndEdit) {
        ShowErrorDialog(hwnd, TEXT("Failed to create edit control for new tab."));
        return -1;
    }
    
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
    
    /* Clean up memory-mapped file handles (Requirement 1.3) */
    if (pTab->pMappedView) {
        UnmapViewOfFile(pTab->pMappedView);
        pTab->pMappedView = NULL;
    }
    if (pTab->hFileMapping) {
        CloseHandle(pTab->hFileMapping);
        pTab->hFileMapping = NULL;
    }
    
    /* Remove tab from tab control */
    TabCtrl_DeleteItem(g_AppState.hwndTab, nTabIndex);
    
    /* Shift remaining tabs */
    for (int i = nTabIndex; i < g_AppState.nTabCount - 1; i++) {
        g_AppState.tabs[i] = g_AppState.tabs[i + 1];
    }
    
    g_AppState.nTabCount--;
    
    /* Sync session after tab close */
    MarkSessionDirty();
    
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

/* Close all tabs */
void CloseAllTabs(HWND hwnd) {
    /* Check for any unsaved changes in all tabs */
    for (int i = 0; i < g_AppState.nTabCount; i++) {
        if (g_AppState.tabs[i].bModified) {
            /* Switch to the modified tab to show it to user */
            SwitchToTab(hwnd, i);

            /* Ask user what to do */
            int result = MessageBox(hwnd,
                TEXT("You have unsaved changes in one or more tabs.\n\nDo you want to save all changes before closing?"),
                TEXT("Unsaved Changes"),
                MB_YESNOCANCEL | MB_ICONWARNING);

            if (result == IDCANCEL) {
                return; /* User cancelled */
            } else if (result == IDYES) {
                /* Save all modified tabs */
                for (int j = 0; j < g_AppState.nTabCount; j++) {
                    if (g_AppState.tabs[j].bModified) {
                        SwitchToTab(hwnd, j);
                        if (!FileSave(hwnd)) {
                            /* If save failed, ask if user wants to continue */
                            if (MessageBox(hwnd,
                                TEXT("Failed to save file. Continue closing?"),
                                TEXT("Save Failed"),
                                MB_YESNO | MB_ICONERROR) != IDYES) {
                                return;
                            }
                        }
                    }
                }
            }
            /* If IDNO, close without saving */
            break;
        }
    }

    /* Close all tabs from back to front */
    while (g_AppState.nTabCount > 1) {
        /* Destroy edit control */
        if (g_AppState.tabs[g_AppState.nTabCount - 1].hwndEdit) {
            DestroyWindow(g_AppState.tabs[g_AppState.nTabCount - 1].hwndEdit);
        }

        /* Destroy line number window */
        if (g_AppState.tabs[g_AppState.nTabCount - 1].lineNumState.hwndLineNumbers) {
            DestroyWindow(g_AppState.tabs[g_AppState.nTabCount - 1].lineNumState.hwndLineNumbers);
        }

        /* Free content buffer if allocated */
        if (g_AppState.tabs[g_AppState.nTabCount - 1].pContent) {
            HeapFree(GetProcessHeap(), 0, g_AppState.tabs[g_AppState.nTabCount - 1].pContent);
            g_AppState.tabs[g_AppState.nTabCount - 1].pContent = NULL;
        }

        /* Remove tab from tab control */
        TabCtrl_DeleteItem(g_AppState.hwndTab, g_AppState.nTabCount - 1);
        g_AppState.nTabCount--;
    }

    /* Reset the last remaining tab to untitled */
    TabState* pLastTab = &g_AppState.tabs[0];
    if (pLastTab->hwndEdit) {
        SetWindowText(pLastTab->hwndEdit, TEXT(""));
    }
    pLastTab->szFileName[0] = TEXT('\0');
    pLastTab->bUntitled = TRUE;
    pLastTab->bModified = FALSE;
    pLastTab->language = LANG_NONE;

    /* Update tab title */
    UpdateTabTitle(0);
    SwitchToTab(hwnd, 0);
    MarkSessionDirty();
}

/* Close all tabs except the current one */
void CloseOtherTabs(HWND hwnd, int nKeepTab) {
    if (nKeepTab < 0 || nKeepTab >= g_AppState.nTabCount) return;
    if (g_AppState.nTabCount <= 1) return; /* Only one tab, nothing to close */

    /* Check for unsaved changes in other tabs */
    BOOL bHasUnsaved = FALSE;
    for (int i = 0; i < g_AppState.nTabCount; i++) {
        if (i != nKeepTab && g_AppState.tabs[i].bModified) {
            bHasUnsaved = TRUE;
            break;
        }
    }

    if (bHasUnsaved) {
        int result = MessageBox(hwnd,
            TEXT("Some tabs have unsaved changes.\n\nDo you want to save all changes before closing?"),
            TEXT("Unsaved Changes"),
            MB_YESNOCANCEL | MB_ICONWARNING);

        if (result == IDCANCEL) {
            return; /* User cancelled */
        } else if (result == IDYES) {
            /* Save all modified tabs except the one we're keeping */
            for (int i = 0; i < g_AppState.nTabCount; i++) {
                if (i != nKeepTab && g_AppState.tabs[i].bModified) {
                    SwitchToTab(hwnd, i);
                    if (!FileSave(hwnd)) {
                        if (MessageBox(hwnd,
                            TEXT("Failed to save file. Continue closing?"),
                            TEXT("Save Failed"),
                            MB_YESNO | MB_ICONERROR) != IDYES) {
                            return;
                        }
                    }
                }
            }
        }
    }

    /* Close all tabs after the kept tab */
    while (g_AppState.nTabCount > nKeepTab + 1) {
        int idx = g_AppState.nTabCount - 1;
        if (g_AppState.tabs[idx].hwndEdit) {
            DestroyWindow(g_AppState.tabs[idx].hwndEdit);
        }
        if (g_AppState.tabs[idx].lineNumState.hwndLineNumbers) {
            DestroyWindow(g_AppState.tabs[idx].lineNumState.hwndLineNumbers);
        }
        if (g_AppState.tabs[idx].pContent) {
            HeapFree(GetProcessHeap(), 0, g_AppState.tabs[idx].pContent);
        }
        TabCtrl_DeleteItem(g_AppState.hwndTab, idx);
        g_AppState.nTabCount--;
    }

    /* Close all tabs before the kept tab */
    while (nKeepTab > 0) {
        if (g_AppState.tabs[0].hwndEdit) {
            DestroyWindow(g_AppState.tabs[0].hwndEdit);
        }
        if (g_AppState.tabs[0].lineNumState.hwndLineNumbers) {
            DestroyWindow(g_AppState.tabs[0].lineNumState.hwndLineNumbers);
        }
        if (g_AppState.tabs[0].pContent) {
            HeapFree(GetProcessHeap(), 0, g_AppState.tabs[0].pContent);
        }
        TabCtrl_DeleteItem(g_AppState.hwndTab, 0);

        /* Shift all tabs down */
        for (int i = 0; i < g_AppState.nTabCount - 1; i++) {
            g_AppState.tabs[i] = g_AppState.tabs[i + 1];
        }
        g_AppState.nTabCount--;
        nKeepTab--; /* Adjust index since we shifted */
    }

    /* Switch to the kept tab (now at index 0) */
    SwitchToTab(hwnd, 0);
    MarkSessionDirty();
}

/* Switch to a specific tab - optimized for performance */
void SwitchToTab(HWND hwnd, int nTabIndex) {
    if (nTabIndex < 0 || nTabIndex >= g_AppState.nTabCount) return;
    
    /* Skip if already on this tab and tab is visible */
    if (nTabIndex == g_AppState.nCurrentTab && 
        g_AppState.tabs[nTabIndex].hwndEdit &&
        IsWindowVisible(g_AppState.tabs[nTabIndex].hwndEdit)) {
        return;
    }
    
    /* Reset line number cache when switching tabs for accurate rendering */
    ResetLineNumberCache();
    
    /* Disable redraw during switch for smoother transition */
    SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    
    /* Hide current edit control and line numbers - with NULL checks */
    if (g_AppState.nCurrentTab >= 0 && g_AppState.nCurrentTab < g_AppState.nTabCount) {
        TabState* pCurrentTab = &g_AppState.tabs[g_AppState.nCurrentTab];
        if (pCurrentTab->hwndEdit) {
            ShowWindow(pCurrentTab->hwndEdit, SW_HIDE);
        }
        if (pCurrentTab->lineNumState.hwndLineNumbers) {
            ShowWindow(pCurrentTab->lineNumState.hwndLineNumbers, SW_HIDE);
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
            /* Only get line count if edit control exists */
            if (pTab->hwndEdit) {
                int nLines = (int)SendMessage(pTab->hwndEdit, EM_GETLINECOUNT, 0, 0);
                pTab->lineNumState.nLineNumberWidth = CalculateLineNumberWidth(nLines);
            } else {
                pTab->lineNumState.nLineNumberWidth = CalculateLineNumberWidth(1);
            }
        }
        if (pTab->lineNumState.hwndLineNumbers) {
            ShowWindow(pTab->lineNumState.hwndLineNumbers, SW_SHOW);
        }
    }
    
    /* Show edit control */
    if (pTab->hwndEdit) {
        ShowWindow(pTab->hwndEdit, SW_SHOW);
        
        /* Apply lazy syntax refresh if needed (after theme change) */
        if (pTab->bNeedsSyntaxRefresh && g_bSyntaxHighlight && pTab->language != LANG_NONE) {
            ApplySyntaxHighlighting(pTab->hwndEdit, pTab->language);
            pTab->bNeedsSyntaxRefresh = FALSE;
        }
    }
    
    /* Re-enable redraw */
    SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    
    /* Reposition controls */
    RepositionControls(hwnd);
    
    /* Update tab control selection */
    TabCtrl_SetCurSel(g_AppState.hwndTab, nTabIndex);
    
    /* Update window title */
    UpdateWindowTitle(hwnd);
    
    /* Set focus after everything is done */
    if (pTab->hwndEdit) {
        SetFocus(pTab->hwndEdit);
    }
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
        /* Extract filename from full path - include extension */
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
    
    /* Force tab control to redraw */
    InvalidateRect(g_AppState.hwndTab, NULL, FALSE);
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
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szClassName;
    wc.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    
    /* Fallback to default icon if custom icon not found */
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
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
            g_AppState.bRelativeLineNumbers = FALSE;  /* Relative numbers OFF by default */
            g_AppState.nTabCount = 0;
            g_AppState.nCurrentTab = -1;
            
            /* Create font for edit controls - will be updated after session load */
            g_hFont = GetCurrentFontHandle();
            
            /* Create smaller font for tab control - use system UI font */
            {
                NONCLIENTMETRICS ncm;
                ncm.cbSize = sizeof(ncm);
                SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
                g_hTabFont = CreateFontIndirect(&ncm.lfMenuFont);
            }
            
            /* Create status bar */
            g_AppState.hwndStatus = CreateStatusBar(hwnd, g_AppState.hInstance);
            
            /* Create tab control with owner draw for close buttons */
            /* TCS_FIXEDWIDTH ensures all tabs have the same width set by TCM_SETITEMSIZE */
            g_AppState.hwndTab = CreateWindowEx(
                0,
                WC_TABCONTROL,
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH,
                0, 0, 0, TAB_HEIGHT,
                hwnd,
                (HMENU)IDC_TAB,
                g_AppState.hInstance,
                NULL
            );
            /* Use smaller tab font for better text fit */
            SendMessage(g_AppState.hwndTab, WM_SETFONT, (WPARAM)g_hTabFont, TRUE);
            
            /* Subclass tab control to handle mouse events for close button */
            g_OrigTabProc = (WNDPROC)SetWindowLongPtr(g_AppState.hwndTab, GWLP_WNDPROC, (LONG_PTR)TabSubclassProc);
            
            /* Set tab item size - wide enough for long filenames with extension + close button */
            /* TCM_SETITEMSIZE with TCS_FIXEDWIDTH ensures fixed width tabs */
            /* Width: 200px should be enough for most filenames with smaller font */
            /* Height: Slightly smaller than TAB_HEIGHT for better visual appearance */
            SendMessage(g_AppState.hwndTab, TCM_SETITEMSIZE, 0, MAKELPARAM(200, TAB_HEIGHT - 6));
            
            /* Initialize theme system */
            InitTheme();
            
            /* Try to load previous session, otherwise create first tab */
            if (!LoadSession(hwnd)) {
                AddNewTab(hwnd, TEXT("Untitled"));
            }
            
            /* Apply theme to all controls */
            ApplyThemeToWindow(hwnd);
            
            /* Initialize menu check marks */
            HMENU hMenu = GetMenu(hwnd);
            CheckMenuItem(hMenu, IDM_VIEW_LINENUMBERS, 
                          g_AppState.bShowLineNumbers ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_VIEW_RELATIVENUM, 
                          g_AppState.bRelativeLineNumbers ? MF_CHECKED : MF_UNCHECKED);
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
            
            /* Initialize session system (auto-save timer) */
            InitSessionSystem(hwnd);
            
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
            } else if (wParam == TIMER_AUTOSAVE) {
                /* Auto-save session periodically */
                HandleSessionTimer(hwnd);
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
        
        case WM_ACTIVATE: {
            /* Auto-focus edit control when window is activated */
            if (LOWORD(wParam) != WA_INACTIVE) {
                HWND hwndEdit = GetCurrentEdit();
                if (hwndEdit) {
                    SetFocus(hwndEdit);
                }
            }
            return 0;
        }
        
        case WM_SETFOCUS: {
            /* When main window gets focus, redirect to edit control */
            HWND hwndEdit = GetCurrentEdit();
            if (hwndEdit) {
                SetFocus(hwndEdit);
            }
            return 0;
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
                /* Draw tab with close button using theme colors */
                RECT rc = pDIS->rcItem;
                BOOL bSelected = (pDIS->itemState & ODS_SELECTED);
                const ThemeColors* pTheme = GetThemeColors();
                
                /* Fill background with theme color */
                HBRUSH hBgBrush = CreateSolidBrush(bSelected ? pTheme->crTabActive : pTheme->crTabInactive);
                FillRect(pDIS->hDC, &rc, hBgBrush);
                DeleteObject(hBgBrush);
                
                /* Get tab text */
                TCHAR szText[MAX_PATH];
                TCITEM tci = {0};
                tci.mask = TCIF_TEXT;
                tci.pszText = szText;
                tci.cchTextMax = MAX_PATH;
                TabCtrl_GetItem(g_AppState.hwndTab, pDIS->itemID, &tci);
                
                /* Draw active tab indicator line (top border) */
                if (bSelected) {
                    RECT rcIndicator = rc;
                    rcIndicator.bottom = rcIndicator.top + 3;
                    HBRUSH hIndicatorBrush = CreateSolidBrush(pTheme->crKeyword); /* Use accent color */
                    FillRect(pDIS->hDC, &rcIndicator, hIndicatorBrush);
                    DeleteObject(hIndicatorBrush);
                }

                /* Select tab font for drawing (smaller than edit font) */
                HFONT hOldFont = (HFONT)SelectObject(pDIS->hDC, g_hTabFont ? g_hTabFont : g_hFont);

                /* Calculate text area - better padding for improved spacing */
                RECT rcText = rc;
                rcText.left += TAB_PADDING;
                rcText.right -= (CLOSE_BTN_SIZE + TAB_PADDING + 4);
                rcText.top += 1; /* Slight vertical adjustment for better centering */

                /* Draw text with theme color - make active tab text brighter */
                SetBkMode(pDIS->hDC, TRANSPARENT);
                COLORREF textColor = bSelected ? pTheme->crForeground : pTheme->crTabText;
                SetTextColor(pDIS->hDC, textColor);
                DrawText(pDIS->hDC, szText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                
                /* Restore font */
                SelectObject(pDIS->hDC, hOldFont);
                
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
                case IDM_FILE_CLOSEALL:
                    CloseAllTabs(hwnd);
                    break;
                case IDM_FILE_CLOSEOTHERS:
                    CloseOtherTabs(hwnd, g_AppState.nCurrentTab);
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
                
                case IDM_FORMAT_FONT:
                    ShowFontDialog(hwnd);
                    break;
                
                /* View menu */
                case IDM_VIEW_LINENUMBERS:
                    ToggleLineNumbers(hwnd);
                    break;
                
                case IDM_VIEW_RELATIVENUM:
                    ToggleRelativeLineNumbers(hwnd);
                    break;
                
                case IDM_VIEW_SYNTAX: {
                    /* Check if trying to enable syntax on large file (Requirement 5.3) */
                    if (!g_bSyntaxHighlight) {
                        TabState* pCurrentTab = GetCurrentTabState();
                        if (pCurrentTab && pCurrentTab->dwTotalFileSize > THRESHOLD_SYNTAX_OFF) {
                            int nResult = MessageBox(hwnd,
                                TEXT("Enabling syntax highlighting on large files may cause performance issues.\n\n")
                                TEXT("The file is larger than 1MB. Syntax highlighting may cause:\n")
                                TEXT("- Slow scrolling\n")
                                TEXT("- Delayed response to typing\n")
                                TEXT("- High memory usage\n\n")
                                TEXT("Do you want to enable syntax highlighting anyway?"),
                                TEXT("Performance Warning"),
                                MB_YESNO | MB_ICONWARNING);
                            if (nResult != IDYES) {
                                break;
                            }
                        }
                    }
                    
                    /* Toggle syntax highlighting */
                    g_bSyntaxHighlight = !g_bSyntaxHighlight;
                    HMENU hMenu = GetMenu(hwnd);
                    CheckMenuItem(hMenu, IDM_VIEW_SYNTAX, 
                                  g_bSyntaxHighlight ? MF_CHECKED : MF_UNCHECKED);
                    /* Re-apply highlighting to all tabs */
                    for (int i = 0; i < g_AppState.nTabCount; i++) {
                        TabState* pTabItem = &g_AppState.tabs[i];
                        if (pTabItem->hwndEdit) {
                            if (g_bSyntaxHighlight) {
                                /* Skip syntax for large files (Requirement 5.1) */
                                if (pTabItem->dwTotalFileSize > THRESHOLD_SYNTAX_OFF) {
                                    continue;
                                }
                                pTabItem->language = DetectLanguage(pTabItem->szFileName);
                                ApplySyntaxHighlighting(pTabItem->hwndEdit, pTabItem->language);
                            } else {
                                /* Reset to default theme color */
                                ApplyThemeToEdit(pTabItem->hwndEdit);
                            }
                        }
                    }
                    /* Mark session dirty to save syntax highlight state */
                    MarkSessionDirty();
                    break;
                }
                
                case IDM_VIEW_VIMMODE:
                    ToggleVimMode(hwnd);
                    break;
                
                /* Theme selection */
                case IDM_THEME_LIGHT:
                    SetTheme(THEME_LIGHT);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_DARK:
                    SetTheme(THEME_DARK);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_TOKYO_NIGHT:
                    SetTheme(THEME_TOKYO_NIGHT);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_TOKYO_STORM:
                    SetTheme(THEME_TOKYO_NIGHT_STORM);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_TOKYO_LIGHT:
                    SetTheme(THEME_TOKYO_NIGHT_LIGHT);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_MONOKAI:
                    SetTheme(THEME_MONOKAI);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_DRACULA:
                    SetTheme(THEME_DRACULA);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_ONE_DARK:
                    SetTheme(THEME_ONE_DARK);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_NORD:
                    SetTheme(THEME_NORD);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_GRUVBOX:
                    SetTheme(THEME_GRUVBOX_DARK);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_EVERFOREST_DARK:
                    SetTheme(THEME_EVERFOREST_DARK);
                    ApplyThemeToWindow(hwnd);
                    break;
                case IDM_THEME_EVERFOREST_LIGHT:
                    SetTheme(THEME_EVERFOREST_LIGHT);
                    ApplyThemeToWindow(hwnd);
                    break;
                
                /* Zoom controls */
                case IDM_VIEW_ZOOMIN:
                    ZoomIn(hwnd);
                    break;
                case IDM_VIEW_ZOOMOUT:
                    ZoomOut(hwnd);
                    break;
                case IDM_VIEW_ZOOMRESET:
                    ZoomReset(hwnd);
                    break;
                
                /* Tab navigation */
                case IDM_TAB_NEXT:
                    if (g_AppState.nTabCount > 1) {
                        int nNext = (g_AppState.nCurrentTab + 1) % g_AppState.nTabCount;
                        SwitchToTab(hwnd, nNext);
                    }
                    break;
                case IDM_TAB_PREV:
                    if (g_AppState.nTabCount > 1) {
                        int nPrev = (g_AppState.nCurrentTab - 1 + g_AppState.nTabCount) % g_AppState.nTabCount;
                        SwitchToTab(hwnd, nPrev);
                    }
                    break;
                case IDM_TAB_1: case IDM_TAB_2: case IDM_TAB_3:
                case IDM_TAB_4: case IDM_TAB_5: case IDM_TAB_6:
                case IDM_TAB_7: case IDM_TAB_8: case IDM_TAB_9: {
                    int nTab = LOWORD(wParam) - IDM_TAB_1;
                    if (nTab < g_AppState.nTabCount) {
                        SwitchToTab(hwnd, nTab);
                    }
                    break;
                }
                
                /* Extended edit operations */
                case IDM_EDIT_GOTOLINE:
                    ShowGoToLineDialog(hwnd);
                    break;
                case IDM_EDIT_DUPLICATELINE:
                    if (hwndEdit) EditDuplicateLine(hwndEdit);
                    break;
                case IDM_EDIT_DELETELINE:
                    if (hwndEdit) EditDeleteLine(hwndEdit);
                    break;
                case IDM_EDIT_MOVELINEUP:
                    if (hwndEdit) EditMoveLineUp(hwndEdit);
                    break;
                case IDM_EDIT_MOVELINEDOWN:
                    if (hwndEdit) EditMoveLineDown(hwndEdit);
                    break;
                case IDM_EDIT_TOGGLECOMMENT:
                    if (hwndEdit) EditToggleComment(hwndEdit);
                    break;
                case IDM_EDIT_INDENT:
                    if (hwndEdit) EditIndent(hwndEdit);
                    break;
                case IDM_EDIT_UNINDENT:
                    if (hwndEdit) EditUnindent(hwndEdit);
                    break;
                case IDM_EDIT_LOADMORE:
                    LoadMoreContent(hwnd);
                    break;

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
            /* Save session before closing */
            SaveSessionOnExit(hwnd);
            
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
            /* Cleanup session system */
            CleanupSessionSystem();
            
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
            if (g_hTabFont) {
                DeleteObject(g_hTabFont);
                g_hTabFont = NULL;
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
    
    /* Mark session dirty */
    MarkSessionDirty();
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

/* Global variable to store command line file path */
static TCHAR g_szCmdLineFile[MAX_PATH] = {0};

/* Parse command line arguments to extract file path */
static void ParseCommandLine(LPSTR lpCmdLine) {
    if (!lpCmdLine || lpCmdLine[0] == '\0') {
        g_szCmdLineFile[0] = TEXT('\0');
        return;
    }
    
    /* Skip leading whitespace */
    while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
        lpCmdLine++;
    }
    
    if (*lpCmdLine == '\0') {
        g_szCmdLineFile[0] = TEXT('\0');
        return;
    }
    
    /* Handle quoted paths (e.g., "C:\My Documents\file.txt") */
    char szPath[MAX_PATH] = {0};
    int i = 0;
    
    if (*lpCmdLine == '"') {
        lpCmdLine++; /* Skip opening quote */
        while (*lpCmdLine && *lpCmdLine != '"' && i < MAX_PATH - 1) {
            szPath[i++] = *lpCmdLine++;
        }
    } else {
        /* Unquoted path - read until space or end */
        while (*lpCmdLine && *lpCmdLine != ' ' && *lpCmdLine != '\t' && i < MAX_PATH - 1) {
            szPath[i++] = *lpCmdLine++;
        }
    }
    szPath[i] = '\0';
    
    /* Convert to TCHAR (Unicode) */
    if (szPath[0] != '\0') {
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, g_szCmdLineFile, MAX_PATH);
    } else {
        g_szCmdLineFile[0] = TEXT('\0');
    }
}

/* Open file from command line after window is created */
static void OpenCommandLineFile(HWND hwnd) {
    if (g_szCmdLineFile[0] == TEXT('\0')) {
        return;
    }
    
    /* Check if file exists */
    DWORD dwAttrib = GetFileAttributes(g_szCmdLineFile);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        /* File doesn't exist - show error */
        TCHAR szError[MAX_PATH + 64];
        _sntprintf(szError, MAX_PATH + 64, 
            TEXT("File tidak ditemukan:\n%s"), g_szCmdLineFile);
        MessageBox(hwnd, szError, TEXT("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        /* It's a directory, not a file */
        TCHAR szError[MAX_PATH + 64];
        _sntprintf(szError, MAX_PATH + 64, 
            TEXT("Path adalah direktori, bukan file:\n%s"), g_szCmdLineFile);
        MessageBox(hwnd, szError, TEXT("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    /* Get current tab state */
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return;
    
    /* Load the file */
    if (ReadFileContent(pTab->hwndEdit, g_szCmdLineFile)) {
        /* Update tab state */
        _tcscpy(pTab->szFileName, g_szCmdLineFile);
        pTab->bUntitled = FALSE;
        pTab->bModified = FALSE;
        
        /* Update tab title and window title */
        UpdateTabTitle(g_AppState.nCurrentTab);
        UpdateWindowTitle(hwnd);
        
        /* Detect language for syntax highlighting */
        extern LanguageType DetectLanguage(const TCHAR* szFileName);
        extern void ApplySyntaxHighlighting(HWND hwndEdit, LanguageType lang);
        extern BOOL g_bSyntaxHighlight;
        
        pTab->language = DetectLanguage(g_szCmdLineFile);
        if (g_bSyntaxHighlight && pTab->language != LANG_NONE) {
            /* Only apply syntax highlighting for small files */
            if (pTab->dwTotalFileSize < THRESHOLD_SYNTAX_OFF) {
                ApplySyntaxHighlighting(pTab->hwndEdit, pTab->language);
            }
        }
    } else {
        /* Failed to read file */
        TCHAR szError[MAX_PATH + 64];
        _sntprintf(szError, MAX_PATH + 64, 
            TEXT("Gagal membuka file:\n%s"), g_szCmdLineFile);
        MessageBox(hwnd, szError, TEXT("Error"), MB_OK | MB_ICONERROR);
    }
}

/* WinMain entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    
    (void)hPrevInstance;
    
    /* Parse command line arguments */
    ParseCommandLine(lpCmdLine);
    
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
    
    /* Open file from command line if provided */
    OpenCommandLineFile(g_AppState.hwndMain);
    
    /* Message loop with accelerator and dialog handling */
    while (GetMessage(&msg, NULL, 0, 0)) {
        HWND hwndFindDlg = GetFindReplaceDialog();
        
        /* Check if Find/Replace dialog is open and message is for it */
        if (hwndFindDlg && IsDialogMessage(hwndFindDlg, &msg)) {
            /* Dialog handled the message - skip accelerator and normal processing */
            continue;
        }
        
        /* Process accelerators for main window */
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
