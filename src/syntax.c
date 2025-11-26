#include "notepad.h"
#include "syntax.h"
#include "theme.h"
#include <richedit.h>

/* Global syntax highlighting flag - ON by default */
BOOL g_bSyntaxHighlight = TRUE;

/* Get syntax color from current theme */
COLORREF GetSyntaxColor(SyntaxColorType colorType) {
    const ThemeColors* pTheme = GetThemeColors();
    
    switch (colorType) {
        case SYNTAX_COLOR_KEYWORD:
            return pTheme->crKeyword;
        case SYNTAX_COLOR_STRING:
            return pTheme->crString;
        case SYNTAX_COLOR_COMMENT:
            return pTheme->crComment;
        case SYNTAX_COLOR_NUMBER:
            return pTheme->crNumber;
        case SYNTAX_COLOR_PREPROCESSOR:
            return pTheme->crPreprocessor;
        case SYNTAX_COLOR_TYPE:
            return pTheme->crType;
        case SYNTAX_COLOR_FUNCTION:
            return pTheme->crFunction;
        case SYNTAX_COLOR_OPERATOR:
            return pTheme->crOperator;
        case SYNTAX_COLOR_DEFAULT:
            return pTheme->crForeground;
        case SYNTAX_COLOR_VARIABLE:
            return pTheme->crFunction;
        case SYNTAX_COLOR_CONSTANT:
            return pTheme->crNumber;
        default:
            return pTheme->crForeground;
    }
}

/* C/C++ keywords */
static const WCHAR* g_CKeywords[] = {
    L"auto", L"break", L"case", L"char", L"const", L"continue", L"default",
    L"do", L"double", L"else", L"enum", L"extern", L"float", L"for", L"goto",
    L"if", L"inline", L"int", L"long", L"register", L"return", L"short",
    L"signed", L"sizeof", L"static", L"struct", L"switch", L"typedef",
    L"union", L"unsigned", L"void", L"volatile", L"while", L"class",
    L"public", L"private", L"protected", L"virtual", L"override", L"final",
    L"nullptr", L"new", L"delete", L"this", L"template", L"typename",
    L"namespace", L"using", L"try", L"catch", L"throw", L"true", L"false",
    L"bool", L"explicit", L"friend", L"mutable", L"operator", L"constexpr",
    NULL
};

/* JavaScript/TypeScript keywords */
static const WCHAR* g_JSKeywords[] = {
    L"break", L"case", L"catch", L"class", L"const", L"continue", L"debugger",
    L"default", L"delete", L"do", L"else", L"export", L"extends", L"finally",
    L"for", L"function", L"if", L"import", L"in", L"instanceof", L"let", L"new",
    L"return", L"super", L"switch", L"this", L"throw", L"try", L"typeof", L"var",
    L"void", L"while", L"with", L"yield", L"async", L"await", L"static",
    L"implements", L"interface", L"package", L"private", L"protected", L"public",
    L"enum", L"type", L"namespace", L"abstract", L"declare", L"readonly",
    L"true", L"false", L"null", L"undefined", L"NaN", L"Infinity",
    NULL
};

/* Python keywords */
static const WCHAR* g_PythonKeywords[] = {
    L"False", L"None", L"True", L"and", L"as", L"assert", L"async", L"await",
    L"break", L"class", L"continue", L"def", L"del", L"elif", L"else", L"except",
    L"finally", L"for", L"from", L"global", L"if", L"import", L"in", L"is",
    L"lambda", L"nonlocal", L"not", L"or", L"pass", L"raise", L"return", L"try",
    L"while", L"with", L"yield", L"self", L"print",
    NULL
};


/* Go keywords */
static const WCHAR* g_GoKeywords[] = {
    L"break", L"case", L"chan", L"const", L"continue", L"default", L"defer",
    L"else", L"fallthrough", L"for", L"func", L"go", L"goto", L"if", L"import",
    L"interface", L"map", L"package", L"range", L"return", L"select", L"struct",
    L"switch", L"type", L"var", L"true", L"false", L"nil", L"iota",
    NULL
};

/* Rust keywords */
static const WCHAR* g_RustKeywords[] = {
    L"as", L"break", L"const", L"continue", L"crate", L"else", L"enum",
    L"extern", L"false", L"fn", L"for", L"if", L"impl", L"in", L"let", L"loop",
    L"match", L"mod", L"move", L"mut", L"pub", L"ref", L"return", L"self",
    L"Self", L"static", L"struct", L"super", L"trait", L"true", L"type",
    L"unsafe", L"use", L"where", L"while", L"async", L"await", L"dyn",
    NULL
};

