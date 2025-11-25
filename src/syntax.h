#ifndef SYNTAX_H
#define SYNTAX_H

#include <windows.h>
#include <richedit.h>
#include <tchar.h>

/* LanguageType is defined in notepad.h to avoid circular dependency */

/* Syntax color types */
typedef enum {
    SYNTAX_COLOR_KEYWORD = 0,
    SYNTAX_COLOR_STRING,
    SYNTAX_COLOR_COMMENT,
    SYNTAX_COLOR_NUMBER,
    SYNTAX_COLOR_PREPROCESSOR,
    SYNTAX_COLOR_TYPE,
    SYNTAX_COLOR_FUNCTION,
    SYNTAX_COLOR_OPERATOR,
    SYNTAX_COLOR_DEFAULT,
    SYNTAX_COLOR_TAG,
    SYNTAX_COLOR_ATTRIBUTE,
    SYNTAX_COLOR_VARIABLE,
    SYNTAX_COLOR_CONSTANT,
    SYNTAX_COLOR_COUNT
} SyntaxColorType;

/* Get syntax color from current theme */
COLORREF GetSyntaxColor(SyntaxColorType colorType);

/* Legacy macros for compatibility - now use theme colors */
#define COLOR_KEYWORD       GetSyntaxColor(SYNTAX_COLOR_KEYWORD)
#define COLOR_STRING        GetSyntaxColor(SYNTAX_COLOR_STRING)
#define COLOR_COMMENT       GetSyntaxColor(SYNTAX_COLOR_COMMENT)
#define COLOR_NUMBER        GetSyntaxColor(SYNTAX_COLOR_NUMBER)
#define COLOR_PREPROCESSOR  GetSyntaxColor(SYNTAX_COLOR_PREPROCESSOR)
#define COLOR_TYPE          GetSyntaxColor(SYNTAX_COLOR_TYPE)
#define COLOR_FUNCTION      GetSyntaxColor(SYNTAX_COLOR_FUNCTION)
#define COLOR_OPERATOR      GetSyntaxColor(SYNTAX_COLOR_OPERATOR)
#define COLOR_DEFAULT       GetSyntaxColor(SYNTAX_COLOR_DEFAULT)
#define COLOR_TAG           GetSyntaxColor(SYNTAX_COLOR_PREPROCESSOR)
#define COLOR_ATTRIBUTE     GetSyntaxColor(SYNTAX_COLOR_STRING)
#define COLOR_VARIABLE      GetSyntaxColor(SYNTAX_COLOR_VARIABLE)
#define COLOR_CONSTANT      GetSyntaxColor(SYNTAX_COLOR_NUMBER)

/* Syntax highlighting functions */
LanguageType DetectLanguage(const TCHAR* szFileName);
void ApplySyntaxHighlighting(HWND hwndEdit, LanguageType lang);
void HighlightLine(HWND hwndEdit, int nLine, LanguageType lang);
void SetupSyntaxHighlighting(HWND hwndEdit, LanguageType lang);
const TCHAR* GetLanguageName(LanguageType lang);

/* Enable/disable syntax highlighting */
extern BOOL g_bSyntaxHighlight;

#endif /* SYNTAX_H */
