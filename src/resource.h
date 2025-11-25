#ifndef RESOURCE_H
#define RESOURCE_H

/* Application Icon */
#define IDI_APPICON 1

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
#define IDM_EDIT_FIND       206
#define IDM_EDIT_REPLACE    207
#define IDM_EDIT_FINDNEXT   208

/* Help menu command IDs */
#define IDM_HELP_CONTENTS   302

/* Format menu command IDs */
#define IDM_FORMAT_WORDWRAP 251
#define IDM_FORMAT_FONT     252

/* View menu command IDs */
#define IDM_VIEW_LINENUMBERS 261
#define IDM_VIEW_SYNTAX      262
#define IDM_VIEW_VIMMODE     263
#define IDM_VIEW_RELATIVENUM 264

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
#define SB_PART_VIMMODE     7
#define SB_PART_COUNT       8

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

/* Tab navigation command IDs */
#define IDM_TAB_NEXT        108
#define IDM_TAB_PREV        109
#define IDM_TAB_1           110
#define IDM_TAB_2           111
#define IDM_TAB_3           112
#define IDM_TAB_4           113
#define IDM_TAB_5           114
#define IDM_TAB_6           115
#define IDM_TAB_7           116
#define IDM_TAB_8           117
#define IDM_TAB_9           118

/* Edit extended command IDs */
#define IDM_EDIT_GOTOLINE       209
#define IDM_EDIT_DUPLICATELINE  210
#define IDM_EDIT_DELETELINE     211
#define IDM_EDIT_MOVELINEUP     212
#define IDM_EDIT_MOVELINEDOWN   213
#define IDM_EDIT_TOGGLECOMMENT  214
#define IDM_EDIT_INDENT         215
#define IDM_EDIT_UNINDENT       216

/* View extended command IDs */
#define IDM_VIEW_ZOOMIN     265
#define IDM_VIEW_ZOOMOUT    266
#define IDM_VIEW_ZOOMRESET  267
#define IDM_VIEW_FULLSCREEN 268

/* Theme menu command IDs */
#define IDM_THEME_LIGHT         270
#define IDM_THEME_DARK          271
#define IDM_THEME_TOKYO_NIGHT   272
#define IDM_THEME_TOKYO_STORM   273
#define IDM_THEME_TOKYO_LIGHT   274
#define IDM_THEME_MONOKAI       275
#define IDM_THEME_DRACULA       276
#define IDM_THEME_ONE_DARK      277
#define IDM_THEME_NORD          278
#define IDM_THEME_GRUVBOX       279
#define IDM_THEME_EVERFOREST_DARK  280
#define IDM_THEME_EVERFOREST_LIGHT 281

/* Find/Replace dialog IDs */
#define IDD_FINDREPLACE     500
#define IDC_FIND_TEXT       501
#define IDC_REPLACE_TEXT    502
#define IDC_FIND_NEXT       503
#define IDC_REPLACE         504
#define IDC_REPLACE_ALL     505
#define IDC_MATCH_CASE      506
#define IDC_WHOLE_WORD      507

/* Go to Line dialog IDs */
#define IDD_GOTOLINE        550
#define IDC_GOTOLINE_EDIT   551
#define IDC_GOTOLINE_INFO   552

/* Help dialog ID */
#define IDD_HELP            600
#define IDC_HELP_TEXT       601

#endif /* RESOURCE_H */
