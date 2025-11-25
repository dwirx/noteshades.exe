#ifndef RESOURCE_H
#define RESOURCE_H

/* File menu command IDs */
#define IDM_FILE_NEW        101
#define IDM_FILE_OPEN       102
#define IDM_FILE_SAVE       103
#define IDM_FILE_SAVEAS     104
#define IDM_FILE_EXIT       105

/* Edit menu command IDs */
#define IDM_EDIT_UNDO       201
#define IDM_EDIT_CUT        202
#define IDM_EDIT_COPY       203
#define IDM_EDIT_PASTE      204
#define IDM_EDIT_SELECTALL  205

/* Format menu command IDs */
#define IDM_FORMAT_WORDWRAP 251

/* View menu command IDs */
#define IDM_VIEW_LINENUMBERS 261
#define IDM_VIEW_SYNTAX      262

/* Help menu command IDs */
#define IDM_HELP_ABOUT      301

/* Control IDs */
#define IDC_EDIT            401
#define IDC_STATUS          402
#define IDC_TAB             403
#define IDC_LINENUMBERS     404

/* Tab menu command IDs */
#define IDM_FILE_NEWTAB     106
#define IDM_FILE_CLOSETAB   107

/* Menu resource ID */
#define IDR_MAINMENU        1000

/* Accelerator table ID */
#define IDR_ACCEL           1001

/* Status bar part indices */
#define SB_PART_FILETYPE    0
#define SB_PART_LENGTH      1
#define SB_PART_LINES       2
#define SB_PART_POSITION    3
#define SB_PART_LINEENDING  4
#define SB_PART_ENCODING    5
#define SB_PART_INSERTMODE  6
#define SB_PART_COUNT       7

/* Status bar widths */
#define SB_WIDTH_FILETYPE   120
#define SB_WIDTH_LENGTH     100
#define SB_WIDTH_LINES      80
#define SB_WIDTH_POSITION   180
#define SB_WIDTH_LINEENDING 100
#define SB_WIDTH_ENCODING   70
#define SB_WIDTH_INSERTMODE 50

/* Timer IDs */
#define TIMER_STATUSBAR     4

#endif /* RESOURCE_H */
