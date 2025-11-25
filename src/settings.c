/*
 * settings.c - Settings persistence for XNote using JSON
 */
#include "notepad.h"
#include "syntax.h"
#include "vim_mode.h"
#include <shlobj.h>
#include <stdio.h>

#define SETTINGS_FILENAME TEXT("\\xnote_settings.json")
#define MAX_RECENT_FILES 10

static TCHAR g_RecentFiles[MAX_RECENT_FILES][MAX_PATH];
static int g_nRecentCount = 0;
static TCHAR g_SessionFiles[MAX_TABS][MAX_PATH];
static int g_nSessionCount = 0;
static int g_nSessionActiveTab = 0;
static int g_nWindowX = CW_USEDEFAULT;
static int g_nWindowY = CW_USEDEFAULT;
static int g_nWindowWidth = 1200;
static int g_nWindowHeight = 800;
static BOOL g_bWindowMaximized = FALSE;

static BOOL GetSettingsPath(TCHAR* szPath, DWORD nSize) {
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        _tcsncat(szPath, TEXT("\\XNote"), nSize - _tcslen(szPath) - 1);
        CreateDirectory(szPath, NULL);
        _tcsncat(szPath, SETTINGS_FILENAME, nSize - _tcslen(szPath) - 1);
        return TRUE;
    }
    return FALSE;
}

static void EscapeJsonString(const TCHAR* src, char* dest, int destSize) {
    int j = 0, i;
    for (i = 0; src[i] && j < destSize - 2; i++) {
        if (src[i] == '\\') { dest[j++] = '\\'; dest[j++] = '\\'; }
        else if (src[i] == '"') { dest[j++] = '\\'; dest[j++] = '"'; }
        else if (src[i] < 128) { dest[j++] = (char)src[i]; }
    }
    dest[j] = '\0';
}

static void UnescapeJsonString(const char* src, TCHAR* dest, int destSize) {
    int j = 0, i;
    for (i = 0; src[i] && j < destSize - 1; i++) {
        if (src[i] == '\\' && src[i+1]) {
            i++;
            if (src[i] == '\\') dest[j++] = '\\';
            else if (src[i] == '"') dest[j++] = '"';
            else dest[j++] = (TCHAR)src[i];
        } else { dest[j++] = (TCHAR)src[i]; }
    }
    dest[j] = '\0';
}


static BOOL ParseJsonBool(const char* json, const char* key, BOOL def) {
    char sk[64]; const char* p;
    sprintf(sk, "\"%s\"", key);
    p = strstr(json, sk);
    if (!p) return def;
    p = strchr(p, ':'); if (!p) return def;
    while (*++p == ' ' || *p == '\t');
    if (strncmp(p, "true", 4) == 0) return TRUE;
    return (strncmp(p, "false", 5) == 0) ? FALSE : def;
}

static int ParseJsonInt(const char* json, const char* key, int def) {
    char sk[64]; const char* p;
    sprintf(sk, "\"%s\"", key);
    p = strstr(json, sk);
    if (!p) return def;
    p = strchr(p, ':'); if (!p) return def;
    while (*++p == ' ' || *p == '\t');
    return atoi(p);
}

static int ParseJsonStringArray(const char* json, const char* key, TCHAR arr[][MAX_PATH], int maxCount) {
    char sk[64]; const char* p; int count = 0;
    sprintf(sk, "\"%s\"", key);
    p = strstr(json, sk); if (!p) return 0;
    p = strchr(p, '['); if (!p) return 0; p++;
    while (*p && *p != ']' && count < maxCount) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
        if (*p == '"') {
            char temp[MAX_PATH * 2]; int i = 0; p++;
            while (*p && *p != '"' && i < (int)sizeof(temp) - 1) {
                if (*p == '\\' && *(p+1)) { temp[i++] = *p++; }
                temp[i++] = *p++;
            }
            temp[i] = '\0'; if (*p == '"') p++;
            UnescapeJsonString(temp, arr[count], MAX_PATH); count++;
        } else if (*p && *p != ']') { p++; }
    }
    return count;
}


