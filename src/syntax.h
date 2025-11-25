#ifndef SYNTAX_H
#define SYNTAX_H

#include <windows.h>
#include <richedit.h>
#include <tchar.h>

/* LanguageType is defined in notepad.h to avoid circular dependency */

/* Syntax colors - Light theme (for white background) */
#define COLOR_KEYWORD       RGB(0, 0, 255)       /* Blue - keywords */
#define COLOR_STRING        RGB(163, 21, 21)     /* Dark red - strings */
#define COLOR_COMMENT       RGB(0, 128, 0)       /* Green - comments */
#define COLOR_NUMBER        RGB(9, 134, 88)      /* Teal - numbers */
#define COLOR_PREPROCESSOR  RGB(128, 128, 128)   /* Gray - preprocessor */
#define COLOR_TYPE          RGB(43, 145, 175)    /* Cyan - types */
#define COLOR_FUNCTION      RGB(116, 83, 31)     /* Brown - functions */
#define COLOR_OPERATOR      RGB(0, 0, 0)         /* Black - operators */
#define COLOR_DEFAULT       RGB(0, 0, 0)         /* Black - default text */
#define COLOR_TAG           RGB(128, 0, 0)       /* Dark red - HTML tags */
#define COLOR_ATTRIBUTE     RGB(255, 0, 0)       /* Red - attributes */
#define COLOR_VARIABLE      RGB(0, 16, 128)      /* Dark blue - variables */
#define COLOR_CONSTANT      RGB(9, 134, 88)      /* Teal - constants */

/* Background color */
#define SYNTAX_COLOR_BG     RGB(255, 255, 255)   /* White background */

/* Syntax highlighting functions */
LanguageType DetectLanguage(const TCHAR* szFileName);
void ApplySyntaxHighlighting(HWND hwndEdit, LanguageType lang);
void HighlightLine(HWND hwndEdit, int nLine, LanguageType lang);
void SetupSyntaxHighlighting(HWND hwndEdit, LanguageType lang);
const TCHAR* GetLanguageName(LanguageType lang);

/* Enable/disable syntax highlighting */
extern BOOL g_bSyntaxHighlight;

#endif /* SYNTAX_H */
