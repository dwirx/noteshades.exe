#include "multi_cursor.h"
#include "notepad.h"
#include <richedit.h>
#include <windowsx.h>

/* Initialize multi-cursor state */
void MultiCursor_Init(MultiCursorState* pState) {
    if (!pState) return;
    
    ZeroMemory(pState, sizeof(MultiCursorState));
    pState->nCursorCount = 1;
    pState->nPrimaryCursor = 0;
    pState->cursors[0].dwPosition = 0;
    pState->cursors[0].dwSelStart = 0;
    pState->cursors[0].dwSelEnd = 0;
    pState->cursors[0].bHasSelection = FALSE;
    pState->bColumnMode = FALSE;
    pState->bActive = FALSE;
}

/* Add a cursor at position */
BOOL MultiCursor_Add(MultiCursorState* pState, DWORD dwPosition) {
    if (!pState) return FALSE;
    if (pState->nCursorCount >= MAX_CURSORS) return FALSE;
    
    /* Check if cursor already exists at this position */
    if (MultiCursor_FindAtPosition(pState, dwPosition) >= 0) {
        return FALSE;
    }
    
    /* Add new cursor */
    int nIndex = pState->nCursorCount;
    pState->cursors[nIndex].dwPosition = dwPosition;
    pState->cursors[nIndex].dwSelStart = dwPosition;
    pState->cursors[nIndex].dwSelEnd = dwPosition;
    pState->cursors[nIndex].bHasSelection = FALSE;
    pState->nCursorCount++;
    pState->bActive = (pState->nCursorCount > 1);
    
    /* Sort cursors by position */
    MultiCursor_Sort(pState);
    
    return TRUE;
}

/* Add a cursor with selection */
BOOL MultiCursor_AddWithSelection(MultiCursorState* pState, DWORD dwSelStart, DWORD dwSelEnd) {
    if (!pState) return FALSE;
    if (pState->nCursorCount >= MAX_CURSORS) return FALSE;
    
    /* Add new cursor with selection */
    int nIndex = pState->nCursorCount;
    pState->cursors[nIndex].dwPosition = dwSelEnd;
    pState->cursors[nIndex].dwSelStart = dwSelStart;
    pState->cursors[nIndex].dwSelEnd = dwSelEnd;
    pState->cursors[nIndex].bHasSelection = (dwSelStart != dwSelEnd);
    pState->nCursorCount++;
    pState->bActive = (pState->nCursorCount > 1);
    
    /* Sort cursors by position */
    MultiCursor_Sort(pState);
    
    return TRUE;
}


/* Remove cursor at position */
BOOL MultiCursor_Remove(MultiCursorState* pState, DWORD dwPosition) {
    if (!pState) return FALSE;
    
    int nIndex = MultiCursor_FindAtPosition(pState, dwPosition);
    if (nIndex < 0) return FALSE;
    
    return MultiCursor_RemoveByIndex(pState, nIndex);
}

/* Remove cursor by index */
BOOL MultiCursor_RemoveByIndex(MultiCursorState* pState, int nIndex) {
    if (!pState) return FALSE;
    if (nIndex < 0 || nIndex >= pState->nCursorCount) return FALSE;
    
    /* Don't remove if only one cursor left */
    if (pState->nCursorCount <= 1) return FALSE;
    
    /* Shift remaining cursors */
    for (int i = nIndex; i < pState->nCursorCount - 1; i++) {
        pState->cursors[i] = pState->cursors[i + 1];
    }
    pState->nCursorCount--;
    
    /* Adjust primary cursor index if needed */
    if (pState->nPrimaryCursor >= pState->nCursorCount) {
        pState->nPrimaryCursor = pState->nCursorCount - 1;
    } else if (pState->nPrimaryCursor > nIndex) {
        pState->nPrimaryCursor--;
    }
    
    pState->bActive = (pState->nCursorCount > 1);
    return TRUE;
}

/* Clear all cursors except primary */
void MultiCursor_Clear(MultiCursorState* pState) {
    if (!pState) return;
    
    /* Keep only the primary cursor - preserve its position */
    if (pState->nPrimaryCursor > 0 && pState->nPrimaryCursor < pState->nCursorCount) {
        pState->cursors[0] = pState->cursors[pState->nPrimaryCursor];
    }
    
    /* Reset to single cursor state */
    pState->nCursorCount = 1;
    pState->nPrimaryCursor = 0;
    pState->bColumnMode = FALSE;
    pState->bActive = FALSE;
    pState->szSearchText[0] = TEXT('\0');
    
    /* Clear selection on remaining cursor */
    pState->cursors[0].bHasSelection = FALSE;
    pState->cursors[0].dwSelStart = pState->cursors[0].dwPosition;
    pState->cursors[0].dwSelEnd = pState->cursors[0].dwPosition;
}

