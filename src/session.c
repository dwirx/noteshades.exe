/**
 * session.c - Session persistence for XNote
 * Menyimpan dan memuat state aplikasi dalam format JSON
 */

#include "session.h"
#include "notepad.h"
#include <shlobj.h>
#include <stdio.h>

/* Session dirty flag */
static BOOL g_bSessionDirty = FALSE;

/* Current session version */
#define SESSION_VERSION 1

/* Get session file path in AppData folder */
BOOL GetSessionFilePath(TCHAR* szPath, DWORD dwSize) {
    TCHAR szAppData[MAX_PATH];
    
    /* Get AppData\Local folder */
    if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szAppData))) {
        return FALSE;
    }
    
    /* Create XNote folder if not exists */
    _sntprintf(szPath, dwSize, TEXT("%s\\XNote"), szAppData);
    CreateDirectory(szPath, NULL);
    
    /* Append session file name */
    _sntprintf(szPath, dwSize, TEXT("%s\\XNote\\%s"), szAppData, SESSION_FILE_NAME);
    
    return TRUE;
}

/* Escape string for JSON */
static void EscapeJsonString(const TCHAR* src, char* dest, int destSize) {
    int j = 0;
    for (int i = 0; src[i] && j < destSize - 2; i++) {
        TCHAR c = src[i];
        if (c == '\\') {
            dest[j++] = '\\';
            dest[j++] = '\\';
        } else if (c == '"') {
            dest[j++] = '\\';
            dest[j++] = '"';
        } else if (c == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else if (c == '\r') {
            dest[j++] = '\\';
            dest[j++] = 'r';
        } else if (c == '\t') {
            dest[j++] = '\\';
            dest[j++] = 't';
        } else if (c < 128) {
            dest[j++] = (char)c;
        }
    }
    dest[j] = '\0';
}

/* Write JSON to file */
static BOOL WriteJsonToFile(const TCHAR* szPath, SessionData* pSession) {
    FILE* fp;
    char escapedPath[MAX_PATH * 2];
    
#ifdef UNICODE
    fp = _wfopen(szPath, L"w");
#else
    fp = fopen(szPath, "w");
#endif
    
    if (!fp) return FALSE;
    
    /* Write JSON header */
    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": %d,\n", pSession->nVersion);
    fprintf(fp, "  \"activeTab\": %d,\n", pSession->nActiveTab);
    fprintf(fp, "  \"wordWrap\": %s,\n", pSession->bWordWrap ? "true" : "false");
    fprintf(fp, "  \"showLineNumbers\": %s,\n", pSession->bShowLineNumbers ? "true" : "false");
    fprintf(fp, "  \"relativeLineNumbers\": %s,\n", pSession->bRelativeLineNumbers ? "true" : "false");
    fprintf(fp, "  \"window\": {\n");
    fprintf(fp, "    \"x\": %d,\n", pSession->nWindowX);
    fprintf(fp, "    \"y\": %d,\n", pSession->nWindowY);
    fprintf(fp, "    \"width\": %d,\n", pSession->nWindowWidth);
    fprintf(fp, "    \"height\": %d,\n", pSession->nWindowHeight);
    fprintf(fp, "    \"maximized\": %s\n", pSession->bMaximized ? "true" : "false");
    fprintf(fp, "  },\n");
    
    /* Write tabs array */
    fprintf(fp, "  \"tabs\": [\n");
    for (int i = 0; i < pSession->nTabCount; i++) {
        SessionTab* pTab = &pSession->tabs[i];
        EscapeJsonString(pTab->szFilePath, escapedPath, sizeof(escapedPath));
        
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"filePath\": \"%s\",\n", escapedPath);
        fprintf(fp, "      \"cursorPos\": %d,\n", pTab->nCursorPos);
        fprintf(fp, "      \"scrollPos\": %d,\n", pTab->nScrollPos);
        fprintf(fp, "      \"selStart\": %d,\n", pTab->nSelStart);
        fprintf(fp, "      \"selEnd\": %d,\n", pTab->nSelEnd);
        fprintf(fp, "      \"modified\": %s,\n", pTab->bModified ? "true" : "false");
        fprintf(fp, "      \"language\": %d\n", (int)pTab->language);
        fprintf(fp, "    }%s\n", (i < pSession->nTabCount - 1) ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return TRUE;
}


/* Simple JSON parser helpers */
static char* SkipWhitespace(char* p) {
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    return p;
}

static char* FindChar(char* p, char c) {
    while (*p && *p != c) p++;
    return p;
}

static int ParseInt(char* p) {
    return atoi(p);
}

static BOOL ParseBool(char* p) {
    return (strncmp(p, "true", 4) == 0);
}

/* Parse escaped JSON string */
static void ParseJsonString(char* src, TCHAR* dest, int destSize) {
    int j = 0;
    for (int i = 0; src[i] && src[i] != '"' && j < destSize - 1; i++) {
        if (src[i] == '\\' && src[i+1]) {
            i++;
            switch (src[i]) {
                case '\\': dest[j++] = '\\'; break;
                case '"': dest[j++] = '"'; break;
                case 'n': dest[j++] = '\n'; break;
                case 'r': dest[j++] = '\r'; break;
                case 't': dest[j++] = '\t'; break;
                default: dest[j++] = src[i]; break;
            }
        } else {
            dest[j++] = (TCHAR)src[i];
        }
    }
    dest[j] = '\0';
}

/* Read and parse JSON from file */
static BOOL ReadJsonFromFile(const TCHAR* szPath, SessionData* pSession) {
    FILE* fp;
    char* pBuffer;
    long fileSize;
    char* p;
    int tabIndex = 0;
    
    ZeroMemory(pSession, sizeof(SessionData));
    
#ifdef UNICODE
    fp = _wfopen(szPath, L"r");
#else
    fp = fopen(szPath, "r");
#endif
    
    if (!fp) return FALSE;
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > 1024 * 1024) {
        fclose(fp);
        return FALSE;
    }
    
    /* Read file content */
    pBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize + 1);
    if (!pBuffer) {
        fclose(fp);
        return FALSE;
    }
    
    fread(pBuffer, 1, fileSize, fp);
    fclose(fp);
    
    /* Simple JSON parsing */
    p = pBuffer;
    
    /* Parse version */
    p = strstr(p, "\"version\"");
    if (p) {
        p = FindChar(p, ':');
        p = SkipWhitespace(p + 1);
        pSession->nVersion = ParseInt(p);
    }
    
    /* Parse activeTab */
    p = pBuffer;
    p = strstr(p, "\"activeTab\"");
    if (p) {
        p = FindChar(p, ':');
        p = SkipWhitespace(p + 1);
        pSession->nActiveTab = ParseInt(p);
    }
    
    /* Parse wordWrap */
    p = pBuffer;
    p = strstr(p, "\"wordWrap\"");
    if (p) {
        p = FindChar(p, ':');
        p = SkipWhitespace(p + 1);
        pSession->bWordWrap = ParseBool(p);
    }
    
    /* Parse showLineNumbers */
    p = pBuffer;
    p = strstr(p, "\"showLineNumbers\"");
    if (p) {
        p = FindChar(p, ':');
        p = SkipWhitespace(p + 1);
        pSession->bShowLineNumbers = ParseBool(p);
    }
    
    /* Parse relativeLineNumbers */
    p = pBuffer;
    p = strstr(p, "\"relativeLineNumbers\"");
    if (p) {
        p = FindChar(p, ':');
        p = SkipWhitespace(p + 1);
        pSession->bRelativeLineNumbers = ParseBool(p);
    }
    
    /* Parse window settings */
    p = pBuffer;
    p = strstr(p, "\"window\"");
    if (p) {
        char* windowEnd = FindChar(p, '}');
        char* temp;
        
        temp = strstr(p, "\"x\"");
        if (temp && temp < windowEnd) {
            temp = FindChar(temp, ':');
            temp = SkipWhitespace(temp + 1);
            pSession->nWindowX = ParseInt(temp);
        }
        
        temp = strstr(p, "\"y\"");
        if (temp && temp < windowEnd) {
            temp = FindChar(temp, ':');
            temp = SkipWhitespace(temp + 1);
            pSession->nWindowY = ParseInt(temp);
        }
        
        temp = strstr(p, "\"width\"");
        if (temp && temp < windowEnd) {
            temp = FindChar(temp, ':');
            temp = SkipWhitespace(temp + 1);
            pSession->nWindowWidth = ParseInt(temp);
        }
        
        temp = strstr(p, "\"height\"");
        if (temp && temp < windowEnd) {
            temp = FindChar(temp, ':');
            temp = SkipWhitespace(temp + 1);
            pSession->nWindowHeight = ParseInt(temp);
        }
        
        temp = strstr(p, "\"maximized\"");
        if (temp && temp < windowEnd) {
            temp = FindChar(temp, ':');
            temp = SkipWhitespace(temp + 1);
            pSession->bMaximized = ParseBool(temp);
        }
    }
    
    /* Parse tabs array */
    p = pBuffer;
    p = strstr(p, "\"tabs\"");
    if (p) {
        p = FindChar(p, '[');
        p++;
        
        while (*p && tabIndex < MAX_TABS) {
            char* tabStart = FindChar(p, '{');
            if (!*tabStart) break;
            
            char* tabEnd = FindChar(tabStart, '}');
            if (!*tabEnd) break;
            
            SessionTab* pTab = &pSession->tabs[tabIndex];
            char* temp;
            
            /* Parse filePath */
            temp = strstr(tabStart, "\"filePath\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                if (*temp == '"') {
                    temp++;
                    ParseJsonString(temp, pTab->szFilePath, MAX_PATH);
                }
            }
            
            /* Parse cursorPos */
            temp = strstr(tabStart, "\"cursorPos\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->nCursorPos = ParseInt(temp);
            }
            
            /* Parse scrollPos */
            temp = strstr(tabStart, "\"scrollPos\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->nScrollPos = ParseInt(temp);
            }
            
            /* Parse selStart */
            temp = strstr(tabStart, "\"selStart\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->nSelStart = ParseInt(temp);
            }
            
            /* Parse selEnd */
            temp = strstr(tabStart, "\"selEnd\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->nSelEnd = ParseInt(temp);
            }
            
            /* Parse modified */
            temp = strstr(tabStart, "\"modified\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->bModified = ParseBool(temp);
            }
            
            /* Parse language */
            temp = strstr(tabStart, "\"language\"");
            if (temp && temp < tabEnd) {
                temp = FindChar(temp, ':');
                temp = SkipWhitespace(temp + 1);
                pTab->language = (LanguageType)ParseInt(temp);
            }
            
            tabIndex++;
            p = tabEnd + 1;
        }
    }
    
    pSession->nTabCount = tabIndex;
    
    HeapFree(GetProcessHeap(), 0, pBuffer);
    return TRUE;
}