/* Java keywords */
static const WCHAR* g_JavaKeywords[] = {
    L"abstract", L"assert", L"boolean", L"break", L"byte", L"case", L"catch",
    L"char", L"class", L"const", L"continue", L"default", L"do", L"double",
    L"else", L"enum", L"extends", L"final", L"finally", L"float", L"for",
    L"goto", L"if", L"implements", L"import", L"instanceof", L"int",
    L"interface", L"long", L"native", L"new", L"package", L"private",
    L"protected", L"public", L"return", L"short", L"static", L"strictfp",
    L"super", L"switch", L"synchronized", L"this", L"throw", L"throws",
    L"transient", L"try", L"void", L"volatile", L"while", L"true", L"false",
    L"null",
    NULL
};

/* SQL keywords */
static const WCHAR* g_SQLKeywords[] = {
    L"SELECT", L"FROM", L"WHERE", L"INSERT", L"UPDATE", L"DELETE", L"CREATE",
    L"DROP", L"ALTER", L"TABLE", L"INDEX", L"VIEW", L"DATABASE", L"INTO",
    L"VALUES", L"SET", L"AND", L"OR", L"NOT", L"NULL", L"IS", L"IN", L"LIKE",
    L"BETWEEN", L"JOIN", L"INNER", L"LEFT", L"RIGHT", L"OUTER", L"ON", L"AS",
    L"ORDER", L"BY", L"GROUP", L"HAVING", L"LIMIT", L"OFFSET", L"UNION",
    L"ALL", L"DISTINCT", L"COUNT", L"SUM", L"AVG", L"MAX", L"MIN", L"ASC",
    L"DESC", L"PRIMARY", L"KEY", L"FOREIGN", L"REFERENCES", L"CONSTRAINT",
    L"DEFAULT", L"CHECK", L"UNIQUE", L"CASCADE", L"TRUNCATE", L"BEGIN",
    L"COMMIT", L"ROLLBACK", L"TRANSACTION", L"GRANT", L"REVOKE",
    /* Lowercase versions */
    L"select", L"from", L"where", L"insert", L"update", L"delete", L"create",
    L"drop", L"alter", L"table", L"index", L"view", L"database", L"into",
    L"values", L"set", L"and", L"or", L"not", L"null", L"is", L"in", L"like",
    L"between", L"join", L"inner", L"left", L"right", L"outer", L"on", L"as",
    L"order", L"by", L"group", L"having", L"limit", L"offset", L"union",
    NULL
};

/* CSS keywords/properties */
static const WCHAR* g_CSSKeywords[] = {
    L"color", L"background", L"margin", L"padding", L"border", L"width",
    L"height", L"display", L"position", L"top", L"left", L"right", L"bottom",
    L"font", L"text", L"align", L"flex", L"grid", L"transform", L"transition",
    L"animation", L"opacity", L"visibility", L"overflow", L"z-index",
    L"important", L"none", L"auto", L"inherit", L"initial", L"block", L"inline",
    L"relative", L"absolute", L"fixed", L"static", L"center", L"solid",
    NULL
};

/* JSON keywords */
static const WCHAR* g_JSONKeywords[] = {
    L"true", L"false", L"null",
    NULL
};