void LoadSettings(void) {
    TCHAR szPath[MAX_PATH]; FILE* fp; long size; char* json;
    if (!GetSettingsPath(szPath, MAX_PATH)) return;
    fp = _tfopen(szPath, TEXT("rb")); if (!fp) return;
    fseek(fp, 0, SEEK_END); size = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > 1024 * 1024) { fclose(fp); return; }
    json = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    if (!json) { fclose(fp); return; }
    fread(json, 1, size, fp); fclose(fp); json[size] = '\0';
    g_AppState.bWordWrap = ParseJsonBool(json, "wordWrap", FALSE);
    g_AppState.bShowLineNumbers = ParseJsonBool(json, "showLineNumbers", FALSE);
    g_AppState.bRelativeLineNumbers = ParseJsonBool(json, "relativeLineNumbers", FALSE);
    g_bSyntaxHighlight = ParseJsonBool(json, "syntaxHighlight", TRUE);
    SetVimModeEnabled(ParseJsonBool(json, "vimMode", FALSE));
    g_nWindowX = ParseJsonInt(json, "windowX", CW_USEDEFAULT);
    g_nWindowY = ParseJsonInt(json, "windowY", CW_USEDEFAULT);
    g_nWindowWidth = ParseJsonInt(json, "windowWidth", 1200);
    g_nWindowHeight = ParseJsonInt(json, "windowHeight", 800);
    g_bWindowMaximized = ParseJsonBool(json, "windowMaximized", FALSE);
    g_nRecentCount = ParseJsonStringArray(json, "recentFiles", g_RecentFiles, MAX_RECENT_FILES);
    g_nSessionCount = ParseJsonStringArray(json, "sessionFiles", g_SessionFiles, MAX_TABS);
    g_nSessionActiveTab = ParseJsonInt(json, "sessionActiveTab", 0);
    HeapFree(GetProcessHeap(), 0, json);
}