/* Get cursor count */
int MultiCursor_GetCount(MultiCursorState* pState) {
    if (!pState) return 0;
    return pState->nCursorCount;
}

/* Check if multi-cursor mode is active */
BOOL MultiCursor_IsActive(MultiCursorState* pState) {
    if (!pState) return FALSE;
    return pState->bActive;
}

/* Find cursor at position */
int MultiCursor_FindAtPosition(MultiCursorState* pState, DWORD dwPosition) {
    if (!pState) return -1;
    
    for (int i = 0; i < pState->nCursorCount; i++) {
        /* Check if position is within cursor's selection or at cursor position */
        if (pState->cursors[i].bHasSelection) {
            if (dwPosition >= pState->cursors[i].dwSelStart && 
                dwPosition <= pState->cursors[i].dwSelEnd) {
                return i;
            }
        } else {
            if (pState->cursors[i].dwPosition == dwPosition) {
                return i;
            }
        }
    }
    return -1;
}

/* Compare function for sorting cursors */
static int CompareCursors(const void* a, const void* b) {
    const CursorInfo* ca = (const CursorInfo*)a;
    const CursorInfo* cb = (const CursorInfo*)b;
    
    DWORD posA = ca->bHasSelection ? ca->dwSelStart : ca->dwPosition;
    DWORD posB = cb->bHasSelection ? cb->dwSelStart : cb->dwPosition;
    
    if (posA < posB) return -1;
    if (posA > posB) return 1;
    return 0;
}

/* Sort cursors by position */
void MultiCursor_Sort(MultiCursorState* pState) {
    if (!pState || pState->nCursorCount <= 1) return;
    
    /* Remember primary cursor position before sort */
    DWORD dwPrimaryPos = pState->cursors[pState->nPrimaryCursor].dwPosition;
    
    /* Sort cursors */
    qsort(pState->cursors, pState->nCursorCount, sizeof(CursorInfo), CompareCursors);
    
    /* Find new primary cursor index */
    for (int i = 0; i < pState->nCursorCount; i++) {
        if (pState->cursors[i].dwPosition == dwPrimaryPos) {
            pState->nPrimaryCursor = i;
            break;
        }
    }
}

/* Sync primary cursor with RichEdit selection */
void MultiCursor_SyncFromEdit(HWND hEdit, MultiCursorState* pState) {
    if (!hEdit || !pState) return;
    
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    if (pState->nCursorCount > 0) {
        pState->cursors[pState->nPrimaryCursor].dwPosition = dwEnd;
        pState->cursors[pState->nPrimaryCursor].dwSelStart = dwStart;
        pState->cursors[pState->nPrimaryCursor].dwSelEnd = dwEnd;
        pState->cursors[pState->nPrimaryCursor].bHasSelection = (dwStart != dwEnd);
    }
}


/* Get pixel position for cursor rendering */
CursorPixelPos MultiCursor_GetPixelPos(HWND hEdit, DWORD dwCharPos) {
    CursorPixelPos pos = {0, 0, 16};
    if (!hEdit) return pos;
    
    /* Get character position in pixels */
    POINTL pt;
    SendMessage(hEdit, EM_POSFROMCHAR, (WPARAM)&pt, dwCharPos);
    pos.x = pt.x;
    pos.y = pt.y;
    
    /* Get line height from font metrics */
    HDC hdc = GetDC(hEdit);
    if (hdc) {
        HFONT hFont = (HFONT)SendMessage(hEdit, WM_GETFONT, 0, 0);
        if (hFont) {
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            TEXTMETRIC tm;
            GetTextMetrics(hdc, &tm);
            pos.height = tm.tmHeight;
            SelectObject(hdc, hOldFont);
        }
        ReleaseDC(hEdit, hdc);
    }
    
    return pos;
}

