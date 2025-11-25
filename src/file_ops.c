#include "notepad.h"
#include "syntax.h"
#include <stdio.h>

/* Update window title based on current tab */
void UpdateWindowTitle(HWND hwnd) {
    TCHAR szTitle[MAX_PATH + 32];
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab) {
        _sntprintf(szTitle, MAX_PATH + 32, TEXT("Untitled - %s"), APP_NAME);
    } else if (pTab->bUntitled) {
        _sntprintf(szTitle, MAX_PATH + 32, TEXT("Untitled - %s"), APP_NAME);
    } else {
        TCHAR* pFileName = _tcsrchr(pTab->szFileName, TEXT('\\'));
        if (pFileName) {
            pFileName++;
        } else {
            pFileName = pTab->szFileName;
        }
        _sntprintf(szTitle, MAX_PATH + 32, TEXT("%s - %s"), pFileName, APP_NAME);
    }
    
    SetWindowText(hwnd, szTitle);
}

/* Check if buffer has UTF-8 BOM */
static BOOL HasUtf8Bom(const char* pBuffer, DWORD dwSize) {
    if (dwSize >= 3) {
        return (unsigned char)pBuffer[0] == 0xEF &&
               (unsigned char)pBuffer[1] == 0xBB &&
               (unsigned char)pBuffer[2] == 0xBF;
    }
    return FALSE;
}

/* Detect line ending type from buffer */
static LineEndingType DetectLineEnding(const char* pBuffer, DWORD dwSize) {
    BOOL bHasCR = FALSE;
    BOOL bHasLF = FALSE;
    BOOL bHasCRLF = FALSE;
    
    for (DWORD i = 0; i < dwSize; i++) {
        if (pBuffer[i] == '\r') {
            if (i + 1 < dwSize && pBuffer[i + 1] == '\n') {
                bHasCRLF = TRUE;
                i++; /* Skip the LF */
            } else {
                bHasCR = TRUE;
            }
        } else if (pBuffer[i] == '\n') {
            bHasLF = TRUE;
        }
    }
    
    /* Prioritize: CRLF > LF > CR */
    if (bHasCRLF) return LINE_ENDING_CRLF;
    if (bHasLF) return LINE_ENDING_LF;
    if (bHasCR) return LINE_ENDING_CR;
    
    /* Default to Windows line ending */
    return LINE_ENDING_CRLF;
}

/* Read file content into edit control - simple and reliable method */
BOOL ReadFileContent(HWND hEdit, const TCHAR* szFileName) {
    HANDLE hFile;
    LARGE_INTEGER liFileSize;
    DWORD dwFileSize, dwBytesRead;
    char* pBuffer;
    WCHAR* pWideBuffer;
    int nWideLen;
    const char* pText;
    DWORD dwTextSize;
    
    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    /* Get file size */
    if (!GetFileSizeEx(hFile, &liFileSize)) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    /* Limit to 100MB for practical use with this method */
    if (liFileSize.HighPart > 0 || liFileSize.LowPart > 100 * 1024 * 1024) {
        CloseHandle(hFile);
        ShowErrorDialog(GetParent(hEdit), TEXT("File is too large (max 100MB)."));
        return FALSE;
    }
    
    dwFileSize = liFileSize.LowPart;
    
    /* Allocate buffer */
    pBuffer = (char*)HeapAlloc(GetProcessHeap(), 0, dwFileSize + 1);
    if (!pBuffer) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    /* Read entire file */
    if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL)) {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        CloseHandle(hFile);
        return FALSE;
    }
    
    pBuffer[dwBytesRead] = '\0';
    CloseHandle(hFile);
    
    /* Detect line ending type and store in tab state */
    TabState* pTab = GetCurrentTabState();
    if (pTab) {
        pTab->lineEnding = DetectLineEnding(pBuffer, dwBytesRead);
    }
    
    /* Check for UTF-8 BOM and skip it */
    if (HasUtf8Bom(pBuffer, dwBytesRead)) {
        pText = pBuffer + 3;
        dwTextSize = dwBytesRead - 3;
    } else {
        pText = pBuffer;
        dwTextSize = dwBytesRead;
    }
    
    /* Try UTF-8 first, then ANSI */
    nWideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize, NULL, 0);
    
    if (nWideLen == 0) {
        /* UTF-8 failed, try ANSI */
        nWideLen = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize, NULL, 0);
    }
    
    if (nWideLen > 0) {
        pWideBuffer = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (pWideBuffer) {
            /* Try UTF-8 first */
            int nConverted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize, pWideBuffer, nWideLen);
            if (nConverted == 0) {
                /* Fallback to ANSI */
                nConverted = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize, pWideBuffer, nWideLen);
            }
            
            if (nConverted > 0) {
                pWideBuffer[nConverted] = L'\0';
                SetWindowTextW(hEdit, pWideBuffer);
            }
            
            HeapFree(GetProcessHeap(), 0, pWideBuffer);
        }
    } else {
        /* Last resort: set as ANSI directly */
        SetWindowTextA(hEdit, pText);
    }
    
    HeapFree(GetProcessHeap(), 0, pBuffer);
    
    /* Move cursor to beginning */
    SendMessage(hEdit, EM_SETSEL, 0, 0);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    
    return TRUE;
}

/* UTF-8 BOM bytes */
static const unsigned char UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

