#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource.h"

/* Forward declare LanguageType from syntax.h */
typedef enum {
    LANG_NONE = 0,
    LANG_C,
    LANG_CPP,
    LANG_JAVA,
    LANG_JAVASCRIPT,
    LANG_TYPESCRIPT,
    LANG_PYTHON,
    LANG_GO,
    LANG_RUST,
    LANG_HTML,
    LANG_CSS,
    LANG_JSON,
    LANG_XML,
    LANG_YAML,
    LANG_SQL,
    LANG_PHP,
    LANG_RUBY,
    LANG_SHELL,
    LANG_BATCH,
    LANG_POWERSHELL,
    LANG_MARKDOWN,
    LANG_COUNT
} LanguageType;

/* Application name */
#define APP_NAME TEXT("XNote")
#define APP_VERSION TEXT("1.0")

/* Maximum number of tabs */
#define MAX_TABS 32

/* Line number state structure */
typedef struct {
    BOOL bShowLineNumbers;       /* Flag to show/hide line numbers */
    HWND hwndLineNumbers;        /* Handle for line number window */
    int nLineNumberWidth;        /* Width of line number panel (in pixels) */
} LineNumberState;

/* Line ending types */
typedef enum {
    LINE_ENDING_CRLF = 0,        /* Windows (CR LF) */
    LINE_ENDING_LF,              /* Unix (LF) */
    LINE_ENDING_CR               /* Mac (CR) */
} LineEndingType;

/* Large file loading modes */
typedef enum {
    FILEMODE_NORMAL = 0,         /* Normal mode - full file loaded (<50MB) */
    FILEMODE_PARTIAL,            /* Partial mode - chunked loading (50-200MB) */
    FILEMODE_READONLY,           /* Read-only preview mode (200MB-1GB) */
    FILEMODE_MMAP                /* Memory-mapped mode (>1GB) */
} FileModeType;

/* Large file threshold constants - OPTIMIZED for better responsiveness */
#define THRESHOLD_PROGRESS      (1 * 1024 * 1024)       /* 1MB - show progress dialog */
#define THRESHOLD_PARTIAL       (2 * 1024 * 1024)       /* 2MB - switch to partial mode */
#define THRESHOLD_READONLY      (10 * 1024 * 1024)      /* 10MB - switch to read-only */
#define THRESHOLD_MMAP          (50 * 1024 * 1024)      /* 50MB - switch to memory-mapped */
#define THRESHOLD_SYNTAX_OFF    (256 * 1024)            /* 256KB - disable syntax highlighting */
#define THRESHOLD_LINE_SYNTAX   5000                     /* 5000 lines - disable syntax */

/* Chunk size constants for large file loading - OPTIMIZED based on benchmark */
#define INITIAL_CHUNK_SIZE      (512 * 1024)            /* 512KB initial load for instant display */
#define LOAD_MORE_CHUNK         (1 * 1024 * 1024)       /* 1MB per F5 press */
#define PREVIEW_SIZE            (512 * 1024)            /* 512KB preview for read-only */
#define THREAD_CHUNK_SIZE       (64 * 1024)             /* 64KB read chunks - optimal from benchmark */
#define STREAM_CHUNK_SIZE       (32 * 1024)             /* 32KB for streaming to RichEdit */
#define SMALL_FILE_THRESHOLD    (512 * 1024)            /* 512KB - read all at once */

/* Undo buffer limits */
#define UNDO_LIMIT_LARGE_FILE   100                     /* Max undo operations for large files */

/* Tab/Document state structure */
typedef struct {
    TCHAR szFileName[MAX_PATH];  /* Full path of current file */
    BOOL bModified;              /* Unsaved changes flag */
    BOOL bUntitled;              /* New document without name flag */
    HWND hwndEdit;               /* Edit control for this tab */
    WCHAR* pContent;             /* Content buffer for large files */
    DWORD dwContentSize;         /* Size of content */
    LineNumberState lineNumState; /* Line number state for this tab */
    LineEndingType lineEnding;   /* Line ending type */
    BOOL bInsertMode;            /* Insert/Overwrite mode */
    LanguageType language;       /* Language for syntax highlighting */
    BOOL bNeedsSyntaxRefresh;    /* Flag for lazy syntax refresh after theme change */

    /* Large file support */
    FileModeType fileMode;       /* File loading mode */
    HANDLE hFileMapping;         /* Memory-mapped file handle */
    LPVOID pMappedView;          /* Mapped view pointer */
    DWORD dwTotalFileSize;       /* Total file size on disk */
    DWORD dwLoadedSize;          /* Amount currently loaded in editor */
    DWORD dwChunkSize;           /* Chunk size for partial loading */
} TabState;

/* Application state structure */
typedef struct {
    HINSTANCE hInstance;         /* Application instance handle */
    HWND hwndMain;               /* Main window handle */
    HWND hwndTab;                /* Tab control handle */
    HWND hwndStatus;             /* Status bar handle */
    HACCEL hAccel;               /* Accelerator table handle */
    BOOL bWordWrap;              /* Word wrap enabled flag */
    BOOL bShowLineNumbers;       /* Global line numbers enabled flag */
    BOOL bRelativeLineNumbers;   /* Relative line numbers enabled flag */
    int nTabCount;               /* Number of open tabs */
    int nCurrentTab;             /* Currently active tab index */
    TabState tabs[MAX_TABS];     /* Array of tab states */
} AppState;

