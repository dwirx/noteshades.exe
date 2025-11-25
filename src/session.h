/**
 * session.h - Session persistence for XNote
 * Menyimpan dan memuat state aplikasi dalam format JSON
 */

#ifndef SESSION_H
#define SESSION_H

#include <windows.h>
#include "notepad.h"

/* Session file name */
#define SESSION_FILE_NAME TEXT("xnote_session.json")

/* Session data structure for a single tab */
typedef struct {
    TCHAR szFilePath[MAX_PATH];  /* Full path to file */
    int nCursorPos;              /* Cursor position */
    int nScrollPos;              /* Scroll position (first visible line) */
    int nSelStart;               /* Selection start */
    int nSelEnd;                 /* Selection end */
    BOOL bModified;              /* Has unsaved changes */
    LanguageType language;       /* Syntax highlighting language */
} SessionTab;

/* Session data structure */
typedef struct {
    int nVersion;                /* Session format version */
    int nTabCount;               /* Number of tabs */
    int nActiveTab;              /* Currently active tab index */
    BOOL bWordWrap;              /* Word wrap enabled */
    BOOL bShowLineNumbers;       /* Line numbers enabled */
    BOOL bRelativeLineNumbers;   /* Relative line numbers enabled */
    BOOL bVimMode;               /* Vim mode enabled */
    BOOL bSyntaxHighlight;       /* Syntax highlighting enabled */
    int nWindowX;                /* Window X position */
    int nWindowY;                /* Window Y position */
    int nWindowWidth;            /* Window width */
    int nWindowHeight;           /* Window height */
    BOOL bMaximized;             /* Window maximized state */
    SessionTab tabs[MAX_TABS];   /* Tab data array */
} SessionData;

/* Session functions */
BOOL GetSessionFilePath(TCHAR* szPath, DWORD dwSize);
BOOL SaveSession(HWND hwnd);
BOOL LoadSession(HWND hwnd);
BOOL SaveSessionOnExit(HWND hwnd);
void SyncNewFileToSession(int nTabIndex);
void MarkSessionDirty(void);

/* Auto-save timer */
#define TIMER_AUTOSAVE 100
#define AUTOSAVE_INTERVAL 5000  /* 5 seconds */

/* Initialize session system */
void InitSessionSystem(HWND hwnd);
void CleanupSessionSystem(void);
void HandleSessionTimer(HWND hwnd);

#endif /* SESSION_H */
