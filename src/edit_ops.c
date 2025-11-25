#include "notepad.h"

/* Undo last edit operation */
void EditUndo(HWND hEdit) {
    SendMessage(hEdit, EM_UNDO, 0, 0);
}

/* Cut selected text to clipboard */
void EditCut(HWND hEdit) {
    SendMessage(hEdit, WM_CUT, 0, 0);
}

/* Copy selected text to clipboard */
void EditCopy(HWND hEdit) {
    SendMessage(hEdit, WM_COPY, 0, 0);
}

/* Paste clipboard content at cursor position */
void EditPaste(HWND hEdit) {
    SendMessage(hEdit, WM_PASTE, 0, 0);
}

/* Select all text in edit control */
void EditSelectAll(HWND hEdit) {
    SendMessage(hEdit, EM_SETSEL, 0, -1);
}
