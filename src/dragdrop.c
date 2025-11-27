#include "notepad.h"
#include <shellapi.h>
#include <shlwapi.h>

/* Valid text file extensions */
static const TCHAR* g_szValidExtensions[] = {
    TEXT(".txt"), TEXT(".c"), TEXT(".h"), TEXT(".cpp"), TEXT(".hpp"),
    TEXT(".java"), TEXT(".js"), TEXT(".ts"), TEXT(".jsx"), TEXT(".tsx"),
    TEXT(".py"), TEXT(".go"), TEXT(".rs"), TEXT(".html"), TEXT(".htm"),
    TEXT(".css"), TEXT(".json"), TEXT(".xml"), TEXT(".yaml"), TEXT(".yml"),
    TEXT(".sql"), TEXT(".php"), TEXT(".rb"), TEXT(".sh"), TEXT(".bat"),
    TEXT(".ps1"), TEXT(".md"), TEXT(".markdown"), TEXT(".log"), TEXT(".ini"),
    TEXT(".cfg"), TEXT(".conf"), TEXT(".rc"), TEXT(".gitignore"), TEXT(".env"),
    TEXT(".csv"), TEXT(".tsv"), TEXT(".makefile"), TEXT(".cmake"),
    NULL
};

/* Enable drag & drop for window */
void DragDrop_Enable(HWND hwnd) {
    DragAcceptFiles(hwnd, TRUE);
}

/* Check if file extension is valid for text editing */
BOOL DragDrop_IsValidFile(const TCHAR* szFileName) {
    if (!szFileName || szFileName[0] == TEXT('\0')) return FALSE;
    
    /* Get file extension */
    const TCHAR* szExt = PathFindExtension(szFileName);
    if (!szExt || szExt[0] == TEXT('\0')) {
        /* Files without extension are considered valid (could be text) */
        return TRUE;
    }
    
    /* Check against valid extensions */
    for (int i = 0; g_szValidExtensions[i] != NULL; i++) {
        if (_tcsicmp(szExt, g_szValidExtensions[i]) == 0) {
            return TRUE;
        }
    }
    
    /* Check if file is small enough to be text (< 10MB) */
    HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, 
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD dwSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
        
        /* If file is small, try to detect if it's binary */
        if (dwSize < 10 * 1024 * 1024) {
            /* Read first 8KB to check for binary content */
            hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                BYTE buffer[8192];
                DWORD dwRead;
                if (ReadFile(hFile, buffer, sizeof(buffer), &dwRead, NULL)) {
                    CloseHandle(hFile);
                    
                    /* Check for null bytes (binary indicator) */
                    for (DWORD i = 0; i < dwRead; i++) {
                        if (buffer[i] == 0) {
                            return FALSE; /* Binary file */
                        }
                    }
                    return TRUE; /* Appears to be text */
                }
                CloseHandle(hFile);
            }
        }
    }
    
    return FALSE;
}

/* Find if file is already open in a tab */
int DragDrop_FindOpenFile(const TCHAR* szFileName) {
    if (!szFileName || szFileName[0] == TEXT('\0')) return -1;
    
    for (int i = 0; i < g_AppState.nTabCount; i++) {
        if (_tcsicmp(g_AppState.tabs[i].szFileName, szFileName) == 0) {
            return i;
        }
    }
    return -1;
}

/* Handle WM_DROPFILES message */
void DragDrop_HandleFiles(HWND hwnd, HDROP hDrop) {
    if (!hDrop) return;
    
    /* Get number of dropped files */
    UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
    
    if (nFiles == 0) {
        DragFinish(hDrop);
        return;
    }
    
    BOOL bAnyOpened = FALSE;
    int nLastOpenedTab = -1;
    
    for (UINT i = 0; i < nFiles; i++) {
        /* Get file path */
        TCHAR szFileName[MAX_PATH];
        if (DragQueryFile(hDrop, i, szFileName, MAX_PATH) == 0) {
            continue;
        }
        
        /* Check if it's a directory */
        DWORD dwAttrib = GetFileAttributes(szFileName);
        if (dwAttrib != INVALID_FILE_ATTRIBUTES && 
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            /* Skip directories */
            continue;
        }
        
        /* Check if file is already open */
        int nExistingTab = DragDrop_FindOpenFile(szFileName);
        if (nExistingTab >= 0) {
            /* Switch to existing tab */
            SwitchToTab(hwnd, nExistingTab);
            nLastOpenedTab = nExistingTab;
            bAnyOpened = TRUE;
            continue;
        }
        
        /* Validate file type */
        if (!DragDrop_IsValidFile(szFileName)) {
            /* Show warning for invalid file */
            TCHAR szMsg[MAX_PATH + 100];
            _sntprintf(szMsg, MAX_PATH + 100, 
                TEXT("The file '%s' appears to be a binary file and cannot be opened as text."),
                PathFindFileName(szFileName));
            MessageBox(hwnd, szMsg, TEXT("Invalid File Type"), MB_OK | MB_ICONWARNING);
            continue;
        }
        
        /* Check if current tab is empty untitled */
        TabState* pCurrentTab = GetCurrentTabState();
        BOOL bUseCurrentTab = FALSE;
        
        if (pCurrentTab && pCurrentTab->bUntitled && !pCurrentTab->bModified) {
            /* Check if edit control is empty */
            if (pCurrentTab->hwndEdit) {
                int nLen = GetWindowTextLength(pCurrentTab->hwndEdit);
                if (nLen == 0) {
                    bUseCurrentTab = TRUE;
                }
            }
        }
        
        if (bUseCurrentTab) {
            /* Load file into current tab */
            TabState* pTab = GetCurrentTabState();
            if (pTab && pTab->hwndEdit && ReadFileContent(pTab->hwndEdit, szFileName)) {
                _tcscpy(pTab->szFileName, szFileName);
                pTab->bUntitled = FALSE;
                pTab->bModified = FALSE;
                UpdateTabTitle(g_AppState.nCurrentTab);
                UpdateWindowTitle(hwnd);
                nLastOpenedTab = g_AppState.nCurrentTab;
                bAnyOpened = TRUE;
            }
        } else {
            /* Create new tab and load file */
            int nNewTab = AddNewTab(hwnd, PathFindFileName(szFileName));
            if (nNewTab >= 0) {
                TabState* pTab = &g_AppState.tabs[nNewTab];
                if (pTab->hwndEdit && ReadFileContent(pTab->hwndEdit, szFileName)) {
                    _tcscpy(pTab->szFileName, szFileName);
                    pTab->bUntitled = FALSE;
                    pTab->bModified = FALSE;
                    UpdateTabTitle(nNewTab);
                    UpdateWindowTitle(hwnd);
                    nLastOpenedTab = nNewTab;
                    bAnyOpened = TRUE;
                } else {
                    /* Failed to load, close the tab */
                    CloseTab(hwnd, nNewTab);
                }
            }
        }
    }
    
    /* Switch to last opened tab */
    if (bAnyOpened && nLastOpenedTab >= 0) {
        SwitchToTab(hwnd, nLastOpenedTab);
    }
    
    DragFinish(hDrop);
}