/* Detect language from file extension */
LanguageType DetectLanguage(const TCHAR* szFileName) {
    if (!szFileName || szFileName[0] == TEXT('\0')) {
        return LANG_NONE;
    }
    
    const TCHAR* pExt = _tcsrchr(szFileName, TEXT('.'));
    if (!pExt) return LANG_NONE;
    
    /* C/C++ */
    if (_tcsicmp(pExt, TEXT(".c")) == 0) return LANG_C;
    if (_tcsicmp(pExt, TEXT(".h")) == 0) return LANG_C;
    if (_tcsicmp(pExt, TEXT(".cpp")) == 0) return LANG_CPP;
    if (_tcsicmp(pExt, TEXT(".cxx")) == 0) return LANG_CPP;
    if (_tcsicmp(pExt, TEXT(".cc")) == 0) return LANG_CPP;
    if (_tcsicmp(pExt, TEXT(".hpp")) == 0) return LANG_CPP;
    if (_tcsicmp(pExt, TEXT(".hxx")) == 0) return LANG_CPP;
    
    /* JavaScript/TypeScript */
    if (_tcsicmp(pExt, TEXT(".js")) == 0) return LANG_JAVASCRIPT;
    if (_tcsicmp(pExt, TEXT(".jsx")) == 0) return LANG_JAVASCRIPT;
    if (_tcsicmp(pExt, TEXT(".mjs")) == 0) return LANG_JAVASCRIPT;
    if (_tcsicmp(pExt, TEXT(".ts")) == 0) return LANG_TYPESCRIPT;
    if (_tcsicmp(pExt, TEXT(".tsx")) == 0) return LANG_TYPESCRIPT;
    
    /* Python */
    if (_tcsicmp(pExt, TEXT(".py")) == 0) return LANG_PYTHON;
    if (_tcsicmp(pExt, TEXT(".pyw")) == 0) return LANG_PYTHON;
    if (_tcsicmp(pExt, TEXT(".pyx")) == 0) return LANG_PYTHON;
    
    /* Go */
    if (_tcsicmp(pExt, TEXT(".go")) == 0) return LANG_GO;
    
    /* Rust */
    if (_tcsicmp(pExt, TEXT(".rs")) == 0) return LANG_RUST;
    
    /* Java */
    if (_tcsicmp(pExt, TEXT(".java")) == 0) return LANG_JAVA;
    
    /* Web */
    if (_tcsicmp(pExt, TEXT(".html")) == 0) return LANG_HTML;
    if (_tcsicmp(pExt, TEXT(".htm")) == 0) return LANG_HTML;
    if (_tcsicmp(pExt, TEXT(".xml")) == 0) return LANG_XML;
    if (_tcsicmp(pExt, TEXT(".css")) == 0) return LANG_CSS;
    if (_tcsicmp(pExt, TEXT(".scss")) == 0) return LANG_CSS;
    if (_tcsicmp(pExt, TEXT(".less")) == 0) return LANG_CSS;
    
    /* Data formats */
    if (_tcsicmp(pExt, TEXT(".json")) == 0) return LANG_JSON;
    if (_tcsicmp(pExt, TEXT(".yaml")) == 0) return LANG_YAML;
    if (_tcsicmp(pExt, TEXT(".yml")) == 0) return LANG_YAML;
    
    /* SQL */
    if (_tcsicmp(pExt, TEXT(".sql")) == 0) return LANG_SQL;
    
    /* PHP */
    if (_tcsicmp(pExt, TEXT(".php")) == 0) return LANG_PHP;
    
    /* Ruby */
    if (_tcsicmp(pExt, TEXT(".rb")) == 0) return LANG_RUBY;
    
    /* Shell */
    if (_tcsicmp(pExt, TEXT(".sh")) == 0) return LANG_SHELL;
    if (_tcsicmp(pExt, TEXT(".bash")) == 0) return LANG_SHELL;
    if (_tcsicmp(pExt, TEXT(".zsh")) == 0) return LANG_SHELL;
    if (_tcsicmp(pExt, TEXT(".bat")) == 0) return LANG_BATCH;
    if (_tcsicmp(pExt, TEXT(".cmd")) == 0) return LANG_BATCH;
    if (_tcsicmp(pExt, TEXT(".ps1")) == 0) return LANG_POWERSHELL;
    
    /* Markdown */
    if (_tcsicmp(pExt, TEXT(".md")) == 0) return LANG_MARKDOWN;
    if (_tcsicmp(pExt, TEXT(".markdown")) == 0) return LANG_MARKDOWN;
    
    return LANG_NONE;
}

/* Get language name string */
const TCHAR* GetLanguageName(LanguageType lang) {
    switch (lang) {
        case LANG_C: return TEXT("C");
        case LANG_CPP: return TEXT("C++");
        case LANG_JAVA: return TEXT("Java");
        case LANG_JAVASCRIPT: return TEXT("JavaScript");
        case LANG_TYPESCRIPT: return TEXT("TypeScript");
        case LANG_PYTHON: return TEXT("Python");
        case LANG_GO: return TEXT("Go");
        case LANG_RUST: return TEXT("Rust");
        case LANG_HTML: return TEXT("HTML");
        case LANG_CSS: return TEXT("CSS");
        case LANG_JSON: return TEXT("JSON");
        case LANG_XML: return TEXT("XML");
        case LANG_YAML: return TEXT("YAML");
        case LANG_SQL: return TEXT("SQL");
        case LANG_PHP: return TEXT("PHP");
        case LANG_RUBY: return TEXT("Ruby");
        case LANG_SHELL: return TEXT("Shell");
        case LANG_BATCH: return TEXT("Batch");
        case LANG_POWERSHELL: return TEXT("PowerShell");
        case LANG_MARKDOWN: return TEXT("Markdown");
        default: return TEXT("Plain Text");
    }
}