/* Save current session to JSON file */
BOOL SaveSession(HWND hwnd) {
    TCHAR szPath[MAX_PATH];
    SessionData session;
    WINDOWPLACEMENT wp;
    
    if (!GetSessionFilePath(szPath, MAX_PATH)) {
        return FALSE;
    }
    
    ZeroMemory(&session, sizeof(SessionData));
    session.nVersion = SESSION_VERSION;
    session.nTabCount = g_AppState.nTabCount;
    session.nActiveTab = g_AppState.nCurrentTab;
    session.bWordWrap = g_AppState.bWordWrap;
    session.bShowLineNumbers = g_AppState.bShowLineNumbers;
    session.bRelativeLineNumbers = g_AppState.bRelativeLineNumbers;
    
    /* Get window position and size */
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);
    
    session.bMaximized = (wp.showCmd == SW_MAXIMIZE);
    session.nWindowX = wp.rcNormalPosition.left;
    session.nWindowY = wp.rcNormalPosition.top;
    session.nWindowWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    session.nWindowHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    
    /* Save each tab's state */
    for (int i = 0; i < g_AppState.nTabCount && i < MAX_TABS; i++) {
        TabState* pTab = &g_AppState.tabs[i];
        SessionTab* pSessionTab = &session.tabs[i];
        
        /* Copy file path */
        if (pTab->bUntitled) {
            pSessionTab->szFilePath[0] = '\0';  /* Empty path for untitled */
        } else {
            _tcscpy(pSessionTab->szFilePath, pTab->szFileName);
        }
        
        /* Get cursor and selection info */
        if (pTab->hwndEdit) {
            DWORD dwStart, dwEnd;
            SendMessage(pTab->hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
            pSessionTab->nSelStart = (int)dwStart;
            pSessionTab->nSelEnd = (int)dwEnd;
            pSessionTab->nCursorPos = (int)dwEnd;
            pSessionTab->nScrollPos = (int)SendMessage(pTab->hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
        }
        
        pSessionTab->bModified = pTab->bModified;
        pSessionTab->language = pTab->language;
    }
    
    g_bSessionDirty = FALSE;
    return WriteJsonToFile(szPath, &session);
}

/* Load session from JSON file */
BOOL LoadSession(HWND hwnd) {
    TCHAR szPath[MAX_PATH];
    SessionData session;
    
    if (!GetSessionFilePath(szPath, MAX_PATH)) {
        return FALSE;
    }
    
    /* Check if session file exists */
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }
    
    if (!ReadJsonFromFile(szPath, &session)) {
        return FALSE;
    }
    
    /* Validate session data */
    if (session.nVersion != SESSION_VERSION || session.nTabCount <= 0) {
        return FALSE;
    }
    
    /* Apply window settings */
    if (session.nWindowWidth > 0 && session.nWindowHeight > 0) {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &wp);
        
        wp.rcNormalPosition.left = session.nWindowX;
        wp.rcNormalPosition.top = session.nWindowY;
        wp.rcNormalPosition.right = session.nWindowX + session.nWindowWidth;
        wp.rcNormalPosition.bottom = session.nWindowY + session.nWindowHeight;
        wp.showCmd = session.bMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL;
        
        SetWindowPlacement(hwnd, &wp);
    }
    
    /* Apply global settings */
    g_AppState.bWordWrap = session.bWordWrap;
    g_AppState.bShowLineNumbers = session.bShowLineNumbers;
    g_AppState.bRelativeLineNumbers = session.bRelativeLineNumbers;
    
    /* Update menu check marks */
    HMENU hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu, IDM_FORMAT_WORDWRAP, session.bWordWrap ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_LINENUMBERS, session.bShowLineNumbers ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_RELATIVENUM, session.bRelativeLineNumbers ? MF_CHECKED : MF_UNCHECKED);
    
    /* Close default tab created at startup */
    if (g_AppState.nTabCount == 1 && g_AppState.tabs[0].bUntitled && !g_AppState.tabs[0].bModified) {
        /* Will be replaced by loaded tabs */
        if (g_AppState.tabs[0].hwndEdit) {
            DestroyWindow(g_AppState.tabs[0].hwndEdit);
        }
        if (g_AppState.tabs[0].lineNumState.hwndLineNumbers) {
            DestroyWindow(g_AppState.tabs[0].lineNumState.hwndLineNumbers);
        }
        TabCtrl_DeleteItem(g_AppState.hwndTab, 0);
        g_AppState.nTabCount = 0;
        g_AppState.nCurrentTab = -1;
    }
    
    /* Load each tab */
    for (int i = 0; i < session.nTabCount && i < MAX_TABS; i++) {
        SessionTab* pSessionTab = &session.tabs[i];
        
        /* Check if file exists (skip if not) */
        if (pSessionTab->szFilePath[0] != '\0') {
            if (GetFileAttributes(pSessionTab->szFilePath) == INVALID_FILE_ATTRIBUTES) {
                continue;  /* Skip non-existent files */
            }
        }
        
        /* Create new tab */
        TCHAR szTitle[MAX_PATH];
        if (pSessionTab->szFilePath[0] == '\0') {
            _tcscpy(szTitle, TEXT("Untitled"));
        } else {
            TCHAR* pFileName = _tcsrchr(pSessionTab->szFilePath, TEXT('\\'));
            if (pFileName) {
                _tcscpy(szTitle, pFileName + 1);
            } else {
                _tcscpy(szTitle, pSessionTab->szFilePath);
            }
        }
        
        int nNewTab = AddNewTab(hwnd, szTitle);
        if (nNewTab < 0) continue;
        
        TabState* pTab = &g_AppState.tabs[nNewTab];
        
        /* Load file content if not untitled */
        if (pSessionTab->szFilePath[0] != '\0') {
            if (ReadFileContent(pTab->hwndEdit, pSessionTab->szFilePath)) {
                _tcscpy(pTab->szFileName, pSessionTab->szFilePath);
                pTab->bUntitled = FALSE;
                pTab->language = pSessionTab->language;
                
                /* Restore cursor and scroll position */
                SendMessage(pTab->hwndEdit, EM_SETSEL, pSessionTab->nSelStart, pSessionTab->nSelEnd);
                SendMessage(pTab->hwndEdit, EM_LINESCROLL, 0, pSessionTab->nScrollPos);
                
                UpdateTabTitle(nNewTab);
            }
        }
    }
    
    /* If no tabs were loaded, create a default one */
    if (g_AppState.nTabCount == 0) {
        AddNewTab(hwnd, TEXT("Untitled"));
    }
    
    /* Switch to the previously active tab */
    if (session.nActiveTab >= 0 && session.nActiveTab < g_AppState.nTabCount) {
        SwitchToTab(hwnd, session.nActiveTab);
    } else {
        SwitchToTab(hwnd, 0);
    }
    
    UpdateWindowTitle(hwnd);
    return TRUE;
}

/* Save session on application exit */
BOOL SaveSessionOnExit(HWND hwnd) {
    return SaveSession(hwnd);
}

/* Sync newly opened file to session */
void SyncNewFileToSession(int nTabIndex) {
    (void)nTabIndex;  /* Parameter reserved for future use */
    g_bSessionDirty = TRUE;
}

/* Mark session as dirty (needs saving) */
void MarkSessionDirty(void) {
    g_bSessionDirty = TRUE;
}

/* Initialize session system */
void InitSessionSystem(HWND hwnd) {
    /* Set up auto-save timer */
    SetTimer(hwnd, TIMER_AUTOSAVE, AUTOSAVE_INTERVAL, NULL);
}

/* Cleanup session system */
void CleanupSessionSystem(void) {
    /* Nothing to cleanup currently */
}

/* Handle auto-save timer */
void HandleSessionTimer(HWND hwnd) {
    if (g_bSessionDirty) {
        SaveSession(hwnd);
    }
}
