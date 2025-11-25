#include "notepad.h"
#include <richedit.h>

static TCHAR g_szFindText[256] = TEXT("");
static TCHAR g_szReplaceText[256] = TEXT("");
static BOOL g_bMatchCase = FALSE;
static BOOL g_bWholeWord = FALSE;
static HWND g_hwndFindReplace = NULL;

static const TCHAR szFilter[] = 
    TEXT("All Supported Files\0*.txt;*.md;*.json;*.xml;*.html;*.htm;*.css;*.js;*.ts;*.py;*.c;*.cpp;*.h;*.hpp;*.java;*.go;*.rs;*.sql;*.yaml;*.yml;*.sh;*.bat;*.ps1;*.php;*.rb;*.log;*.ini;*.cfg;*.conf\0")
    TEXT("Text Files (*.txt)\0*.txt\0")
    TEXT("Markdown (*.md)\0*.md\0")
    TEXT("JSON (*.json)\0*.json\0")
    TEXT("XML (*.xml)\0*.xml\0")
    TEXT("HTML (*.html, *.htm)\0*.html;*.htm\0")
    TEXT("CSS (*.css)\0*.css\0")
    TEXT("JavaScript (*.js)\0*.js\0")
    TEXT("TypeScript (*.ts)\0*.ts\0")
    TEXT("Python (*.py)\0*.py\0")
    TEXT("C/C++ (*.c, *.cpp, *.h, *.hpp)\0*.c;*.cpp;*.h;*.hpp\0")
    TEXT("Java (*.java)\0*.java\0")
    TEXT("Go (*.go)\0*.go\0")
    TEXT("Rust (*.rs)\0*.rs\0")
    TEXT("SQL (*.sql)\0*.sql\0")
    TEXT("YAML (*.yaml, *.yml)\0*.yaml;*.yml\0")
    TEXT("Shell Script (*.sh)\0*.sh\0")
    TEXT("Batch (*.bat, *.cmd)\0*.bat;*.cmd\0")
    TEXT("PowerShell (*.ps1)\0*.ps1\0")
    TEXT("PHP (*.php)\0*.php\0")
    TEXT("Ruby (*.rb)\0*.rb\0")
    TEXT("Log Files (*.log)\0*.log\0")
    TEXT("Config Files (*.ini, *.cfg, *.conf)\0*.ini;*.cfg;*.conf\0")
    TEXT("All Files (*.*)\0*.*\0\0");

BOOL ShowOpenDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile) {
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nMaxFile;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("txt");
    return GetOpenFileName(&ofn);
}

BOOL ShowSaveDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile) {
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nMaxFile;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = TEXT("txt");
    return GetSaveFileName(&ofn);
}

int ShowConfirmSaveDialog(HWND hwnd) {
    return MessageBox(hwnd, TEXT("Do you want to save changes?"), APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
}

void ShowAboutDialog(HWND hwnd) {
    MessageBox(hwnd, TEXT("XNote Version 1.0\n\nA fast text editor.\n\nPress F1 for shortcuts."), TEXT("About"), MB_OK | MB_ICONINFORMATION);
}

void ShowErrorDialog(HWND hwnd, const TCHAR* szMessage) {
    MessageBox(hwnd, szMessage, TEXT("Error"), MB_OK | MB_ICONERROR);
}

/* Simple and reliable Find function */
static BOOL FindTextInEdit(HWND hwndEdit, const TCHAR* szFind, BOOL bMatchCase, BOOL bWholeWord, BOOL bFromStart) {
    if (!hwndEdit || !szFind || szFind[0] == TEXT('\0')) return FALSE;
    
    /* Get text length */
    GETTEXTLENGTHEX gtl = {0};
    gtl.flags = GTL_DEFAULT | GTL_NUMCHARS;
    gtl.codepage = 1200;
    int nTextLen = (int)SendMessage(hwndEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    if (nTextLen <= 0) {
        nTextLen = GetWindowTextLength(hwndEdit);
    }
    if (nTextLen == 0) return FALSE;
    
    /* Allocate buffer */
    WCHAR* pText = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nTextLen + 4) * sizeof(WCHAR));
    if (!pText) return FALSE;
    
    /* Get text using GETTEXTEX */
    GETTEXTEX gtex = {0};
    gtex.cb = (nTextLen + 2) * sizeof(WCHAR);
    gtex.flags = GT_DEFAULT;
    gtex.codepage = 1200;
    int nGot = (int)SendMessage(hwndEdit, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)pText);
    if (nGot == 0) {
        GetWindowTextW(hwndEdit, pText, nTextLen + 1);
    }
    
    /* Get current cursor position */
    DWORD dwStart = 0, dwEnd = 0;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    int nStartPos = bFromStart ? 0 : (int)dwEnd;
    int nFindLen = (int)wcslen(szFind);
    int nActualLen = (int)wcslen(pText);
    
    BOOL bFound = FALSE;
    
    /* Search through text */
    for (int i = nStartPos; i <= nActualLen - nFindLen; i++) {
        BOOL bMatch;
        if (bMatchCase) {
            bMatch = (wcsncmp(&pText[i], szFind, nFindLen) == 0);
        } else {
            bMatch = (_wcsnicmp(&pText[i], szFind, nFindLen) == 0);
        }
        
        if (bMatch) {
            /* Check whole word */
            if (bWholeWord) {
                BOOL bWordStart = (i == 0) || !iswalnum(pText[i - 1]);
                BOOL bWordEnd = (i + nFindLen >= nActualLen) || !iswalnum(pText[i + nFindLen]);
                if (!bWordStart || !bWordEnd) continue;
            }
            
            /* Found! Select the text */
            SendMessage(hwndEdit, EM_SETSEL, (WPARAM)i, (LPARAM)(i + nFindLen));
            SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
            SetFocus(hwndEdit);
            bFound = TRUE;
            break;
        }
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    return bFound;
}