/* Get keywords array for language */
static const WCHAR** GetKeywords(LanguageType lang) {
    switch (lang) {
        case LANG_C:
        case LANG_CPP:
            return g_CKeywords;
        case LANG_JAVASCRIPT:
        case LANG_TYPESCRIPT:
            return g_JSKeywords;
        case LANG_PYTHON:
            return g_PythonKeywords;
        case LANG_GO:
            return g_GoKeywords;
        case LANG_RUST:
            return g_RustKeywords;
        case LANG_JAVA:
            return g_JavaKeywords;
        case LANG_SQL:
            return g_SQLKeywords;
        case LANG_CSS:
            return g_CSSKeywords;
        case LANG_JSON:
            return g_JSONKeywords;
        default:
            return NULL;
    }
}

/* Check if word is a keyword */
static BOOL IsKeyword(const WCHAR* word, int len, LanguageType lang) {
    const WCHAR** keywords = GetKeywords(lang);
    if (!keywords) return FALSE;
    
    for (int i = 0; keywords[i] != NULL; i++) {
        if (wcslen(keywords[i]) == (size_t)len && 
            wcsncmp(word, keywords[i], len) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Check if character is identifier char */
static BOOL IsIdentChar(WCHAR ch) {
    return (ch >= L'a' && ch <= L'z') ||
           (ch >= L'A' && ch <= L'Z') ||
           (ch >= L'0' && ch <= L'9') ||
           ch == L'_' || ch == L'$';  /* $ is valid in JS identifiers */
}

/* Check if character is digit */
static BOOL IsDigit(WCHAR ch) {
    return ch >= L'0' && ch <= L'9';
}

/* Set text color for range in RichEdit */
/* Optimized: Set range color with direct COLORREF value */
static void SetRangeColorDirect(HWND hwndEdit, int nStart, int nEnd, COLORREF color) {
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    
    /* Select range and apply color - no need to save/restore for batch operations */
    SendMessage(hwndEdit, EM_SETSEL, nStart, nEnd);
    SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

/* Legacy wrapper for compatibility */
static void SetRangeColor(HWND hwndEdit, int nStart, int nEnd, COLORREF color) {
    SetRangeColorDirect(hwndEdit, nStart, nEnd, color);
}

/* Get comment style for language */
static void GetCommentStyle(LanguageType lang, WCHAR* lineComment, WCHAR* blockStart, WCHAR* blockEnd) {
    lineComment[0] = L'\0';
    blockStart[0] = L'\0';
    blockEnd[0] = L'\0';
    
    switch (lang) {
        case LANG_C:
        case LANG_CPP:
        case LANG_JAVA:
        case LANG_JAVASCRIPT:
        case LANG_TYPESCRIPT:
        case LANG_GO:
        case LANG_RUST:
        case LANG_CSS:
        case LANG_PHP:
            wcscpy(lineComment, L"//");
            wcscpy(blockStart, L"/*");
            wcscpy(blockEnd, L"*/");
            break;
        case LANG_PYTHON:
        case LANG_SHELL:
        case LANG_YAML:
            wcscpy(lineComment, L"#");
            break;
        case LANG_SQL:
            wcscpy(lineComment, L"--");
            wcscpy(blockStart, L"/*");
            wcscpy(blockEnd, L"*/");
            break;
        case LANG_HTML:
        case LANG_XML:
            wcscpy(blockStart, L"<!--");
            wcscpy(blockEnd, L"-->");
            break;
        case LANG_BATCH:
            wcscpy(lineComment, L"REM ");
            break;
        case LANG_POWERSHELL:
            wcscpy(lineComment, L"#");
            wcscpy(blockStart, L"<#");
            wcscpy(blockEnd, L"#>");
            break;
        default:
            break;
    }
}


/* Apply syntax highlighting to visible text */
void ApplySyntaxHighlighting(HWND hwndEdit, LanguageType lang) {
    if (!hwndEdit || !g_bSyntaxHighlight || lang == LANG_NONE) return;
    
    /* Get text length */
    int nLen = GetWindowTextLengthW(hwndEdit);
    if (nLen == 0) return;

    /* SKIP syntax highlighting for large files for performance
     * Large files cause significant lag due to:
     * - Full text buffer allocation (nLen * 2 bytes for WCHAR)
     * - Sequential parsing of entire document character-by-character
     * - Multiple SendMessage calls for coloring ranges
     * - RichEdit control becomes slow with many formatting ranges
     *
     * Limit set to 1MB characters (~2MB file size) for optimal performance
     * This ensures smooth editing even on slower machines.
     * For larger files, syntax highlighting is disabled automatically.
     */
    if (nLen > 1024 * 1024) {
        return; /* No highlighting for files > 1MB chars for better performance */
    }
    
    /* Cache theme colors for performance */
    const ThemeColors* pTheme = GetThemeColors();
    COLORREF crDefault = pTheme->crForeground;
    COLORREF crKeyword = pTheme->crKeyword;
    COLORREF crString = pTheme->crString;
    COLORREF crComment = pTheme->crComment;
    COLORREF crNumber = pTheme->crNumber;
    COLORREF crPreprocessor = pTheme->crPreprocessor;
    
    /* Disable redraw and event mask for performance */
    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
    DWORD dwOldMask = (DWORD)SendMessage(hwndEdit, EM_SETEVENTMASK, 0, 0);
    
    /* Allocate buffer for text */
    WCHAR* pText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nLen + 2) * sizeof(WCHAR));
    if (!pText) {
        SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
        return;
    }
    
    /* Get text content */
    GetWindowTextW(hwndEdit, pText, nLen + 1);
    
    /* Get comment style */
    WCHAR lineComment[8], blockStart[8], blockEnd[8];
    GetCommentStyle(lang, lineComment, blockStart, blockEnd);
    int lineCommentLen = (int)wcslen(lineComment);
    int blockStartLen = (int)wcslen(blockStart);
    int blockEndLen = (int)wcslen(blockEnd);
    
    /* First, set all text to default color */
    SetRangeColorDirect(hwndEdit, 0, nLen, crDefault);
    
    int i = 0;
    BOOL inBlockComment = FALSE;
    WCHAR stringChar = 0;
    
    /* For JSON, highlight keys differently */
    BOOL isJSON = (lang == LANG_JSON);
    
    while (i < nLen) {
        /* Block comment */
        if (inBlockComment) {
            int start = i;
            while (i < nLen) {
                if (blockEndLen > 0 && i + blockEndLen <= nLen &&
                    wcsncmp(&pText[i], blockEnd, blockEndLen) == 0) {
                    i += blockEndLen;
                    inBlockComment = FALSE;
                    break;
                }
                i++;
            }
            SetRangeColorDirect(hwndEdit, start, i, crComment);
            continue;
        }
        
        /* Line comment - check BEFORE block comment for languages like C/JS */
        if (lineCommentLen > 0 && i + lineCommentLen <= nLen &&
            wcsncmp(&pText[i], lineComment, lineCommentLen) == 0) {
            int start = i;
            while (i < nLen && pText[i] != L'\n') i++;
            SetRangeColorDirect(hwndEdit, start, i, crComment);
            continue;
        }
        
        /* Check for block comment start */
        if (blockStartLen > 0 && i + blockStartLen <= nLen &&
            wcsncmp(&pText[i], blockStart, blockStartLen) == 0) {
            int start = i;
            inBlockComment = TRUE;
            i += blockStartLen;
            /* Find end of block comment */
            while (i < nLen) {
                if (i + blockEndLen <= nLen &&
                    wcsncmp(&pText[i], blockEnd, blockEndLen) == 0) {
                    i += blockEndLen;
                    inBlockComment = FALSE;
                    break;
                }
                i++;
            }
            SetRangeColorDirect(hwndEdit, start, i, crComment);
            continue;
        }
        
        /* String */
        if (pText[i] == L'"' || pText[i] == L'\'' || 
            (lang == LANG_PYTHON && pText[i] == L'`') ||
            (lang == LANG_JAVASCRIPT && pText[i] == L'`') ||
            (lang == LANG_TYPESCRIPT && pText[i] == L'`')) {
            stringChar = pText[i];
            int start = i;
            i++;
            while (i < nLen) {
                if (pText[i] == L'\\' && i + 1 < nLen) {
                    i += 2; /* Skip escaped char */
                    continue;
                }
                if (pText[i] == stringChar) {
                    i++;
                    break;
                }
                if (pText[i] == L'\n' && stringChar != L'`') {
                    break; /* End of line for non-template strings */
                }
                i++;
            }
            
            /* For JSON, check if this is a key (followed by :) */
            if (isJSON && stringChar == L'"') {
                int j = i;
                while (j < nLen && (pText[j] == L' ' || pText[j] == L'\t')) j++;
                if (j < nLen && pText[j] == L':') {
                    SetRangeColorDirect(hwndEdit, start, i, crKeyword);
                } else {
                    SetRangeColorDirect(hwndEdit, start, i, crString);
                }
            } else {
                SetRangeColorDirect(hwndEdit, start, i, crString);
            }
            continue;
        }
        
        /* Preprocessor (C/C++) */
        if ((lang == LANG_C || lang == LANG_CPP) && pText[i] == L'#') {
            int start = i;
            while (i < nLen && pText[i] != L'\n') i++;
            SetRangeColorDirect(hwndEdit, start, i, crPreprocessor);
            continue;
        }
        
        /* Number */
        if (IsDigit(pText[i]) || 
            (pText[i] == L'.' && i + 1 < nLen && IsDigit(pText[i + 1]))) {
            int start = i;
            /* Handle hex (0x), binary (0b), octal (0o) */
            if (pText[i] == L'0' && i + 1 < nLen) {
                if (pText[i + 1] == L'x' || pText[i + 1] == L'X') {
                    i += 2;
                    while (i < nLen && ((pText[i] >= L'0' && pText[i] <= L'9') ||
                           (pText[i] >= L'a' && pText[i] <= L'f') ||
                           (pText[i] >= L'A' && pText[i] <= L'F'))) i++;
                } else if (pText[i + 1] == L'b' || pText[i + 1] == L'B') {
                    i += 2;
                    while (i < nLen && (pText[i] == L'0' || pText[i] == L'1')) i++;
                }
            }
            if (i == start) {
                while (i < nLen && (IsDigit(pText[i]) || pText[i] == L'.' || 
                       pText[i] == L'e' || pText[i] == L'E' ||
                       pText[i] == L'f' || pText[i] == L'F' ||
                       pText[i] == L'l' || pText[i] == L'L' ||
                       pText[i] == L'u' || pText[i] == L'U')) i++;
            }
            SetRangeColorDirect(hwndEdit, start, i, crNumber);
            continue;
        }
        
        /* Identifier/Keyword */
        if (IsIdentChar(pText[i]) && !IsDigit(pText[i])) {
            int start = i;
            while (i < nLen && IsIdentChar(pText[i])) i++;
            int len = i - start;
            
            if (IsKeyword(&pText[start], len, lang)) {
                SetRangeColorDirect(hwndEdit, start, i, crKeyword);
            }
            continue;
        }
        
        /* HTML/XML tags */
        if ((lang == LANG_HTML || lang == LANG_XML) && pText[i] == L'<') {
            int start = i;
            i++;
            /* Skip closing tag slash */
            if (i < nLen && pText[i] == L'/') i++;
            /* Tag name */
            int tagStart = i;
            while (i < nLen && IsIdentChar(pText[i])) i++;
            if (i > tagStart) {
                SetRangeColor(hwndEdit, start, i, COLOR_TAG);
            }
            /* Attributes */
            while (i < nLen && pText[i] != L'>') {
                if (IsIdentChar(pText[i]) && !IsDigit(pText[i])) {
                    int attrStart = i;
                    while (i < nLen && IsIdentChar(pText[i])) i++;
                    SetRangeColor(hwndEdit, attrStart, i, COLOR_ATTRIBUTE);
                } else if (pText[i] == L'"' || pText[i] == L'\'') {
                    stringChar = pText[i];
                    int strStart = i;
                    i++;
                    while (i < nLen && pText[i] != stringChar) i++;
                    if (i < nLen) i++;
                    SetRangeColor(hwndEdit, strStart, i, COLOR_STRING);
                } else {
                    i++;
                }
            }
            if (i < nLen) i++; /* Skip '>' */
            continue;
        }
        
        i++;
    }
    
    HeapFree(GetProcessHeap(), 0, pText);
    
    /* Restore selection to start */
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    
    /* Restore event mask and re-enable redraw */
    SendMessage(hwndEdit, EM_SETEVENTMASK, 0, dwOldMask);
    SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndEdit, NULL, FALSE);
}

/* Setup syntax highlighting for edit control */
void SetupSyntaxHighlighting(HWND hwndEdit, LanguageType lang) {
    if (!hwndEdit) return;
    
    /* Set default text color */
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = COLOR_DEFAULT;
    SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    /* Apply highlighting if enabled */
    if (g_bSyntaxHighlight && lang != LANG_NONE) {
        ApplySyntaxHighlighting(hwndEdit, lang);
    }
}