/* Update cursor positions after document change */
void MultiCursor_AdjustPositions(MultiCursorState* pState, DWORD dwChangePos, int nDelta) {
    if (!pState) return;
    
    for (int i = 0; i < pState->nCursorCount; i++) {
        /* Adjust position */
        if (pState->cursors[i].dwPosition > dwChangePos) {
            if (nDelta < 0 && pState->cursors[i].dwPosition < dwChangePos - nDelta) {
                pState->cursors[i].dwPosition = dwChangePos;
            } else {
                pState->cursors[i].dwPosition += nDelta;
            }
        }
        
        /* Adjust selection start */
        if (pState->cursors[i].dwSelStart > dwChangePos) {
            if (nDelta < 0 && pState->cursors[i].dwSelStart < dwChangePos - nDelta) {
                pState->cursors[i].dwSelStart = dwChangePos;
            } else {
                pState->cursors[i].dwSelStart += nDelta;
            }
        }
        
        /* Adjust selection end */
        if (pState->cursors[i].dwSelEnd > dwChangePos) {
            if (nDelta < 0 && pState->cursors[i].dwSelEnd < dwChangePos - nDelta) {
                pState->cursors[i].dwSelEnd = dwChangePos;
            } else {
                pState->cursors[i].dwSelEnd += nDelta;
            }
        }
    }
}

/* Find next occurrence of text in document */
static DWORD FindNextOccurrence(HWND hEdit, const TCHAR* szText, DWORD dwStartPos, BOOL bWrap) {
    if (!hEdit || !szText || szText[0] == TEXT('\0')) return (DWORD)-1;
    
    int nTextLen = (int)_tcslen(szText);
    int nDocLen = GetWindowTextLength(hEdit);
    
    if (nDocLen == 0 || nTextLen > nDocLen) return (DWORD)-1;
    
    /* Get document text */
    TCHAR* szDoc = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nDocLen + 1) * sizeof(TCHAR));
    if (!szDoc) return (DWORD)-1;
    
    GetWindowText(hEdit, szDoc, nDocLen + 1);
    
    /* Search from start position */
    DWORD dwFound = (DWORD)-1;
    for (DWORD i = dwStartPos; i <= (DWORD)(nDocLen - nTextLen); i++) {
        if (_tcsnicmp(szDoc + i, szText, nTextLen) == 0) {
            dwFound = i;
            break;
        }
    }
    
    /* Wrap around if not found and wrap is enabled */
    if (dwFound == (DWORD)-1 && bWrap && dwStartPos > 0) {
        for (DWORD i = 0; i < dwStartPos && i <= (DWORD)(nDocLen - nTextLen); i++) {
            if (_tcsnicmp(szDoc + i, szText, nTextLen) == 0) {
                dwFound = i;
                break;
            }
        }
    }
    
    HeapFree(GetProcessHeap(), 0, szDoc);
    return dwFound;
}