static int ReplaceAllText(HWND hwndEdit, const TCHAR* szFind, const TCHAR* szReplace, BOOL bMatchCase, BOOL bWholeWord) {
    if (!hwndEdit || !szFind || szFind[0] == TEXT('\0')) return 0;
    
    int nCount = 0;
    
    /* Disable redraw for performance */
    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
    
    /* Start from beginning */
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    
    /* Loop find and replace */
    while (FindTextInEdit(hwndEdit, szFind, bMatchCase, bWholeWord, FALSE)) {
        SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)szReplace);
        nCount++;
        if (nCount > 100000) break;
    }
    
    /* Re-enable redraw */
    SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndEdit, NULL, TRUE);
    
    /* Move cursor to beginning */
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    
    return nCount;
}

INT_PTR CALLBACK FindReplaceDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hDlg, IDC_FIND_TEXT, g_szFindText);
            SetDlgItemText(hDlg, IDC_REPLACE_TEXT, g_szReplaceText);
            CheckDlgButton(hDlg, IDC_MATCH_CASE, g_bMatchCase ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_WHOLE_WORD, g_bWholeWord ? BST_CHECKED : BST_UNCHECKED);
            RECT rcOwner, rcDlg;
            GetWindowRect(GetParent(hDlg), &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            SetWindowPos(hDlg, NULL, rcOwner.left + (rcOwner.right - rcOwner.left - (rcDlg.right - rcDlg.left)) / 2,
                         rcOwner.top + (rcOwner.bottom - rcOwner.top - (rcDlg.bottom - rcDlg.top)) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            SetFocus(GetDlgItem(hDlg, IDC_FIND_TEXT));
            return FALSE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_FIND_NEXT:
                case IDOK: {
                    GetDlgItemText(hDlg, IDC_FIND_TEXT, g_szFindText, 256);
                    g_bMatchCase = IsDlgButtonChecked(hDlg, IDC_MATCH_CASE) == BST_CHECKED;
                    g_bWholeWord = IsDlgButtonChecked(hDlg, IDC_WHOLE_WORD) == BST_CHECKED;
                    HWND hwndEdit = GetCurrentEdit();
                    if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, FALSE)) {
                        if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, TRUE)) {
                            MessageBox(hDlg, TEXT("Text not found."), TEXT("Find"), MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    return TRUE;
                }
                case IDC_REPLACE: {
                    GetDlgItemText(hDlg, IDC_FIND_TEXT, g_szFindText, 256);
                    GetDlgItemText(hDlg, IDC_REPLACE_TEXT, g_szReplaceText, 256);
                    g_bMatchCase = IsDlgButtonChecked(hDlg, IDC_MATCH_CASE) == BST_CHECKED;
                    g_bWholeWord = IsDlgButtonChecked(hDlg, IDC_WHOLE_WORD) == BST_CHECKED;
                    HWND hwndEdit = GetCurrentEdit();
                    if (!hwndEdit) return TRUE;
                    
                    DWORD dwStart = 0, dwEnd = 0;
                    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
                    
                    /* Check if there's a selection that matches */
                    if (dwStart != dwEnd) {
                        int nSelLen = (int)(dwEnd - dwStart);
                        WCHAR* pSel = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nSelLen + 4) * sizeof(WCHAR));
                        if (pSel) {
                            /* Get selected text */
                            GETTEXTEX gtex = {0};
                            gtex.cb = (nSelLen + 2) * sizeof(WCHAR);
                            gtex.flags = GT_SELECTION;
                            gtex.codepage = 1200;
                            SendMessage(hwndEdit, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)pSel);
                            
                            /* Compare with find text */
                            BOOL bMatch;
                            if (g_bMatchCase) {
                                bMatch = (wcscmp(pSel, g_szFindText) == 0);
                            } else {
                                bMatch = (_wcsicmp(pSel, g_szFindText) == 0);
                            }
                            
                            if (bMatch) {
                                SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)g_szReplaceText);
                            }
                            HeapFree(GetProcessHeap(), 0, pSel);
                        }
                    }
                    
                    /* Find next occurrence */
                    if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, FALSE)) {
                        if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, TRUE)) {
                            MessageBox(hDlg, TEXT("No more occurrences found."), TEXT("Replace"), MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    return TRUE;
                }
                case IDC_REPLACE_ALL: {
                    GetDlgItemText(hDlg, IDC_FIND_TEXT, g_szFindText, 256);
                    GetDlgItemText(hDlg, IDC_REPLACE_TEXT, g_szReplaceText, 256);
                    g_bMatchCase = IsDlgButtonChecked(hDlg, IDC_MATCH_CASE) == BST_CHECKED;
                    g_bWholeWord = IsDlgButtonChecked(hDlg, IDC_WHOLE_WORD) == BST_CHECKED;
                    HWND hwndEdit = GetCurrentEdit();
                    int nCount = ReplaceAllText(hwndEdit, g_szFindText, g_szReplaceText, g_bMatchCase, g_bWholeWord);
                    TCHAR szMsg[128];
                    _sntprintf(szMsg, 128, TEXT("Replaced %d occurrence(s)."), nCount);
                    MessageBox(hDlg, szMsg, TEXT("Replace All"), MB_OK | MB_ICONINFORMATION);
                    return TRUE;
                }
                case IDCANCEL:
                    DestroyWindow(hDlg);
                    g_hwndFindReplace = NULL;
                    return TRUE;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hDlg);
            g_hwndFindReplace = NULL;
            return TRUE;
    }
    return FALSE;
}

