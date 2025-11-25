/**
 * theme.h - Theme support for XNote
 * Includes Tokyo Night and other popular themes
 */

#ifndef THEME_H
#define THEME_H

#include <windows.h>

/* Theme types */
typedef enum {
    THEME_LIGHT = 0,        /* Default light theme */
    THEME_DARK,             /* Simple dark theme */
    THEME_TOKYO_NIGHT,      /* Tokyo Night (default dark) */
    THEME_TOKYO_NIGHT_STORM,/* Tokyo Night Storm */
    THEME_TOKYO_NIGHT_LIGHT,/* Tokyo Night Light */
    THEME_MONOKAI,          /* Monokai */
    THEME_DRACULA,          /* Dracula */
    THEME_ONE_DARK,         /* One Dark */
    THEME_NORD,             /* Nord */
    THEME_GRUVBOX_DARK,     /* Gruvbox Dark */
    THEME_EVERFOREST_DARK,  /* Everforest Dark */
    THEME_EVERFOREST_LIGHT, /* Everforest Light */
    THEME_COUNT
} ThemeType;

/* Theme color structure */
typedef struct {
    COLORREF crBackground;      /* Editor background */
    COLORREF crForeground;      /* Default text color */
    COLORREF crLineNumber;      /* Line number color */
    COLORREF crLineNumBg;       /* Line number background */
    COLORREF crCurrentLine;     /* Current line highlight */
    COLORREF crSelection;       /* Selection background */
    COLORREF crCursor;          /* Cursor color */
    COLORREF crComment;         /* Comment color */
    COLORREF crKeyword;         /* Keyword color */
    COLORREF crString;          /* String color */
    COLORREF crNumber;          /* Number color */
    COLORREF crFunction;        /* Function color */
    COLORREF crType;            /* Type color */
    COLORREF crOperator;        /* Operator color */
    COLORREF crPreprocessor;    /* Preprocessor color */
    COLORREF crTabBg;           /* Tab bar background */
    COLORREF crTabActive;       /* Active tab background */
    COLORREF crTabInactive;     /* Inactive tab background */
    COLORREF crTabText;         /* Tab text color */
    COLORREF crStatusBg;        /* Status bar background */
    COLORREF crStatusText;      /* Status bar text */
} ThemeColors;

/* Theme info structure */
typedef struct {
    ThemeType type;
    const TCHAR* szName;
    ThemeColors colors;
} Theme;

/* Global current theme */
extern ThemeType g_CurrentTheme;
extern ThemeColors g_ThemeColors;

/* Theme functions */
void InitTheme(void);
void SetTheme(ThemeType theme);
ThemeType GetCurrentTheme(void);
const TCHAR* GetThemeName(ThemeType theme);
const ThemeColors* GetThemeColors(void);
void ApplyThemeToWindow(HWND hwnd);
void ApplyThemeToEdit(HWND hwndEdit);

/* Get theme count for menu */
int GetThemeCount(void);

#endif /* THEME_H */