/* Check if position is already selected */
static BOOL IsPositionSelected(MultiCursorState* pState, DWORD dwStart, DWORD dwEnd) {
    if (!pState) return FALSE;
    
    for (int i = 0; i < pState->nCursorCount; i++) {
        if (pState->cursors[i].bHasSelection) {
            if (pState->cursors[i].dwSelStart == dwStart && 
                pState->cursors[i].dwSelEnd == dwEnd) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* Select next occurrence of current selection */
BOOL MultiCursor_SelectNextOccurrence(HWND hEdit, MultiCursorState* pState) {
    if (!hEdit || !pState) return FALSE;
    
    /* Get current selection */
    DWORD dwStart, dwEnd;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    
    /* If no selection, select word at cursor */
    if (dwStart == dwEnd) {
        /* Select word at cursor position */
        int nDocLen = GetWindowTextLength(hEdit);
        if (nDocLen == 0) return FALSE;
        
        TCHAR* szDoc = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nDocLen + 1) * sizeof(TCHAR));
        if (!szDoc) return FALSE;
        GetWindowText(hEdit, szDoc, nDocLen + 1);
        
        /* Find word boundaries */
        DWORD dwWordStart = dwStart;
        DWORD dwWordEnd = dwStart;
        
        /* Move back to start of word */
        while (dwWordStart > 0 && (IsCharAlphaNumeric(szDoc[dwWordStart - 1]) || szDoc[dwWordStart - 1] == TEXT('_'))) {
            dwWordStart--;
        }
        
        /* Move forward to end of word */
        while (dwWordEnd < (DWORD)nDocLen && (IsCharAlphaNumeric(szDoc[dwWordEnd]) || szDoc[dwWordEnd] == TEXT('_'))) {
            dwWordEnd++;
        }
        
        HeapFree(GetProcessHeap(), 0, szDoc);
        
        /* If no word found, return */
        if (dwWordStart == dwWordEnd) return FALSE;
        
        /* Select the word */
        SendMessage(hEdit, EM_SETSEL, dwWordStart, dwWordEnd);
        dwStart = dwWordStart;
        dwEnd = dwWordEnd;
    }
    
    /* Get selected text */
    int nSelLen = dwEnd - dwStart;
    if (nSelLen <= 0 || nSelLen >= 256) return FALSE;
    
    TCHAR* szSelText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nSelLen + 1) * sizeof(TCHAR));
    if (!szSelText) return FALSE;
    
    /* Get document text and extract selection */
    int nDocLen = GetWindowTextLength(hEdit);
    TCHAR* szDoc = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (nDocLen + 1) * sizeof(TCHAR));
    if (!szDoc) {
        HeapFree(GetProcessHeap(), 0, szSelText);
        return FALSE;
    }
    
    GetWindowText(hEdit, szDoc, nDocLen + 1);
    _tcsncpy(szSelText, szDoc + dwStart, nSelLen);
    szSelText[nSelLen] = TEXT('\0');
    
    /* Store search text */
    _tcsncpy(pState->szSearchText, szSelText, 255);
    pState->szSearchText[255] = TEXT('\0');
    
    /* If this is first Ctrl+D, initialize first cursor with current selection */
    if (!pState->bActive && pState->nCursorCount <= 1) {
        pState->cursors[0].dwSelStart = dwStart;
        pState->cursors[0].dwSelEnd = dwEnd;
        pState->cursors[0].dwPosition = dwEnd;
        pState->cursors[0].bHasSelection = TRUE;
        pState->nCursorCount = 1;
        pState->nPrimaryCursor = 0;
    }
    
    /* Find next occurrence after last cursor */
    DWORD dwSearchStart = dwEnd;
    if (pState->nCursorCount > 0) {
        /* Find the last cursor position */
        DWORD dwLastPos = 0;
        for (int i = 0; i < pState->nCursorCount; i++) {
            if (pState->cursors[i].dwSelEnd > dwLastPos) {
                dwLastPos = pState->cursors[i].dwSelEnd;
            }
        }
        dwSearchStart = dwLastPos;
    }
    
    DWORD dwFound = FindNextOccurrence(hEdit, szSelText, dwSearchStart, TRUE);
    
    HeapFree(GetProcessHeap(), 0, szDoc);
    HeapFree(GetProcessHeap(), 0, szSelText);
    
    if (dwFound == (DWORD)-1) return FALSE;
    
    /* Check if this occurrence is already selected */
    DWORD dwFoundEnd = dwFound + nSelLen;
    if (IsPositionSelected(pState, dwFound, dwFoundEnd)) {
        return FALSE; /* All occurrences already selected */
    }
    
    /* Add new selection - this will set bActive = TRUE */
    return MultiCursor_AddWithSelection(pState, dwFound, dwFoundEnd);
}


/* Handle typing at all cursor positions */
void MultiCursor_InsertText(HWND hEdit, MultiCursorState* pState, const TCHAR* szText) {
    if (!hEdit || !pState || !szText) return;
    
    /* If only one cursor, let RichEdit handle it normally */
    if (pState->nCursorCount <= 1) {
        /* Just insert at current position */
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText);
        return;
    }
    
    int nTextLen = (int)_tcslen(szText);
    if (nTextLen == 0) return;
    
    /* Sort cursors by position (ascending) */
    MultiCursor_Sort(pState);
    
    /* Disable redraw for performance */
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    
    /* Track cumulative offset as we process from first to last */
    int nCumulativeOffset = 0;
    
    /* Process cursors from FIRST to LAST (ascending order) */
    /* This way we can track cumulative offset properly */
    for (int i = 0; i < pState->nCursorCount; i++) {
        CursorInfo* pCursor = &pState->cursors[i];
        
        /* Calculate adjusted positions based on cumulative offset */
        DWORD dwAdjustedStart = pCursor->bHasSelection ? 
            (pCursor->dwSelStart + nCumulativeOffset) : 
            (pCursor->dwPosition + nCumulativeOffset);
        DWORD dwAdjustedEnd = pCursor->bHasSelection ? 
            (pCursor->dwSelEnd + nCumulativeOffset) : 
            (pCursor->dwPosition + nCumulativeOffset);
        
        DWORD dwOldLen = pCursor->bHasSelection ? (pCursor->dwSelEnd - pCursor->dwSelStart) : 0;
        
        /* Set selection/position with adjusted values */
        SendMessage(hEdit, EM_SETSEL, dwAdjustedStart, dwAdjustedEnd);
        
        /* Replace selection with text */
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText);
        
        /* Update cursor position (store the actual new position) */
        DWORD dwNewPos = dwAdjustedStart + nTextLen;
        
        pCursor->dwPosition = dwNewPos;
        pCursor->dwSelStart = dwNewPos;
        pCursor->dwSelEnd = dwNewPos;
        pCursor->bHasSelection = FALSE;
        
        /* Update cumulative offset for next cursors */
        nCumulativeOffset += (nTextLen - (int)dwOldLen);
    }
    
    /* Re-enable redraw */
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, NULL, TRUE);
    
    /* Set selection to primary cursor */
    if (pState->nPrimaryCursor >= 0 && pState->nPrimaryCursor < pState->nCursorCount) {
        CursorInfo* pPrimary = &pState->cursors[pState->nPrimaryCursor];
        SendMessage(hEdit, EM_SETSEL, pPrimary->dwPosition, pPrimary->dwPosition);
    }
}