/* Global application state */
extern AppState g_AppState;

/* Main window functions */
ATOM RegisterMainWindowClass(HINSTANCE hInstance);
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* File operations */
BOOL FileNew(HWND hwnd);
BOOL FileOpen(HWND hwnd);
BOOL FileSave(HWND hwnd);
BOOL FileSaveAs(HWND hwnd);
BOOL PromptSaveChanges(HWND hwnd);
void UpdateWindowTitle(HWND hwnd);

/* Large file operations */
FileModeType DetectOptimalFileMode(DWORD dwFileSize);
BOOL LoadFileWithMode(HWND hwnd, const TCHAR* szFileName, FileModeType forcedMode);
BOOL LoadMoreContent(HWND hwnd);
void ShowLargeFileInfo(HWND hwnd, DWORD dwFileSize, FileModeType mode);

/* Edit operations */
void EditUndo(HWND hEdit);
void EditCut(HWND hEdit);
void EditCopy(HWND hEdit);
void EditPaste(HWND hEdit);
void EditSelectAll(HWND hEdit);

/* Extended edit operations */
void EditDuplicateLine(HWND hEdit);
void EditDeleteLine(HWND hEdit);
void EditMoveLineUp(HWND hEdit);
void EditMoveLineDown(HWND hEdit);
void EditToggleComment(HWND hEdit);
void EditIndent(HWND hEdit);
void EditUnindent(HWND hEdit);
void EditGoToLine(HWND hEdit, int nLine);

/* Zoom operations */
int GetZoomLevel(void);
void SetZoomLevel(HWND hwnd, int nLevel);
void ZoomIn(HWND hwnd);
void ZoomOut(HWND hwnd);
void ZoomReset(HWND hwnd);

/* Dialog operations */
BOOL ShowOpenDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile);
BOOL ShowSaveDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile);
int ShowConfirmSaveDialog(HWND hwnd);
void ShowAboutDialog(HWND hwnd);
void ShowErrorDialog(HWND hwnd, const TCHAR* szMessage);

/* Find/Replace operations */
void ShowFindDialog(HWND hwnd);
void ShowReplaceDialog(HWND hwnd);
void FindNext(HWND hwnd);
void ReplaceCurrent(HWND hwnd);
void ReplaceAll(HWND hwnd);
HWND GetFindReplaceDialog(void);
INT_PTR CALLBACK FindReplaceDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* Help dialog */
void ShowHelpDialog(HWND hwnd);
INT_PTR CALLBACK HelpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* Go to Line dialog */
void ShowGoToLineDialog(HWND hwnd);
INT_PTR CALLBACK GoToLineDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* Helper functions */
void InitTabState(TabState* pState);
BOOL ReadFileContent(HWND hEdit, const TCHAR* szFileName);
BOOL WriteFileContent(HWND hEdit, const TCHAR* szFileName);
BOOL ReadLargeFile(const TCHAR* szFileName, WCHAR** ppContent, DWORD* pdwSize);
BOOL WriteLargeFile(const TCHAR* szFileName, const WCHAR* pContent, DWORD dwSize);

/* Format operations */
void ToggleWordWrap(HWND hwnd);
void RecreateEditControl(HWND hwnd, int nTabIndex, BOOL bWordWrap);

/* Tab operations */
int AddNewTab(HWND hwnd, const TCHAR* szTitle);
void CloseTab(HWND hwnd, int nTabIndex);
void CloseAllTabs(HWND hwnd);
void CloseOtherTabs(HWND hwnd, int nKeepTab);
void SwitchToTab(HWND hwnd, int nTabIndex);
void UpdateTabTitle(int nTabIndex);
HWND GetCurrentEdit(void);
TabState* GetCurrentTabState(void);

/* Line number operations */
HWND CreateLineNumberWindow(HWND hwndParent, HINSTANCE hInstance);
void UpdateLineNumbers(HWND hwndLineNumbers, HWND hwndEdit);
void ToggleLineNumbers(HWND hwnd);
void ToggleRelativeLineNumbers(HWND hwnd);
void SyncLineNumberScroll(HWND hwndLineNumbers, HWND hwndEdit);
int CalculateLineNumberWidth(int nLineCount);
LRESULT CALLBACK LineNumberWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RepositionControls(HWND hwnd);

/* Status bar operations */
HWND CreateStatusBar(HWND hwndParent, HINSTANCE hInstance);
void UpdateStatusBar(HWND hwnd);
void SetStatusBarParts(HWND hwndStatus, int nWidth);
const TCHAR* GetFileTypeString(const TCHAR* szFileName);
int CountWords(HWND hwndEdit);

/* Vim mode operations */
#include "vim_mode.h"

/* Font operations */
void SetGlobalFont(HFONT hFont);
HFONT GetGlobalFont(void);

#endif /* NOTEPAD_H */
