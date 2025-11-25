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

/* Edit operations */
void EditUndo(HWND hEdit);
void EditCut(HWND hEdit);
void EditCopy(HWND hEdit);
void EditPaste(HWND hEdit);
void EditSelectAll(HWND hEdit);

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
void SwitchToTab(HWND hwnd, int nTabIndex);
void UpdateTabTitle(int nTabIndex);
HWND GetCurrentEdit(void);
TabState* GetCurrentTabState(void);

/* Line number operations */
HWND CreateLineNumberWindow(HWND hwndParent, HINSTANCE hInstance);
void UpdateLineNumbers(HWND hwndLineNumbers, HWND hwndEdit);
void ToggleLineNumbers(HWND hwnd);
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

#endif /* NOTEPAD_H */