/* Handle single character insertion */
void MultiCursor_InsertChar(HWND hEdit, MultiCursorState* pState, TCHAR ch) {
    TCHAR szText[2] = {ch, TEXT('\0')};
    MultiCursor_InsertText(hEdit, pState, szText);
}

/* Handle deletion at all cursor positions */
void MultiCursor_Delete(HWND hEdit, MultiCursorState* pState, BOOL bBackspace) {
    if (!hEdit || !pState) return;
    
    /* If only one cursor, let RichEdit handle it normally */
    if (pState->nCursorCount <= 1) {
        /* Send the appropriate key message */
        SendMessage(hEdit, WM_KEYDOWN, bBackspace ? VK_BACK : VK_DELETE, 0);
        return;
    }
    
    /* Sort cursors by position (ascending) */
    MultiCursor_Sort(pState);
    
    /* Disable redraw */
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    
    /* Track cumulative offset as we process from first to last */
    int nCumulativeOffset = 0;
    
    /* Process cursors from FIRST to LAST (ascending order) */
    for (int i = 0; i < pState->nCursorCount; i++) {
        CursorInfo* pCursor = &pState->cursors[i];
        
        if (pCursor->bHasSelection) {
            /* Calculate adjusted positions */
            DWORD dwAdjustedStart = pCursor->dwSelStart + nCumulativeOffset;
            DWORD dwAdjustedEnd = pCursor->dwSelEnd + nCumulativeOffset;
            int nDeleted = pCursor->dwSelEnd - pCursor->dwSelStart;
            
            /* Delete selection */
            SendMessage(hEdit, EM_SETSEL, dwAdjustedStart, dwAdjustedEnd);
            SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            
            /* Update cursor position */
            pCursor->dwPosition = dwAdjustedStart;
            pCursor->dwSelStart = dwAdjustedStart;
            pCursor->dwSelEnd = dwAdjustedStart;
            pCursor->bHasSelection = FALSE;
            
            /* Update cumulative offset */
            nCumulativeOffset -= nDeleted;
        } else {
            /* Delete single character */
            DWORD dwAdjustedPos = pCursor->dwPosition + nCumulativeOffset;
            DWORD dwDelStart, dwDelEnd;
            
            if (bBackspace) {
                if (dwAdjustedPos == 0) continue;
                dwDelStart = dwAdjustedPos - 1;
                dwDelEnd = dwAdjustedPos;
            } else {
                dwDelStart = dwAdjustedPos;
                dwDelEnd = dwAdjustedPos + 1;
            }
            
            SendMessage(hEdit, EM_SETSEL, dwDelStart, dwDelEnd);
            SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
            
            /* Update cursor position */
            if (bBackspace) {
                pCursor->dwPosition = dwDelStart;
                pCursor->dwSelStart = dwDelStart;
                pCursor->dwSelEnd = dwDelStart;
            } else {
                pCursor->dwPosition = dwDelStart;
                pCursor->dwSelStart = dwDelStart;
                pCursor->dwSelEnd = dwDelStart;
            }
            
            /* Update cumulative offset */
            nCumulativeOffset -= 1;
        }
    }
    
    /* Re-enable redraw */
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, NULL, TRUE);
    
    /* Set selection to primary cursor */
    if (pState->nPrimaryCursor >= 0 && pState->nPrimaryCursor < pState->nCursorCount) {
        CursorInfo* pPrimary = &pState->cursors[pState->nPrimaryCursor];
        SendMessage(hEdit, EM_SETSEL, pPrimary->dwPosition, pPrimary->dwPosition);
    }
}