void ShowFindDialog(HWND hwnd) {
    if (g_hwndFindReplace) { SetFocus(g_hwndFindReplace); return; }
    g_hwndFindReplace = CreateDialog(g_AppState.hInstance, MAKEINTRESOURCE(IDD_FINDREPLACE), hwnd, FindReplaceDlgProc);
    if (g_hwndFindReplace) {
        ShowWindow(GetDlgItem(g_hwndFindReplace, IDC_REPLACE_TEXT), SW_HIDE);
        ShowWindow(GetDlgItem(g_hwndFindReplace, IDC_REPLACE), SW_HIDE);
        ShowWindow(GetDlgItem(g_hwndFindReplace, IDC_REPLACE_ALL), SW_HIDE);
        HWND hwndLabel = GetDlgItem(g_hwndFindReplace, 1001);
        if (hwndLabel) ShowWindow(hwndLabel, SW_HIDE);
        SetWindowText(g_hwndFindReplace, TEXT("Find"));
        ShowWindow(g_hwndFindReplace, SW_SHOW);
    }
}

void ShowReplaceDialog(HWND hwnd) {
    if (g_hwndFindReplace) { DestroyWindow(g_hwndFindReplace); g_hwndFindReplace = NULL; }
    g_hwndFindReplace = CreateDialog(g_AppState.hInstance, MAKEINTRESOURCE(IDD_FINDREPLACE), hwnd, FindReplaceDlgProc);
    if (g_hwndFindReplace) {
        SetWindowText(g_hwndFindReplace, TEXT("Find and Replace"));
        ShowWindow(g_hwndFindReplace, SW_SHOW);
    }
}

void FindNext(HWND hwnd) {
    (void)hwnd;
    if (g_szFindText[0] == TEXT('\0')) { ShowFindDialog(g_AppState.hwndMain); return; }
    HWND hwndEdit = GetCurrentEdit();
    if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, FALSE)) {
        if (!FindTextInEdit(hwndEdit, g_szFindText, g_bMatchCase, g_bWholeWord, TRUE)) {
            MessageBox(g_AppState.hwndMain, TEXT("Text not found."), TEXT("Find"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

INT_PTR CALLBACK HelpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_INITDIALOG: {
            RECT rcOwner, rcDlg;
            GetWindowRect(GetParent(hDlg), &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            SetWindowPos(hDlg, NULL, rcOwner.left + (rcOwner.right - rcOwner.left - (rcDlg.right - rcDlg.left)) / 2,
                         rcOwner.top + (rcOwner.bottom - rcOwner.top - (rcDlg.bottom - rcDlg.top)) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return TRUE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, LOWORD(wParam)); return TRUE; }
            break;
    }
    return FALSE;
}

void ShowHelpDialog(HWND hwnd) {
    DialogBox(g_AppState.hInstance, MAKEINTRESOURCE(IDD_HELP), hwnd, HelpDlgProc);
}
