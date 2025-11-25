/**
 * theme.c - Theme implementation for XNote
 * Tokyo Night and other popular themes
 */

#include "theme.h"
#include "notepad.h"
#include "session.h"
#include "syntax.h"
#include <richedit.h>

/* Global theme state */
ThemeType g_CurrentTheme = THEME_TOKYO_NIGHT;
ThemeColors g_ThemeColors = {0};

/* Theme definitions */
static const Theme g_Themes[] = {
    /* THEME_LIGHT - Default light theme */
    {
        THEME_LIGHT,
        TEXT("Light"),
        {
            RGB(255, 255, 255),  /* Background */
            RGB(0, 0, 0),        /* Foreground */
            RGB(128, 128, 128),  /* Line number */
            RGB(245, 245, 245),  /* Line num bg */
            RGB(255, 255, 200),  /* Current line */
            RGB(173, 214, 255),  /* Selection */
            RGB(0, 0, 0),        /* Cursor */
            RGB(0, 128, 0),      /* Comment */
            RGB(0, 0, 255),      /* Keyword */
            RGB(163, 21, 21),    /* String */
            RGB(9, 134, 88),     /* Number */
            RGB(121, 94, 38),    /* Function */
            RGB(38, 127, 153),   /* Type */
            RGB(0, 0, 0),        /* Operator */
            RGB(111, 0, 138),    /* Preprocessor */
            RGB(240, 240, 240),  /* Tab bg */
            RGB(255, 255, 255),  /* Tab active */
            RGB(225, 225, 225),  /* Tab inactive */
            RGB(0, 0, 0),        /* Tab text */
            RGB(0, 122, 204),    /* Status bg */
            RGB(255, 255, 255),  /* Status text */
        }
    },

    /* THEME_DARK - Simple dark theme */
    {
        THEME_DARK,
        TEXT("Dark"),
        {
            RGB(30, 30, 30),     /* Background */
            RGB(212, 212, 212),  /* Foreground */
            RGB(133, 133, 133),  /* Line number */
            RGB(37, 37, 38),     /* Line num bg */
            RGB(40, 40, 40),     /* Current line */
            RGB(38, 79, 120),    /* Selection */
            RGB(255, 255, 255),  /* Cursor */
            RGB(106, 153, 85),   /* Comment */
            RGB(86, 156, 214),   /* Keyword */
            RGB(206, 145, 120),  /* String */
            RGB(181, 206, 168),  /* Number */
            RGB(220, 220, 170),  /* Function */
            RGB(78, 201, 176),   /* Type */
            RGB(212, 212, 212),  /* Operator */
            RGB(155, 155, 155),  /* Preprocessor */
            RGB(37, 37, 38),     /* Tab bg */
            RGB(30, 30, 30),     /* Tab active */
            RGB(45, 45, 45),     /* Tab inactive */
            RGB(212, 212, 212),  /* Tab text */
            RGB(0, 122, 204),    /* Status bg */
            RGB(255, 255, 255),  /* Status text */
        }
    },

    /* THEME_TOKYO_NIGHT - Tokyo Night (main) */
    {
        THEME_TOKYO_NIGHT,
        TEXT("Tokyo Night"),
        {
            RGB(26, 27, 38),     /* Background - #1a1b26 */
            RGB(169, 177, 214),  /* Foreground - #a9b1d6 */
            RGB(84, 88, 112),    /* Line number - #545870 */
            RGB(22, 23, 32),     /* Line num bg - #16161e */
            RGB(41, 42, 58),     /* Current line - #292a3a */
            RGB(51, 59, 91),     /* Selection - #333b5b */
            RGB(199, 208, 245),  /* Cursor - #c7d0f5 */
            RGB(86, 95, 137),    /* Comment - #565f89 */
            RGB(187, 154, 247),  /* Keyword - #bb9af7 (purple) */
            RGB(158, 206, 106),  /* String - #9ece6a (green) */
            RGB(255, 158, 100),  /* Number - #ff9e64 (orange) */
            RGB(125, 207, 255),  /* Function - #7dcfff (cyan) */
            RGB(42, 195, 222),   /* Type - #2ac3de (teal) */
            RGB(137, 221, 255),  /* Operator - #89ddff */
            RGB(247, 118, 142),  /* Preprocessor - #f7768e (red) */
            RGB(22, 23, 32),     /* Tab bg - #16161e */
            RGB(26, 27, 38),     /* Tab active - #1a1b26 */
            RGB(36, 40, 59),     /* Tab inactive - #24283b */
            RGB(169, 177, 214),  /* Tab text - #a9b1d6 */
            RGB(65, 166, 181),   /* Status bg - #41a6b5 */
            RGB(26, 27, 38),     /* Status text - #1a1b26 */
        }
    },

    /* THEME_TOKYO_NIGHT_STORM - Tokyo Night Storm */
    {
        THEME_TOKYO_NIGHT_STORM,
        TEXT("Tokyo Night Storm"),
        {
            RGB(36, 40, 59),     /* Background - #24283b */
            RGB(169, 177, 214),  /* Foreground - #a9b1d6 */
            RGB(84, 88, 112),    /* Line number - #545870 */
            RGB(30, 33, 48),     /* Line num bg */
            RGB(47, 52, 73),     /* Current line */
            RGB(51, 59, 91),     /* Selection - #333b5b */
            RGB(199, 208, 245),  /* Cursor */
            RGB(86, 95, 137),    /* Comment - #565f89 */
            RGB(187, 154, 247),  /* Keyword - #bb9af7 */
            RGB(158, 206, 106),  /* String - #9ece6a */
            RGB(255, 158, 100),  /* Number - #ff9e64 */
            RGB(125, 207, 255),  /* Function - #7dcfff */
            RGB(42, 195, 222),   /* Type - #2ac3de */
            RGB(137, 221, 255),  /* Operator - #89ddff */
            RGB(247, 118, 142),  /* Preprocessor - #f7768e */
            RGB(30, 33, 48),     /* Tab bg */
            RGB(36, 40, 59),     /* Tab active */
            RGB(47, 52, 73),     /* Tab inactive */
            RGB(169, 177, 214),  /* Tab text */
            RGB(65, 166, 181),   /* Status bg */
            RGB(36, 40, 59),     /* Status text */
        }
    },

    /* THEME_TOKYO_NIGHT_LIGHT - Tokyo Night Light */
    {
        THEME_TOKYO_NIGHT_LIGHT,
        TEXT("Tokyo Night Light"),
        {
            RGB(213, 214, 219),  /* Background - #d5d6db */
            RGB(52, 59, 88),     /* Foreground - #343b58 */
            RGB(150, 152, 163),  /* Line number */
            RGB(223, 224, 229),  /* Line num bg */
            RGB(198, 200, 209),  /* Current line */
            RGB(153, 162, 195),  /* Selection */
            RGB(52, 59, 88),     /* Cursor */
            RGB(150, 152, 163),  /* Comment - #9699a3 */
            RGB(90, 74, 120),    /* Keyword - #5a4a78 */
            RGB(72, 94, 48),     /* String - #485e30 */
            RGB(150, 84, 54),    /* Number - #965436 */
            RGB(34, 102, 120),   /* Function - #226678 */
            RGB(23, 111, 130),   /* Type - #176f82 */
            RGB(52, 59, 88),     /* Operator */
            RGB(140, 68, 74),    /* Preprocessor - #8c444a */
            RGB(223, 224, 229),  /* Tab bg */
            RGB(213, 214, 219),  /* Tab active */
            RGB(198, 200, 209),  /* Tab inactive */
            RGB(52, 59, 88),     /* Tab text */
            RGB(34, 102, 120),   /* Status bg */
            RGB(255, 255, 255),  /* Status text */
        }
    },

    /* THEME_MONOKAI - Monokai */
    {
        THEME_MONOKAI,
        TEXT("Monokai"),
        {
            RGB(39, 40, 34),     /* Background - #272822 */
            RGB(248, 248, 242),  /* Foreground - #f8f8f2 */
            RGB(144, 144, 138),  /* Line number - #90908a */
            RGB(49, 50, 44),     /* Line num bg */
            RGB(60, 61, 54),     /* Current line */
            RGB(73, 72, 62),     /* Selection - #49483e */
            RGB(248, 248, 240),  /* Cursor */
            RGB(117, 113, 94),   /* Comment - #75715e */
            RGB(249, 38, 114),   /* Keyword - #f92672 (pink) */
            RGB(230, 219, 116),  /* String - #e6db74 (yellow) */
            RGB(174, 129, 255),  /* Number - #ae81ff (purple) */
            RGB(166, 226, 46),   /* Function - #a6e22e (green) */
            RGB(102, 217, 239),  /* Type - #66d9ef (cyan) */
            RGB(249, 38, 114),   /* Operator - #f92672 */
            RGB(249, 38, 114),   /* Preprocessor - #f92672 */
            RGB(49, 50, 44),     /* Tab bg */
            RGB(39, 40, 34),     /* Tab active */
            RGB(60, 61, 54),     /* Tab inactive */
            RGB(248, 248, 242),  /* Tab text */
            RGB(166, 226, 46),   /* Status bg */
            RGB(39, 40, 34),     /* Status text */
        }
    },

    /* THEME_DRACULA - Dracula */
    {
        THEME_DRACULA,
        TEXT("Dracula"),
        {
            RGB(40, 42, 54),     /* Background - #282a36 */
            RGB(248, 248, 242),  /* Foreground - #f8f8f2 */
            RGB(98, 114, 164),   /* Line number - #6272a4 */
            RGB(33, 34, 44),     /* Line num bg */
            RGB(68, 71, 90),     /* Current line - #44475a */
            RGB(68, 71, 90),     /* Selection - #44475a */
            RGB(248, 248, 242),  /* Cursor */
            RGB(98, 114, 164),   /* Comment - #6272a4 */
            RGB(255, 121, 198),  /* Keyword - #ff79c6 (pink) */
            RGB(241, 250, 140),  /* String - #f1fa8c (yellow) */
            RGB(189, 147, 249),  /* Number - #bd93f9 (purple) */
            RGB(80, 250, 123),   /* Function - #50fa7b (green) */
            RGB(139, 233, 253),  /* Type - #8be9fd (cyan) */
            RGB(255, 121, 198),  /* Operator - #ff79c6 */
            RGB(255, 85, 85),    /* Preprocessor - #ff5555 (red) */
            RGB(33, 34, 44),     /* Tab bg */
            RGB(40, 42, 54),     /* Tab active */
            RGB(68, 71, 90),     /* Tab inactive */
            RGB(248, 248, 242),  /* Tab text */
            RGB(189, 147, 249),  /* Status bg - #bd93f9 */
            RGB(40, 42, 54),     /* Status text */
        }
    },

    /* THEME_ONE_DARK - One Dark */
    {
        THEME_ONE_DARK,
        TEXT("One Dark"),
        {
            RGB(40, 44, 52),     /* Background - #282c34 */
            RGB(171, 178, 191),  /* Foreground - #abb2bf */
            RGB(76, 82, 99),     /* Line number - #4c5263 */
            RGB(33, 37, 43),     /* Line num bg */
            RGB(44, 49, 58),     /* Current line - #2c313a */
            RGB(62, 68, 81),     /* Selection - #3e4451 */
            RGB(171, 178, 191),  /* Cursor */
            RGB(92, 99, 112),    /* Comment - #5c6370 */
            RGB(198, 120, 221),  /* Keyword - #c678dd (purple) */
            RGB(152, 195, 121),  /* String - #98c379 (green) */
            RGB(209, 154, 102),  /* Number - #d19a66 (orange) */
            RGB(97, 175, 239),   /* Function - #61afef (blue) */
            RGB(86, 182, 194),   /* Type - #56b6c2 (cyan) */
            RGB(171, 178, 191),  /* Operator */
            RGB(224, 108, 117),  /* Preprocessor - #e06c75 (red) */
            RGB(33, 37, 43),     /* Tab bg */
            RGB(40, 44, 52),     /* Tab active */
            RGB(55, 60, 72),     /* Tab inactive */
            RGB(171, 178, 191),  /* Tab text */
            RGB(97, 175, 239),   /* Status bg */
            RGB(40, 44, 52),     /* Status text */
        }
    },

    /* THEME_NORD - Nord */
    {
        THEME_NORD,
        TEXT("Nord"),
        {
            RGB(46, 52, 64),     /* Background - #2e3440 */
            RGB(216, 222, 233),  /* Foreground - #d8dee9 */
            RGB(76, 86, 106),    /* Line number - #4c566a */
            RGB(59, 66, 82),     /* Line num bg - #3b4252 */
            RGB(59, 66, 82),     /* Current line - #3b4252 */
            RGB(67, 76, 94),     /* Selection - #434c5e */
            RGB(216, 222, 233),  /* Cursor */
            RGB(97, 110, 136),   /* Comment - #616e88 */
            RGB(129, 161, 193),  /* Keyword - #81a1c1 (blue) */
            RGB(163, 190, 140),  /* String - #a3be8c (green) */
            RGB(180, 142, 173),  /* Number - #b48ead (purple) */
            RGB(136, 192, 208),  /* Function - #88c0d0 (cyan) */
            RGB(143, 188, 187),  /* Type - #8fbcbb (teal) */
            RGB(129, 161, 193),  /* Operator - #81a1c1 */
            RGB(191, 97, 106),   /* Preprocessor - #bf616a (red) */
            RGB(59, 66, 82),     /* Tab bg */
            RGB(46, 52, 64),     /* Tab active */
            RGB(67, 76, 94),     /* Tab inactive */
            RGB(216, 222, 233),  /* Tab text */
            RGB(136, 192, 208),  /* Status bg */
            RGB(46, 52, 64),     /* Status text */
        }
    },

    /* THEME_GRUVBOX_DARK - Gruvbox Dark */
    {
        THEME_GRUVBOX_DARK,
        TEXT("Gruvbox Dark"),
        {
            RGB(40, 40, 40),     /* Background - #282828 */
            RGB(235, 219, 178),  /* Foreground - #ebdbb2 */
            RGB(146, 131, 116),  /* Line number - #928374 */
            RGB(50, 48, 47),     /* Line num bg - #32302f */
            RGB(60, 56, 54),     /* Current line - #3c3836 */
            RGB(80, 73, 69),     /* Selection - #504945 */
            RGB(235, 219, 178),  /* Cursor */
            RGB(146, 131, 116),  /* Comment - #928374 */
            RGB(251, 73, 52),    /* Keyword - #fb4934 (red) */
            RGB(184, 187, 38),   /* String - #b8bb26 (green) */
            RGB(211, 134, 155),  /* Number - #d3869b (purple) */
            RGB(250, 189, 47),   /* Function - #fabd2f (yellow) */
            RGB(131, 165, 152),  /* Type - #83a598 (aqua) */
            RGB(235, 219, 178),  /* Operator */
            RGB(254, 128, 25),   /* Preprocessor - #fe8019 (orange) */
            RGB(50, 48, 47),     /* Tab bg */
            RGB(40, 40, 40),     /* Tab active */
            RGB(60, 56, 54),     /* Tab inactive */
            RGB(235, 219, 178),  /* Tab text */
            RGB(250, 189, 47),   /* Status bg */
            RGB(40, 40, 40),     /* Status text */
        }
    },
};