/* Move all cursors */
void MultiCursor_Move(HWND hEdit, MultiCursorState* pState, int nDirection) {
    if (!hEdit || !pState) return;
    
    /* If only one cursor, don't intercept - let RichEdit handle normally */
    if (pState->nCursorCount <= 1) return;
    
    int nDocLen = GetWindowTextLength(hEdit);
    
    for (int i = 0; i < pState->nCursorCount; i++) {
        CursorInfo* pCursor = &pState->cursors[i];
        DWORD dwNewPos = pCursor->dwPosition;
        
        switch (nDirection) {
            case MC_MOVE_LEFT:
                if (dwNewPos > 0) dwNewPos--;
                break;
            case MC_MOVE_RIGHT:
                if (dwNewPos < (DWORD)nDocLen) dwNewPos++;
                break;
            case MC_MOVE_UP:
            case MC_MOVE_DOWN: {
                /* Get current line and column */
                int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwNewPos, 0);
                DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
                int nCol = dwNewPos - dwLineStart;
                
                if (nDirection == MC_MOVE_UP && nLine > 0) {
                    nLine--;
                } else if (nDirection == MC_MOVE_DOWN) {
                    int nTotalLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
                    if (nLine < nTotalLines - 1) nLine++;
                }
                
                dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
                int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
                if (nCol > nLineLen) nCol = nLineLen;
                dwNewPos = dwLineStart + nCol;
                break;
            }
            case MC_MOVE_HOME: {
                int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwNewPos, 0);
                dwNewPos = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
                break;
            }
            case MC_MOVE_END: {
                int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwNewPos, 0);
                DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
                int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
                dwNewPos = dwLineStart + nLineLen;
                break;
            }
        }
        
        pCursor->dwPosition = dwNewPos;
        pCursor->dwSelStart = dwNewPos;
        pCursor->dwSelEnd = dwNewPos;
        pCursor->bHasSelection = FALSE;
    }
    
    /* Set selection to primary cursor */
    if (pState->nPrimaryCursor >= 0 && pState->nPrimaryCursor < pState->nCursorCount) {
        CursorInfo* pPrimary = &pState->cursors[pState->nPrimaryCursor];
        SendMessage(hEdit, EM_SETSEL, pPrimary->dwPosition, pPrimary->dwPosition);
    }
}


/* Column selection functions */
void MultiCursor_StartColumnSelect(HWND hEdit, MultiCursorState* pState, int nLine, int nCol) {
    if (!hEdit || !pState) return;
    
    pState->bColumnMode = TRUE;
    pState->nColumnStartLine = nLine;
    pState->nColumnEndLine = nLine;
    pState->nColumnStartCol = nCol;
    pState->nColumnEndCol = nCol;
}

void MultiCursor_ExtendColumnSelect(HWND hEdit, MultiCursorState* pState, int nLine, int nCol) {
    if (!hEdit || !pState || !pState->bColumnMode) return;
    
    pState->nColumnEndLine = nLine;
    pState->nColumnEndCol = nCol;
    
    /* Apply column selection */
    MultiCursor_ApplyColumnSelect(hEdit, pState);
}

void MultiCursor_ApplyColumnSelect(HWND hEdit, MultiCursorState* pState) {
    if (!hEdit || !pState || !pState->bColumnMode) return;
    
    /* Clear existing cursors */
    pState->nCursorCount = 0;
    
    /* Determine line range */
    int nStartLine = min(pState->nColumnStartLine, pState->nColumnEndLine);
    int nEndLine = max(pState->nColumnStartLine, pState->nColumnEndLine);
    int nStartCol = min(pState->nColumnStartCol, pState->nColumnEndCol);
    int nEndCol = max(pState->nColumnStartCol, pState->nColumnEndCol);
    
    /* Create cursor for each line in column selection */
    for (int nLine = nStartLine; nLine <= nEndLine && pState->nCursorCount < MAX_CURSORS; nLine++) {
        DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
        int nLineLen = (int)SendMessage(hEdit, EM_LINELENGTH, dwLineStart, 0);
        
        /* Calculate selection bounds for this line */
        int nSelStart = min(nStartCol, nLineLen);
        int nSelEnd = min(nEndCol, nLineLen);
        
        CursorInfo* pCursor = &pState->cursors[pState->nCursorCount];
        pCursor->dwSelStart = dwLineStart + nSelStart;
        pCursor->dwSelEnd = dwLineStart + nSelEnd;
        pCursor->dwPosition = pCursor->dwSelEnd;
        pCursor->bHasSelection = (nSelStart != nSelEnd);
        
        pState->nCursorCount++;
    }
    
    pState->nPrimaryCursor = 0;
    pState->bActive = (pState->nCursorCount > 1);
}

