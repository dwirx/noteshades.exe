#ifndef MULTI_CURSOR_H
#define MULTI_CURSOR_H

#include <windows.h>
#include <tchar.h>

/* Maximum number of cursors supported */
#define MAX_CURSORS 100

/* Cursor position structure */
typedef struct {
    DWORD dwPosition;      /* Character position in document */
    DWORD dwSelStart;      /* Selection start (same as position if no selection) */
    DWORD dwSelEnd;        /* Selection end */
    BOOL bHasSelection;    /* Whether this cursor has a selection */
} CursorInfo;

/* Multi-cursor state structure */
typedef struct {
    CursorInfo cursors[MAX_CURSORS];  /* Array of cursor positions */
    int nCursorCount;                  /* Number of active cursors */
    int nPrimaryCursor;                /* Index of primary cursor */
    BOOL bColumnMode;                  /* Column selection mode active */
    int nColumnStartCol;               /* Column mode start column */
    int nColumnEndCol;                 /* Column mode end column */
    int nColumnStartLine;              /* Column mode start line */
    int nColumnEndLine;                /* Column mode end line */
    TCHAR szSearchText[256];           /* Text being searched for Ctrl+D */
    BOOL bActive;                      /* Multi-cursor mode is active */
} MultiCursorState;

/* Cursor pixel position for rendering */
typedef struct {
    int x;          /* X coordinate in edit control */
    int y;          /* Y coordinate in edit control */
    int height;     /* Cursor height (line height) */
} CursorPixelPos;

/* Initialize multi-cursor state */
void MultiCursor_Init(MultiCursorState* pState);

/* Add a cursor at position */
BOOL MultiCursor_Add(MultiCursorState* pState, DWORD dwPosition);

/* Add a cursor with selection */
BOOL MultiCursor_AddWithSelection(MultiCursorState* pState, DWORD dwSelStart, DWORD dwSelEnd);

/* Remove cursor at position */
BOOL MultiCursor_Remove(MultiCursorState* pState, DWORD dwPosition);

/* Remove cursor by index */
BOOL MultiCursor_RemoveByIndex(MultiCursorState* pState, int nIndex);

/* Clear all cursors except primary */
void MultiCursor_Clear(MultiCursorState* pState);

/* Get cursor count */
int MultiCursor_GetCount(MultiCursorState* pState);

/* Check if multi-cursor mode is active */
BOOL MultiCursor_IsActive(MultiCursorState* pState);

/* Find cursor at position */
int MultiCursor_FindAtPosition(MultiCursorState* pState, DWORD dwPosition);

/* Sort cursors by position (for proper text operations) */
void MultiCursor_Sort(MultiCursorState* pState);

/* Select next occurrence of current selection */
BOOL MultiCursor_SelectNextOccurrence(HWND hEdit, MultiCursorState* pState);

/* Handle typing at all cursor positions */
void MultiCursor_InsertText(HWND hEdit, MultiCursorState* pState, const TCHAR* szText);

/* Handle single character insertion */
void MultiCursor_InsertChar(HWND hEdit, MultiCursorState* pState, TCHAR ch);

/* Handle deletion at all cursor positions */
void MultiCursor_Delete(HWND hEdit, MultiCursorState* pState, BOOL bBackspace);

/* Move all cursors */
void MultiCursor_Move(HWND hEdit, MultiCursorState* pState, int nDirection);

/* Direction constants for MultiCursor_Move */
#define MC_MOVE_LEFT    0
#define MC_MOVE_RIGHT   1
#define MC_MOVE_UP      2
#define MC_MOVE_DOWN    3
#define MC_MOVE_HOME    4
#define MC_MOVE_END     5

/* Column selection functions */
void MultiCursor_StartColumnSelect(HWND hEdit, MultiCursorState* pState, int nLine, int nCol);
void MultiCursor_ExtendColumnSelect(HWND hEdit, MultiCursorState* pState, int nLine, int nCol);
void MultiCursor_ApplyColumnSelect(HWND hEdit, MultiCursorState* pState);

/* Update cursor positions after document change */
void MultiCursor_AdjustPositions(MultiCursorState* pState, DWORD dwChangePos, int nDelta);

/* Sync primary cursor with RichEdit selection */
void MultiCursor_SyncFromEdit(HWND hEdit, MultiCursorState* pState);

/* Get pixel position for cursor rendering */
CursorPixelPos MultiCursor_GetPixelPos(HWND hEdit, DWORD dwCharPos);

/* Process keyboard input for multi-cursor */
BOOL MultiCursor_ProcessKey(HWND hEdit, MultiCursorState* pState, UINT msg, WPARAM wParam, LPARAM lParam);

/* Process mouse input for multi-cursor (Alt+Click) */
BOOL MultiCursor_ProcessMouse(HWND hEdit, MultiCursorState* pState, UINT msg, WPARAM wParam, LPARAM lParam);

/* Cursor overlay rendering */
void MultiCursor_DrawOverlay(HWND hEdit, HDC hdc, MultiCursorState* pState);
void MultiCursor_ToggleBlink(void);
BOOL MultiCursor_IsCursorVisible(void);

#endif /* MULTI_CURSOR_H */