/* Initialize theme system */
void InitTheme(void) {
    /* Set default theme to Tokyo Night */
    SetTheme(THEME_TOKYO_NIGHT);
}

/* Set current theme */
void SetTheme(ThemeType theme) {
    if (theme >= THEME_COUNT) {
        theme = THEME_TOKYO_NIGHT;
    }
    
    g_CurrentTheme = theme;
    g_ThemeColors = g_Themes[theme].colors;
    
    /* Mark session dirty */
    MarkSessionDirty();
}

/* Get current theme */
ThemeType GetCurrentTheme(void) {
    return g_CurrentTheme;
}

/* Get theme name */
const TCHAR* GetThemeName(ThemeType theme) {
    if (theme >= THEME_COUNT) {
        return TEXT("Unknown");
    }
    return g_Themes[theme].szName;
}

/* Get current theme colors */
const ThemeColors* GetThemeColors(void) {
    return &g_ThemeColors;
}

/* Get theme count */
int GetThemeCount(void) {
    return THEME_COUNT;
}

/* Apply theme to RichEdit control */
void ApplyThemeToEdit(HWND hwndEdit) {
    if (!hwndEdit) return;
    
    /* Set background color */
    SendMessage(hwndEdit, EM_SETBKGNDCOLOR, 0, g_ThemeColors.crBackground);
    
    /* Set default text color using CHARFORMAT2 */
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = g_ThemeColors.crForeground;
    SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    /* Refresh the control */
    InvalidateRect(hwndEdit, NULL, TRUE);
}