/* Process keyboard input for multi-cursor */
BOOL MultiCursor_ProcessKey(HWND hEdit, MultiCursorState* pState, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam; /* Unused parameter */
    
    if (!hEdit || !pState) return FALSE;
    
    /* Handle Ctrl+D for select next occurrence - always available */
    if (msg == WM_KEYDOWN && wParam == 'D' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        if (MultiCursor_SelectNextOccurrence(hEdit, pState)) {
            InvalidateRect(hEdit, NULL, TRUE);
            return TRUE;
        }
        return FALSE;
    }
    
    /* Handle Escape to clear multi-cursor */
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE && pState->bActive) {
        MultiCursor_Clear(pState);
        InvalidateRect(hEdit, NULL, TRUE);
        return TRUE;
    }
    
    /* If multi-cursor not active, don't intercept any other keys */
    if (!pState->bActive) return FALSE;
    
    /* Handle arrow keys - only when multi-cursor is active */
    if (msg == WM_KEYDOWN) {
        int nDirection = -1;
        switch (wParam) {
            case VK_LEFT:  nDirection = MC_MOVE_LEFT; break;
            case VK_RIGHT: nDirection = MC_MOVE_RIGHT; break;
            case VK_UP:    nDirection = MC_MOVE_UP; break;
            case VK_DOWN:  nDirection = MC_MOVE_DOWN; break;
            case VK_HOME:  nDirection = MC_MOVE_HOME; break;
            case VK_END:   nDirection = MC_MOVE_END; break;
        }
        
        if (nDirection >= 0) {
            MultiCursor_Move(hEdit, pState, nDirection);
            InvalidateRect(hEdit, NULL, TRUE);
            return TRUE;
        }
        
        /* Handle backspace */
        if (wParam == VK_BACK) {
            MultiCursor_Delete(hEdit, pState, TRUE);
            return TRUE;
        }
        
        /* Handle delete */
        if (wParam == VK_DELETE) {
            MultiCursor_Delete(hEdit, pState, FALSE);
            return TRUE;
        }
    }
    
    /* Handle character input - only when multi-cursor is active */
    if (msg == WM_CHAR && wParam >= 32) {
        MultiCursor_InsertChar(hEdit, pState, (TCHAR)wParam);
        return TRUE;
    }
    
    /* Handle Enter key */
    if (msg == WM_CHAR && wParam == '\r') {
        MultiCursor_InsertText(hEdit, pState, TEXT("\r\n"));
        return TRUE;
    }
    
    /* Handle Tab key */
    if (msg == WM_CHAR && wParam == '\t') {
        MultiCursor_InsertChar(hEdit, pState, TEXT('\t'));
        return TRUE;
    }
    
    return FALSE;
}

/* Process mouse input for multi-cursor (Alt+Click) */
BOOL MultiCursor_ProcessMouse(HWND hEdit, MultiCursorState* pState, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)wParam; /* Unused parameter */
    
    if (!hEdit || !pState) return FALSE;
    
    /* Handle normal click - clear multi-cursor if active */
    if (msg == WM_LBUTTONDOWN && !(GetKeyState(VK_MENU) & 0x8000)) {
        if (pState->bActive) {
            /* Clear multi-cursor and let RichEdit handle the click normally */
            MultiCursor_Clear(pState);
            InvalidateRect(hEdit, NULL, TRUE);
            /* Don't return TRUE - let RichEdit process the click */
        }
        return FALSE;
    }
    
    /* Handle Alt+Click for adding/removing cursors */
    if (msg == WM_LBUTTONDOWN && (GetKeyState(VK_MENU) & 0x8000)) {
        /* Get character position from mouse coordinates */
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        DWORD dwCharPos = (DWORD)SendMessage(hEdit, EM_CHARFROMPOS, 0, (LPARAM)&pt);
        
        /* Check if cursor exists at this position */
        int nExisting = MultiCursor_FindAtPosition(pState, dwCharPos);
        
        if (nExisting >= 0 && pState->nCursorCount > 1) {
            /* Remove existing cursor */
            MultiCursor_RemoveByIndex(pState, nExisting);
        } else {
            /* Sync current selection as primary cursor first */
            if (!pState->bActive) {
                MultiCursor_SyncFromEdit(hEdit, pState);
            }
            /* Add new cursor */
            MultiCursor_Add(pState, dwCharPos);
        }
        
        InvalidateRect(hEdit, NULL, TRUE);
        return TRUE;
    }
    
    /* Handle Alt+Shift+Drag for column selection */
    if (msg == WM_LBUTTONDOWN && (GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        DWORD dwCharPos = (DWORD)SendMessage(hEdit, EM_CHARFROMPOS, 0, (LPARAM)&pt);
        
        int nLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwCharPos, 0);
        DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, nLine, 0);
        int nCol = dwCharPos - dwLineStart;
        
        MultiCursor_StartColumnSelect(hEdit, pState, nLine, nCol);
        return TRUE;
    }
    
    return FALSE;
}