void SaveSettings(void) {
    TCHAR szPath[MAX_PATH]; FILE* fp; int i, sessionCount = 0;
    if (!GetSettingsPath(szPath, MAX_PATH)) return;
    fp = _tfopen(szPath, TEXT("w")); if (!fp) return;
    fprintf(fp, "{\n");
    fprintf(fp, "  \"wordWrap\": %s,\n", g_AppState.bWordWrap ? "true" : "false");
    fprintf(fp, "  \"showLineNumbers\": %s,\n", g_AppState.bShowLineNumbers ? "true" : "false");
    fprintf(fp, "  \"relativeLineNumbers\": %s,\n", g_AppState.bRelativeLineNumbers ? "true" : "false");
    fprintf(fp, "  \"syntaxHighlight\": %s,\n", g_bSyntaxHighlight ? "true" : "false");
    fprintf(fp, "  \"vimMode\": %s,\n", IsVimModeEnabled() ? "true" : "false");
    fprintf(fp, "  \"windowX\": %d,\n", g_nWindowX);
    fprintf(fp, "  \"windowY\": %d,\n", g_nWindowY);
    fprintf(fp, "  \"windowWidth\": %d,\n", g_nWindowWidth);
    fprintf(fp, "  \"windowHeight\": %d,\n", g_nWindowHeight);
    fprintf(fp, "  \"windowMaximized\": %s,\n", g_bWindowMaximized ? "true" : "false");
    fprintf(fp, "  \"recentFiles\": [\n");
    for (i = 0; i < g_nRecentCount; i++) {
        char escaped[MAX_PATH * 2];
        EscapeJsonString(g_RecentFiles[i], escaped, sizeof(escaped));
        fprintf(fp, "    \"%s\"%s\n", escaped, (i < g_nRecentCount - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"sessionFiles\": [\n");
    for (i = 0; i < g_AppState.nTabCount; i++) {
        if (!g_AppState.tabs[i].bUntitled && g_AppState.tabs[i].szFileName[0]) {
            char escaped[MAX_PATH * 2];
            if (sessionCount > 0) fprintf(fp, ",\n");
            EscapeJsonString(g_AppState.tabs[i].szFileName, escaped, sizeof(escaped));
            fprintf(fp, "    \"%s\"", escaped);
            sessionCount++;
        }
    }
    if (sessionCount > 0) fprintf(fp, "\n");
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"sessionActiveTab\": %d\n", g_AppState.nCurrentTab);
    fprintf(fp, "}\n");
    fclose(fp);
}


void LoadWindowPosition(HWND hwnd) {
    RECT rcWork;
    if (g_bWindowMaximized) { ShowWindow(hwnd, SW_MAXIMIZE); }
    else if (g_nWindowX != CW_USEDEFAULT && g_nWindowY != CW_USEDEFAULT) {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
        if (g_nWindowX >= rcWork.left && g_nWindowX < rcWork.right - 100 &&
            g_nWindowY >= rcWork.top && g_nWindowY < rcWork.bottom - 100) {
            SetWindowPos(hwnd, NULL, g_nWindowX, g_nWindowY, g_nWindowWidth, g_nWindowHeight, SWP_NOZORDER);
        }
    }
}

void SaveWindowPosition(HWND hwnd) {
    WINDOWPLACEMENT wp = {0}; RECT rc;
    wp.length = sizeof(wp); GetWindowPlacement(hwnd, &wp);
    g_bWindowMaximized = (wp.showCmd == SW_MAXIMIZE);
    if (!g_bWindowMaximized) {
        GetWindowRect(hwnd, &rc);
        g_nWindowX = rc.left; g_nWindowY = rc.top;
        g_nWindowWidth = rc.right - rc.left; g_nWindowHeight = rc.bottom - rc.top;
    }
}

void AddRecentFile(const TCHAR* szFileName) {
    int i, j; TCHAR temp[MAX_PATH];
    if (!szFileName || !szFileName[0]) return;
    for (i = 0; i < g_nRecentCount; i++) {
        if (_tcsicmp(g_RecentFiles[i], szFileName) == 0) {
            if (i > 0) {
                _tcscpy(temp, g_RecentFiles[i]);
                for (j = i; j > 0; j--) { _tcscpy(g_RecentFiles[j], g_RecentFiles[j-1]); }
                _tcscpy(g_RecentFiles[0], temp);
            }
            return;
        }
    }
    if (g_nRecentCount < MAX_RECENT_FILES) { g_nRecentCount++; }
    for (i = g_nRecentCount - 1; i > 0; i--) { _tcscpy(g_RecentFiles[i], g_RecentFiles[i-1]); }
    _tcscpy(g_RecentFiles[0], szFileName);
}

int GetRecentFileCount(void) { return g_nRecentCount; }

const TCHAR* GetRecentFile(int index) {
    if (index >= 0 && index < g_nRecentCount) { return g_RecentFiles[index]; }
    return NULL;
}


void RestoreSession(HWND hwnd) {
    BOOL bFirstFile = TRUE; int nRestoredCount = 0, i;
    if (g_nSessionCount == 0) return;
    for (i = 0; i < g_nSessionCount; i++) {
        TabState* pTab; TCHAR* pFileName; int nTab;
        if (GetFileAttributes(g_SessionFiles[i]) == INVALID_FILE_ATTRIBUTES) { continue; }
        if (bFirstFile && g_AppState.nTabCount == 1 && g_AppState.tabs[0].bUntitled && !g_AppState.tabs[0].bModified) {
            pTab = &g_AppState.tabs[0];
            _tcscpy(pTab->szFileName, g_SessionFiles[i]);
            pTab->bUntitled = FALSE;
            if (ReadFileContent(pTab->hwndEdit, pTab->szFileName)) {
                pTab->bModified = FALSE;
                pTab->language = DetectLanguage(pTab->szFileName);
                if (g_bSyntaxHighlight) { ApplySyntaxHighlighting(pTab->hwndEdit, pTab->language); }
                UpdateTabTitle(0); UpdateWindowTitle(hwnd);
                AddRecentFile(pTab->szFileName); nRestoredCount++;
            }
            bFirstFile = FALSE;
        } else {
            pFileName = _tcsrchr(g_SessionFiles[i], TEXT('\\'));
            if (pFileName) pFileName++; else pFileName = g_SessionFiles[i];
            nTab = AddNewTab(hwnd, pFileName);
            if (nTab >= 0) {
                pTab = &g_AppState.tabs[nTab];
                _tcscpy(pTab->szFileName, g_SessionFiles[i]);
                pTab->bUntitled = FALSE;
                if (ReadFileContent(pTab->hwndEdit, pTab->szFileName)) {
                    pTab->bModified = FALSE;
                    pTab->language = DetectLanguage(pTab->szFileName);
                    if (g_bSyntaxHighlight) { ApplySyntaxHighlighting(pTab->hwndEdit, pTab->language); }
                    UpdateTabTitle(nTab);
                    AddRecentFile(pTab->szFileName); nRestoredCount++;
                }
            }
        }
    }
    if (nRestoredCount > 0 && g_nSessionActiveTab >= 0 && g_nSessionActiveTab < g_AppState.nTabCount) {
        SwitchToTab(hwnd, g_nSessionActiveTab);
    }
}

void ClearSession(void) { g_nSessionCount = 0; g_nSessionActiveTab = 0; }