/* Write edit control content to file with UTF-8 encoding */
BOOL WriteFileContent(HWND hEdit, const TCHAR* szFileName) {
    HANDLE hFile;
    DWORD dwBytesWritten;
    WCHAR* pWideBuffer;
    char* pUtf8Buffer;
    int nWideLen, nUtf8Len;
    
    hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    /* Get text length */
    nWideLen = GetWindowTextLengthW(hEdit);
    
    /* Write UTF-8 BOM first */
    WriteFile(hFile, UTF8_BOM, sizeof(UTF8_BOM), &dwBytesWritten, NULL);
    
    if (nWideLen > 0) {
        /* Allocate buffer for wide string */
        pWideBuffer = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (!pWideBuffer) {
            CloseHandle(hFile);
            return FALSE;
        }
        
        GetWindowTextW(hEdit, pWideBuffer, nWideLen + 1);
        
        /* Convert wide string to UTF-8 */
        nUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pWideBuffer, nWideLen, NULL, 0, NULL, NULL);
        if (nUtf8Len > 0) {
            pUtf8Buffer = (char*)HeapAlloc(GetProcessHeap(), 0, nUtf8Len);
            if (pUtf8Buffer) {
                WideCharToMultiByte(CP_UTF8, 0, pWideBuffer, nWideLen, pUtf8Buffer, nUtf8Len, NULL, NULL);
                WriteFile(hFile, pUtf8Buffer, nUtf8Len, &dwBytesWritten, NULL);
                HeapFree(GetProcessHeap(), 0, pUtf8Buffer);
            }
        }
        
        HeapFree(GetProcessHeap(), 0, pWideBuffer);
    }
    
    CloseHandle(hFile);
    return TRUE;
}

/* Create new document in current tab */
BOOL FileNew(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return FALSE;
    
    /* Check for unsaved changes */
    if (pTab->bModified) {
        if (!PromptSaveChanges(hwnd)) {
            return FALSE;
        }
    }
    
    /* Clear edit control */
    SetWindowText(pTab->hwndEdit, TEXT(""));
    
    /* Reset tab state */
    InitTabState(pTab);
    pTab->hwndEdit = GetCurrentEdit(); /* Keep the edit control */
    
    /* Update tab and window title */
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);
    
    return TRUE;
}

/* Open existing file */
BOOL FileOpen(HWND hwnd) {
    TCHAR szFileName[MAX_PATH] = TEXT("");
    TabState* pTab;
    HWND hwndEdit;
    
    /* Show open dialog first */
    if (!ShowOpenDialog(hwnd, szFileName, MAX_PATH)) {
        return FALSE;
    }
    
    /* Get current tab */
    pTab = GetCurrentTabState();
    
    /* If current tab is untitled and unmodified, use it; otherwise create new tab */
    if (!pTab || !pTab->bUntitled || pTab->bModified) {
        int nNewTab = AddNewTab(hwnd, TEXT("Loading..."));
        if (nNewTab < 0) return FALSE;
    }
    
    /* Get the edit control handle directly */
    hwndEdit = GetCurrentEdit();
    if (!hwndEdit) {
        ShowErrorDialog(hwnd, TEXT("Failed to get edit control."));
        return FALSE;
    }
    
    /* Read file content */
    if (!ReadFileContent(hwndEdit, szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to open file."));
        return FALSE;
    }
    
    /* Get tab state again after potential tab creation */
    pTab = GetCurrentTabState();
    if (!pTab) return FALSE;
    
    /* Update tab state */
    _tcscpy(pTab->szFileName, szFileName);
    pTab->bModified = FALSE;
    pTab->bUntitled = FALSE;
    
    /* Detect language and apply syntax highlighting */
    pTab->language = DetectLanguage(szFileName);
    if (g_bSyntaxHighlight && pTab->language != LANG_NONE) {
        ApplySyntaxHighlighting(hwndEdit, pTab->language);
    }
    
    /* Update titles */
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);
    
    /* Force redraw */
    InvalidateRect(hwndEdit, NULL, TRUE);
    UpdateWindow(hwndEdit);
    
    return TRUE;
}

/* Save current file */
BOOL FileSave(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return FALSE;
    
    if (pTab->bUntitled) {
        return FileSaveAs(hwnd);
    }
    
    if (!WriteFileContent(pTab->hwndEdit, pTab->szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to save file."));
        return FALSE;
    }
    
    pTab->bModified = FALSE;
    UpdateTabTitle(g_AppState.nCurrentTab);
    
    return TRUE;
}

/* Save file with new name */
BOOL FileSaveAs(HWND hwnd) {
    TCHAR szFileName[MAX_PATH];
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab) return FALSE;
    
    if (pTab->bUntitled) {
        _tcscpy(szFileName, TEXT("Untitled.txt"));
    } else {
        _tcscpy(szFileName, pTab->szFileName);
    }
    
    if (!ShowSaveDialog(hwnd, szFileName, MAX_PATH)) {
        return FALSE;
    }
    
    if (!WriteFileContent(pTab->hwndEdit, szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to save file."));
        return FALSE;
    }
    
    _tcscpy(pTab->szFileName, szFileName);
    pTab->bModified = FALSE;
    pTab->bUntitled = FALSE;
    
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);
    
    return TRUE;
}

/* Prompt user to save changes */
BOOL PromptSaveChanges(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab || !pTab->bModified) {
        return TRUE;
    }
    
    int nResult = ShowConfirmSaveDialog(hwnd);
    
    switch (nResult) {
        case IDYES:
            return FileSave(hwnd);
        case IDNO:
            return TRUE;
        case IDCANCEL:
        default:
            return FALSE;
    }
}
