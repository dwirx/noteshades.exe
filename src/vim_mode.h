#ifndef VIM_MODE_H
#define VIM_MODE_H

#include <windows.h>
#include <tchar.h>

/* Vim mode states */
typedef enum {
    VIM_MODE_NORMAL = 0,    /* Normal mode - navigation */
    VIM_MODE_INSERT,        /* Insert mode - typing */
    VIM_MODE_VISUAL,        /* Visual mode - character selection */
    VIM_MODE_VISUAL_LINE,   /* Visual Line mode - line selection (V) */
    VIM_MODE_COMMAND,       /* Command mode - : commands */
    VIM_MODE_SEARCH         /* Search mode - / or ? */
} VimModeState;

/* Vim state structure */
typedef struct {
    BOOL bEnabled;              /* Vim mode enabled/disabled */
    VimModeState mode;          /* Current vim mode */
    int nRepeatCount;           /* Repeat count for commands (e.g., 5j) */
    TCHAR chPendingOp;          /* Pending operator (d, y, c) */
    DWORD dwVisualStart;        /* Visual mode selection start */
    int nVisualStartLine;       /* Visual line mode start line */
    int nVisualCurrentLine;     /* Visual line mode current line */
    TCHAR szCommandBuffer[256]; /* Command buffer for : commands */
    int nCommandLen;            /* Command buffer length */
    TCHAR szSearchPattern[256]; /* Last search pattern */
    BOOL bSearchForward;        /* Search direction */
    int nDesiredCol;            /* Desired column for vertical movement */
    HWND hwndMain;              /* Main window handle for commands */
    TCHAR szLastCommand[256];   /* Last executed command */
    TCHAR szRegister[4096];     /* Yank register */
    BOOL bRegisterIsLine;       /* Is register content a full line? */
} VimState;

/* Global vim state */
extern VimState g_VimState;

/* Initialize vim mode */
void InitVimMode(void);

/* Toggle vim mode on/off */
void ToggleVimMode(HWND hwnd);

/* Check if vim mode is enabled */
BOOL IsVimModeEnabled(void);

/* Get current vim mode state */
VimModeState GetVimModeState(void);

/* Get vim mode string for status bar */
const TCHAR* GetVimModeString(void);

/* Process key input in vim mode */
BOOL ProcessVimKey(HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam);

/* Vim navigation commands */
void VimMoveLeft(HWND hwndEdit, int count);
void VimMoveRight(HWND hwndEdit, int count);
void VimMoveUp(HWND hwndEdit, int count);
void VimMoveDown(HWND hwndEdit, int count);
void VimMoveWordForward(HWND hwndEdit, int count);
void VimMoveWordBackward(HWND hwndEdit, int count);
void VimMoveWordEnd(HWND hwndEdit, int count);
void VimMoveLineStart(HWND hwndEdit);
void VimMoveLineEnd(HWND hwndEdit);
void VimMoveFirstNonBlank(HWND hwndEdit);
void VimMoveFirstLine(HWND hwndEdit);
void VimMoveLastLine(HWND hwndEdit);
void VimMoveToLine(HWND hwndEdit, int line);
void VimPageDown(HWND hwndEdit, int count);
void VimPageUp(HWND hwndEdit, int count);
void VimHalfPageDown(HWND hwndEdit, int count);
void VimHalfPageUp(HWND hwndEdit, int count);

/* Vim edit commands */
void VimDeleteChar(HWND hwndEdit, int count);
void VimDeleteCharBefore(HWND hwndEdit, int count);
void VimDeleteLine(HWND hwndEdit, int count);
void VimDeleteToEnd(HWND hwndEdit);
void VimYankLine(HWND hwndEdit, int count);
void VimPaste(HWND hwndEdit);
void VimPasteBefore(HWND hwndEdit);
void VimUndo(HWND hwndEdit);
void VimRedo(HWND hwndEdit);
void VimJoinLines(HWND hwndEdit);
void VimIndentLine(HWND hwndEdit, int count);
void VimUnindentLine(HWND hwndEdit, int count);

/* Vim mode transitions */
void VimEnterInsertMode(HWND hwndEdit);
void VimEnterInsertModeAfter(HWND hwndEdit);
void VimEnterInsertModeLineStart(HWND hwndEdit);
void VimEnterInsertModeLineEnd(HWND hwndEdit);
void VimEnterInsertModeNewLineBelow(HWND hwndEdit);
void VimEnterInsertModeNewLineAbove(HWND hwndEdit);
void VimEnterVisualMode(HWND hwndEdit);
void VimEnterVisualLineMode(HWND hwndEdit);
void VimEnterNormalMode(HWND hwndEdit);

/* Search commands */
void VimSearchForward(HWND hwndEdit, const TCHAR* pattern);
void VimSearchBackward(HWND hwndEdit, const TCHAR* pattern);
void VimSearchNext(HWND hwndEdit);
void VimSearchPrev(HWND hwndEdit);
void VimSearchRealtime(HWND hwndEdit);

/* Command mode */
void VimEnterCommandMode(HWND hwndEdit);
void VimEnterSearchMode(HWND hwndEdit, BOOL bForward);
void VimExecuteCommand(HWND hwndMain, HWND hwndEdit);
const TCHAR* GetVimCommandBuffer(void);

/* Find character on line */
void VimFindCharForward(HWND hwndEdit, TCHAR ch, int count);
void VimFindCharBackward(HWND hwndEdit, TCHAR ch, int count);
void VimFindCharTillForward(HWND hwndEdit, TCHAR ch, int count);
void VimFindCharTillBackward(HWND hwndEdit, TCHAR ch, int count);

/* Additional motions */
void VimMoveMatchingBracket(HWND hwndEdit);
void VimMoveParagraphForward(HWND hwndEdit, int count);
void VimMoveParagraphBackward(HWND hwndEdit, int count);

#endif /* VIM_MODE_H */