/* Apply theme to main window and all controls */
void ApplyThemeToWindow(HWND hwnd) {
    if (!hwnd) return;
    
    /* Apply to all edit controls in tabs */
    for (int i = 0; i < g_AppState.nTabCount; i++) {
        if (g_AppState.tabs[i].hwndEdit) {
            ApplyThemeToEdit(g_AppState.tabs[i].hwndEdit);
            
            /* Re-apply syntax highlighting with new theme colors */
            if (g_bSyntaxHighlight && g_AppState.tabs[i].language != LANG_NONE) {
                ApplySyntaxHighlighting(g_AppState.tabs[i].hwndEdit, g_AppState.tabs[i].language);
            }
        }
        
        /* Refresh line numbers */
        if (g_AppState.tabs[i].lineNumState.hwndLineNumbers) {
            InvalidateRect(g_AppState.tabs[i].lineNumState.hwndLineNumbers, NULL, TRUE);
        }
    }
    
    /* Refresh tab control */
    if (g_AppState.hwndTab) {
        InvalidateRect(g_AppState.hwndTab, NULL, TRUE);
    }
    
    /* Refresh status bar */
    if (g_AppState.hwndStatus) {
        InvalidateRect(g_AppState.hwndStatus, NULL, TRUE);
    }
    
    /* Refresh main window */
    InvalidateRect(hwnd, NULL, TRUE);
}