/* ============================================
 * CURSOR OVERLAY RENDERING
 * ============================================ */

/* Timer ID for cursor blink */
#define TIMER_CURSOR_BLINK 101
static BOOL g_bCursorVisible = TRUE;

/* Draw secondary cursors on edit control */
void MultiCursor_DrawOverlay(HWND hEdit, HDC hdc, MultiCursorState* pState) {
    if (!hEdit || !hdc || !pState) return;
    if (!pState->bActive || pState->nCursorCount <= 1) return;
    
    /* Get edit control client rect */
    RECT rcClient;
    GetClientRect(hEdit, &rcClient);
    
    /* Get font for measuring */
    HFONT hFont = (HFONT)SendMessage(hEdit, WM_GETFONT, 0, 0);
    HFONT hOldFont = NULL;
    if (hFont) {
        hOldFont = (HFONT)SelectObject(hdc, hFont);
    }
    
    /* Get text metrics for cursor height */
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int nCursorHeight = tm.tmHeight;
    
    /* Create cursor pen */
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 128, 0)); /* Orange for secondary cursors */
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    /* Create selection brush */
    HBRUSH hSelBrush = CreateSolidBrush(RGB(100, 149, 237)); /* Cornflower blue */
    
    /* Draw each secondary cursor */
    for (int i = 0; i < pState->nCursorCount; i++) {
        /* Skip primary cursor (handled by RichEdit) */
        if (i == pState->nPrimaryCursor) continue;
        
        CursorInfo* pCursor = &pState->cursors[i];
        
        /* Draw selection highlight if cursor has selection */
        if (pCursor->bHasSelection && pCursor->dwSelStart != pCursor->dwSelEnd) {
            /* Get selection start and end positions */
            POINTL ptStart, ptEnd;
            SendMessage(hEdit, EM_POSFROMCHAR, (WPARAM)&ptStart, pCursor->dwSelStart);
            SendMessage(hEdit, EM_POSFROMCHAR, (WPARAM)&ptEnd, pCursor->dwSelEnd);
            
            /* Draw selection rectangle (simplified - single line only) */
            if (ptStart.y == ptEnd.y) {
                RECT rcSel;
                rcSel.left = ptStart.x;
                rcSel.top = ptStart.y;
                rcSel.right = ptEnd.x;
                rcSel.bottom = ptStart.y + nCursorHeight;
                
                /* Use alpha blend for selection */
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hSelBrush);
                SetBkMode(hdc, TRANSPARENT);
                FillRect(hdc, &rcSel, hSelBrush);
                SelectObject(hdc, hOldBrush);
            }
        }
        
        /* Draw cursor caret if visible */
        if (g_bCursorVisible) {
            CursorPixelPos pos = MultiCursor_GetPixelPos(hEdit, pCursor->dwPosition);
            
            /* Only draw if cursor is visible in client area */
            if (pos.x >= rcClient.left && pos.x <= rcClient.right &&
                pos.y >= rcClient.top && pos.y + nCursorHeight <= rcClient.bottom) {
                MoveToEx(hdc, pos.x, pos.y, NULL);
                LineTo(hdc, pos.x, pos.y + nCursorHeight);
            }
        }
    }
    
    /* Cleanup */
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    DeleteObject(hSelBrush);
    if (hOldFont) {
        SelectObject(hdc, hOldFont);
    }
}

/* Toggle cursor visibility for blink effect */
void MultiCursor_ToggleBlink(void) {
    g_bCursorVisible = !g_bCursorVisible;
}

/* Get cursor visibility state */
BOOL MultiCursor_IsCursorVisible(void) {
    return g_bCursorVisible;
}
