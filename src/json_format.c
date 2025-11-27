/*
 * JSON Formatter for XNote
 * Implements JSON parsing, formatting (pretty print), and minifying
 */

#include "json_format.h"
#include "notepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <richedit.h>

/* Parser state */
typedef struct {
    const WCHAR* input;
    int pos;
    int length;
    int line;
    int column;
    WCHAR errorMessage[256];
} JsonParser;

/* String builder for output */
typedef struct {
    WCHAR* buffer;
    int length;
    int capacity;
} StringBuilder;

/* Forward declarations */
static JsonValue* ParseValue(JsonParser* parser);
static void SkipWhitespace(JsonParser* parser);
static BOOL StringBuilder_Init(StringBuilder* sb, int initialCapacity);
static void StringBuilder_Free(StringBuilder* sb);
static BOOL StringBuilder_Append(StringBuilder* sb, const WCHAR* str);
static BOOL StringBuilder_AppendChar(StringBuilder* sb, WCHAR ch);
static void FormatValue(StringBuilder* sb, JsonValue* value, int indent, int indentSpaces);
static void MinifyValue(StringBuilder* sb, JsonValue* value);

/* ============== Memory Management ============== */

static JsonValue* CreateValue(JsonValueType type) {
    JsonValue* value = (JsonValue*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JsonValue));
    if (value) {
        value->type = type;
    }
    return value;
}

void JsonFreeValue(JsonValue* value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_STRING:
            if (value->data.stringValue) {
                HeapFree(GetProcessHeap(), 0, value->data.stringValue);
            }
            break;
        case JSON_ARRAY:
            for (int i = 0; i < value->data.array.count; i++) {
                JsonFreeValue(value->data.array.items[i]);
            }
            if (value->data.array.items) {
                HeapFree(GetProcessHeap(), 0, value->data.array.items);
            }
            break;
        case JSON_OBJECT:
            for (int i = 0; i < value->data.object.count; i++) {
                if (value->data.object.keys[i]) {
                    HeapFree(GetProcessHeap(), 0, value->data.object.keys[i]);
                }
                JsonFreeValue(value->data.object.values[i]);
            }
            if (value->data.object.keys) {
                HeapFree(GetProcessHeap(), 0, value->data.object.keys);
            }
            if (value->data.object.values) {
                HeapFree(GetProcessHeap(), 0, value->data.object.values);
            }
            break;
        default:
            break;
    }
    HeapFree(GetProcessHeap(), 0, value);
}

void JsonFreeParseResult(JsonParseResult* result) {
    if (!result) return;
    if (result->root) {
        JsonFreeValue(result->root);
    }
    HeapFree(GetProcessHeap(), 0, result);
}

/* ============== String Builder ============== */

static BOOL StringBuilder_Init(StringBuilder* sb, int initialCapacity) {
    sb->buffer = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, initialCapacity * sizeof(WCHAR));
    if (!sb->buffer) return FALSE;
    sb->length = 0;
    sb->capacity = initialCapacity;
    return TRUE;
}

static void StringBuilder_Free(StringBuilder* sb) {
    if (sb->buffer) {
        HeapFree(GetProcessHeap(), 0, sb->buffer);
        sb->buffer = NULL;
    }
}

static BOOL StringBuilder_Grow(StringBuilder* sb, int needed) {
    if (sb->length + needed < sb->capacity) return TRUE;
    
    int newCapacity = sb->capacity * 2;
    while (newCapacity < sb->length + needed) {
        newCapacity *= 2;
    }
    
    WCHAR* newBuffer = (WCHAR*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                            sb->buffer, newCapacity * sizeof(WCHAR));
    if (!newBuffer) return FALSE;
    
    sb->buffer = newBuffer;
    sb->capacity = newCapacity;
    return TRUE;
}

static BOOL StringBuilder_Append(StringBuilder* sb, const WCHAR* str) {
    int len = (int)wcslen(str);
    if (!StringBuilder_Grow(sb, len + 1)) return FALSE;
    wcscpy(sb->buffer + sb->length, str);
    sb->length += len;
    return TRUE;
}

static BOOL StringBuilder_AppendChar(StringBuilder* sb, WCHAR ch) {
    if (!StringBuilder_Grow(sb, 2)) return FALSE;
    sb->buffer[sb->length++] = ch;
    sb->buffer[sb->length] = L'\0';
    return TRUE;
}

static BOOL StringBuilder_AppendIndent(StringBuilder* sb, int spaces) {
    for (int i = 0; i < spaces; i++) {
        if (!StringBuilder_AppendChar(sb, L' ')) return FALSE;
    }
    return TRUE;
}

/* ============== Lexer/Tokenizer ============== */

static void InitParser(JsonParser* parser, const WCHAR* input) {
    parser->input = input;
    parser->pos = 0;
    parser->length = (int)wcslen(input);
    parser->line = 1;
    parser->column = 1;
    parser->errorMessage[0] = L'\0';
}

static WCHAR CurrentChar(JsonParser* parser) {
    if (parser->pos >= parser->length) return L'\0';
    return parser->input[parser->pos];
}

static WCHAR NextChar(JsonParser* parser) {
    if (parser->pos >= parser->length) return L'\0';
    WCHAR ch = parser->input[parser->pos++];
    if (ch == L'\n') {
        parser->line++;
        parser->column = 1;
    } else {
        parser->column++;
    }
    return ch;
}

static void SkipWhitespace(JsonParser* parser) {
    while (parser->pos < parser->length) {
        WCHAR ch = CurrentChar(parser);
        if (ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\r') {
            NextChar(parser);
        } else {
            break;
        }
    }
}

static void SetError(JsonParser* parser, const WCHAR* message) {
    swprintf(parser->errorMessage, 256, L"%s at line %d, column %d", 
             message, parser->line, parser->column);
}


/* ============== Parser Implementation ============== */

static WCHAR* ParseString(JsonParser* parser) {
    if (CurrentChar(parser) != L'"') {
        SetError(parser, L"Expected '\"'");
        return NULL;
    }
    NextChar(parser); /* Skip opening quote */
    
    StringBuilder sb;
    if (!StringBuilder_Init(&sb, 64)) {
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    while (parser->pos < parser->length) {
        WCHAR ch = CurrentChar(parser);
        
        if (ch == L'"') {
            NextChar(parser); /* Skip closing quote */
            WCHAR* result = sb.buffer;
            sb.buffer = NULL; /* Transfer ownership */
            return result;
        }
        
        if (ch == L'\\') {
            NextChar(parser); /* Skip backslash */
            ch = CurrentChar(parser);
            switch (ch) {
                case L'"':  StringBuilder_AppendChar(&sb, L'"'); break;
                case L'\\': StringBuilder_AppendChar(&sb, L'\\'); break;
                case L'/':  StringBuilder_AppendChar(&sb, L'/'); break;
                case L'b':  StringBuilder_AppendChar(&sb, L'\b'); break;
                case L'f':  StringBuilder_AppendChar(&sb, L'\f'); break;
                case L'n':  StringBuilder_AppendChar(&sb, L'\n'); break;
                case L'r':  StringBuilder_AppendChar(&sb, L'\r'); break;
                case L't':  StringBuilder_AppendChar(&sb, L'\t'); break;
                case L'u': {
                    /* Parse 4 hex digits */
                    NextChar(parser);
                    WCHAR hex[5] = {0};
                    for (int i = 0; i < 4; i++) {
                        hex[i] = CurrentChar(parser);
                        if (!((hex[i] >= L'0' && hex[i] <= L'9') ||
                              (hex[i] >= L'a' && hex[i] <= L'f') ||
                              (hex[i] >= L'A' && hex[i] <= L'F'))) {
                            SetError(parser, L"Invalid Unicode escape");
                            StringBuilder_Free(&sb);
                            return NULL;
                        }
                        if (i < 3) NextChar(parser);
                    }
                    unsigned int codepoint = (unsigned int)wcstoul(hex, NULL, 16);
                    StringBuilder_AppendChar(&sb, (WCHAR)codepoint);
                    break;
                }
                default:
                    SetError(parser, L"Invalid escape sequence");
                    StringBuilder_Free(&sb);
                    return NULL;
            }
            NextChar(parser);
        } else if (ch == L'\0' || ch == L'\n') {
            SetError(parser, L"Unterminated string");
            StringBuilder_Free(&sb);
            return NULL;
        } else {
            StringBuilder_AppendChar(&sb, ch);
            NextChar(parser);
        }
    }
    
    SetError(parser, L"Unterminated string");
    StringBuilder_Free(&sb);
    return NULL;
}

static JsonValue* ParseNumber(JsonParser* parser) {
    int startPos = parser->pos;
    BOOL hasDecimal = FALSE;
    BOOL hasExponent = FALSE;
    
    /* Optional minus */
    if (CurrentChar(parser) == L'-') {
        NextChar(parser);
    }
    
    /* Integer part */
    if (CurrentChar(parser) == L'0') {
        NextChar(parser);
    } else if (CurrentChar(parser) >= L'1' && CurrentChar(parser) <= L'9') {
        while (CurrentChar(parser) >= L'0' && CurrentChar(parser) <= L'9') {
            NextChar(parser);
        }
    } else {
        SetError(parser, L"Invalid number");
        return NULL;
    }
    
    /* Decimal part */
    if (CurrentChar(parser) == L'.') {
        hasDecimal = TRUE;
        NextChar(parser);
        if (!(CurrentChar(parser) >= L'0' && CurrentChar(parser) <= L'9')) {
            SetError(parser, L"Invalid number: expected digit after decimal");
            return NULL;
        }
        while (CurrentChar(parser) >= L'0' && CurrentChar(parser) <= L'9') {
            NextChar(parser);
        }
    }
    
    /* Exponent part */
    if (CurrentChar(parser) == L'e' || CurrentChar(parser) == L'E') {
        hasExponent = TRUE;
        NextChar(parser);
        if (CurrentChar(parser) == L'+' || CurrentChar(parser) == L'-') {
            NextChar(parser);
        }
        if (!(CurrentChar(parser) >= L'0' && CurrentChar(parser) <= L'9')) {
            SetError(parser, L"Invalid number: expected digit in exponent");
            return NULL;
        }
        while (CurrentChar(parser) >= L'0' && CurrentChar(parser) <= L'9') {
            NextChar(parser);
        }
    }
    
    /* Extract number string and convert */
    int len = parser->pos - startPos;
    WCHAR* numStr = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!numStr) {
        SetError(parser, L"Out of memory");
        return NULL;
    }
    wcsncpy(numStr, parser->input + startPos, len);
    numStr[len] = L'\0';
    
    JsonValue* value = CreateValue(JSON_NUMBER);
    if (!value) {
        HeapFree(GetProcessHeap(), 0, numStr);
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    value->data.numberValue = _wtof(numStr);
    HeapFree(GetProcessHeap(), 0, numStr);
    
    (void)hasDecimal;
    (void)hasExponent;
    
    return value;
}

static JsonValue* ParseArray(JsonParser* parser) {
    if (CurrentChar(parser) != L'[') {
        SetError(parser, L"Expected '['");
        return NULL;
    }
    NextChar(parser); /* Skip '[' */
    
    JsonValue* array = CreateValue(JSON_ARRAY);
    if (!array) {
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    array->data.array.capacity = 8;
    array->data.array.items = (JsonValue**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                      array->data.array.capacity * sizeof(JsonValue*));
    if (!array->data.array.items) {
        JsonFreeValue(array);
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    SkipWhitespace(parser);
    
    if (CurrentChar(parser) == L']') {
        NextChar(parser);
        return array;
    }
    
    while (1) {
        SkipWhitespace(parser);
        JsonValue* item = ParseValue(parser);
        if (!item) {
            JsonFreeValue(array);
            return NULL;
        }
        
        /* Add to array */
        if (array->data.array.count >= array->data.array.capacity) {
            int newCapacity = array->data.array.capacity * 2;
            JsonValue** newItems = (JsonValue**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                             array->data.array.items,
                                                             newCapacity * sizeof(JsonValue*));
            if (!newItems) {
                JsonFreeValue(item);
                JsonFreeValue(array);
                SetError(parser, L"Out of memory");
                return NULL;
            }
            array->data.array.items = newItems;
            array->data.array.capacity = newCapacity;
        }
        array->data.array.items[array->data.array.count++] = item;
        
        SkipWhitespace(parser);
        
        if (CurrentChar(parser) == L']') {
            NextChar(parser);
            return array;
        }
        
        if (CurrentChar(parser) != L',') {
            SetError(parser, L"Expected ',' or ']'");
            JsonFreeValue(array);
            return NULL;
        }
        NextChar(parser); /* Skip ',' */
    }
}

static JsonValue* ParseObject(JsonParser* parser) {
    if (CurrentChar(parser) != L'{') {
        SetError(parser, L"Expected '{'");
        return NULL;
    }
    NextChar(parser); /* Skip '{' */
    
    JsonValue* obj = CreateValue(JSON_OBJECT);
    if (!obj) {
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    obj->data.object.capacity = 8;
    obj->data.object.keys = (WCHAR**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                obj->data.object.capacity * sizeof(WCHAR*));
    obj->data.object.values = (JsonValue**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                      obj->data.object.capacity * sizeof(JsonValue*));
    if (!obj->data.object.keys || !obj->data.object.values) {
        JsonFreeValue(obj);
        SetError(parser, L"Out of memory");
        return NULL;
    }
    
    SkipWhitespace(parser);
    
    if (CurrentChar(parser) == L'}') {
        NextChar(parser);
        return obj;
    }
    
    while (1) {
        SkipWhitespace(parser);
        
        if (CurrentChar(parser) != L'"') {
            SetError(parser, L"Expected string key");
            JsonFreeValue(obj);
            return NULL;
        }
        
        WCHAR* key = ParseString(parser);
        if (!key) {
            JsonFreeValue(obj);
            return NULL;
        }
        
        SkipWhitespace(parser);
        
        if (CurrentChar(parser) != L':') {
            HeapFree(GetProcessHeap(), 0, key);
            SetError(parser, L"Expected ':'");
            JsonFreeValue(obj);
            return NULL;
        }
        NextChar(parser); /* Skip ':' */
        
        SkipWhitespace(parser);
        JsonValue* value = ParseValue(parser);
        if (!value) {
            HeapFree(GetProcessHeap(), 0, key);
            JsonFreeValue(obj);
            return NULL;
        }
        
        /* Add to object */
        if (obj->data.object.count >= obj->data.object.capacity) {
            int newCapacity = obj->data.object.capacity * 2;
            WCHAR** newKeys = (WCHAR**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                    obj->data.object.keys,
                                                    newCapacity * sizeof(WCHAR*));
            JsonValue** newValues = (JsonValue**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                              obj->data.object.values,
                                                              newCapacity * sizeof(JsonValue*));
            if (!newKeys || !newValues) {
                HeapFree(GetProcessHeap(), 0, key);
                JsonFreeValue(value);
                JsonFreeValue(obj);
                SetError(parser, L"Out of memory");
                return NULL;
            }
            obj->data.object.keys = newKeys;
            obj->data.object.values = newValues;
            obj->data.object.capacity = newCapacity;
        }
        obj->data.object.keys[obj->data.object.count] = key;
        obj->data.object.values[obj->data.object.count] = value;
        obj->data.object.count++;
        
        SkipWhitespace(parser);
        
        if (CurrentChar(parser) == L'}') {
            NextChar(parser);
            return obj;
        }
        
        if (CurrentChar(parser) != L',') {
            SetError(parser, L"Expected ',' or '}'");
            JsonFreeValue(obj);
            return NULL;
        }
        NextChar(parser); /* Skip ',' */
    }
}

static JsonValue* ParseValue(JsonParser* parser) {
    SkipWhitespace(parser);
    
    WCHAR ch = CurrentChar(parser);
    
    if (ch == L'"') {
        WCHAR* str = ParseString(parser);
        if (!str) return NULL;
        JsonValue* value = CreateValue(JSON_STRING);
        if (!value) {
            HeapFree(GetProcessHeap(), 0, str);
            SetError(parser, L"Out of memory");
            return NULL;
        }
        value->data.stringValue = str;
        return value;
    }
    
    if (ch == L'{') {
        return ParseObject(parser);
    }
    
    if (ch == L'[') {
        return ParseArray(parser);
    }
    
    if (ch == L'-' || (ch >= L'0' && ch <= L'9')) {
        return ParseNumber(parser);
    }
    
    /* Check for keywords */
    if (wcsncmp(parser->input + parser->pos, L"true", 4) == 0) {
        parser->pos += 4;
        parser->column += 4;
        JsonValue* value = CreateValue(JSON_BOOL);
        if (value) value->data.boolValue = TRUE;
        return value;
    }
    
    if (wcsncmp(parser->input + parser->pos, L"false", 5) == 0) {
        parser->pos += 5;
        parser->column += 5;
        JsonValue* value = CreateValue(JSON_BOOL);
        if (value) value->data.boolValue = FALSE;
        return value;
    }
    
    if (wcsncmp(parser->input + parser->pos, L"null", 4) == 0) {
        parser->pos += 4;
        parser->column += 4;
        return CreateValue(JSON_NULL);
    }
    
    SetError(parser, L"Unexpected character");
    return NULL;
}


/* ============== Public Parse API ============== */

JsonParseResult* JsonParse(const WCHAR* input) {
    JsonParseResult* result = (JsonParseResult*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                                          sizeof(JsonParseResult));
    if (!result) return NULL;
    
    JsonParser parser;
    InitParser(&parser, input);
    
    result->root = ParseValue(&parser);
    
    if (result->root) {
        SkipWhitespace(&parser);
        if (CurrentChar(&parser) != L'\0') {
            /* Extra content after JSON value */
            SetError(&parser, L"Unexpected content after JSON");
            JsonFreeValue(result->root);
            result->root = NULL;
            result->success = FALSE;
            result->errorLine = parser.line;
            result->errorColumn = parser.column;
            wcscpy(result->errorMessage, parser.errorMessage);
        } else {
            result->success = TRUE;
        }
    } else {
        result->success = FALSE;
        result->errorLine = parser.line;
        result->errorColumn = parser.column;
        wcscpy(result->errorMessage, parser.errorMessage);
    }
    
    return result;
}

/* ============== Formatter Implementation ============== */

static void AppendEscapedString(StringBuilder* sb, const WCHAR* str) {
    StringBuilder_AppendChar(sb, L'"');
    
    while (*str) {
        WCHAR ch = *str++;
        switch (ch) {
            case L'"':  StringBuilder_Append(sb, L"\\\""); break;
            case L'\\': StringBuilder_Append(sb, L"\\\\"); break;
            case L'\b': StringBuilder_Append(sb, L"\\b"); break;
            case L'\f': StringBuilder_Append(sb, L"\\f"); break;
            case L'\n': StringBuilder_Append(sb, L"\\n"); break;
            case L'\r': StringBuilder_Append(sb, L"\\r"); break;
            case L'\t': StringBuilder_Append(sb, L"\\t"); break;
            default:
                if (ch < 0x20) {
                    WCHAR buf[8];
                    swprintf(buf, 8, L"\\u%04x", (unsigned int)ch);
                    StringBuilder_Append(sb, buf);
                } else {
                    StringBuilder_AppendChar(sb, ch);
                }
                break;
        }
    }
    
    StringBuilder_AppendChar(sb, L'"');
}

static void FormatValue(StringBuilder* sb, JsonValue* value, int indent, int indentSpaces) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_NULL:
            StringBuilder_Append(sb, L"null");
            break;
            
        case JSON_BOOL:
            StringBuilder_Append(sb, value->data.boolValue ? L"true" : L"false");
            break;
            
        case JSON_NUMBER: {
            WCHAR buf[64];
            double num = value->data.numberValue;
            /* Check if it's an integer */
            if (num == floor(num) && fabs(num) < 1e15) {
                swprintf(buf, 64, L"%.0f", num);
            } else {
                swprintf(buf, 64, L"%.15g", num);
            }
            StringBuilder_Append(sb, buf);
            break;
        }
            
        case JSON_STRING:
            AppendEscapedString(sb, value->data.stringValue);
            break;
            
        case JSON_ARRAY:
            if (value->data.array.count == 0) {
                StringBuilder_Append(sb, L"[]");
            } else {
                StringBuilder_Append(sb, L"[\r\n");
                for (int i = 0; i < value->data.array.count; i++) {
                    StringBuilder_AppendIndent(sb, indent + indentSpaces);
                    FormatValue(sb, value->data.array.items[i], indent + indentSpaces, indentSpaces);
                    if (i < value->data.array.count - 1) {
                        StringBuilder_AppendChar(sb, L',');
                    }
                    StringBuilder_Append(sb, L"\r\n");
                }
                StringBuilder_AppendIndent(sb, indent);
                StringBuilder_AppendChar(sb, L']');
            }
            break;
            
        case JSON_OBJECT:
            if (value->data.object.count == 0) {
                StringBuilder_Append(sb, L"{}");
            } else {
                StringBuilder_Append(sb, L"{\r\n");
                for (int i = 0; i < value->data.object.count; i++) {
                    StringBuilder_AppendIndent(sb, indent + indentSpaces);
                    AppendEscapedString(sb, value->data.object.keys[i]);
                    StringBuilder_Append(sb, L": ");
                    FormatValue(sb, value->data.object.values[i], indent + indentSpaces, indentSpaces);
                    if (i < value->data.object.count - 1) {
                        StringBuilder_AppendChar(sb, L',');
                    }
                    StringBuilder_Append(sb, L"\r\n");
                }
                StringBuilder_AppendIndent(sb, indent);
                StringBuilder_AppendChar(sb, L'}');
            }
            break;
    }
}

WCHAR* JsonFormat(JsonValue* value, int indentSpaces) {
    if (!value) return NULL;
    
    StringBuilder sb;
    if (!StringBuilder_Init(&sb, 1024)) return NULL;
    
    FormatValue(&sb, value, 0, indentSpaces);
    
    return sb.buffer; /* Caller must free */
}

/* ============== Minifier Implementation ============== */

static void MinifyValue(StringBuilder* sb, JsonValue* value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_NULL:
            StringBuilder_Append(sb, L"null");
            break;
            
        case JSON_BOOL:
            StringBuilder_Append(sb, value->data.boolValue ? L"true" : L"false");
            break;
            
        case JSON_NUMBER: {
            WCHAR buf[64];
            double num = value->data.numberValue;
            if (num == floor(num) && fabs(num) < 1e15) {
                swprintf(buf, 64, L"%.0f", num);
            } else {
                swprintf(buf, 64, L"%.15g", num);
            }
            StringBuilder_Append(sb, buf);
            break;
        }
            
        case JSON_STRING:
            AppendEscapedString(sb, value->data.stringValue);
            break;
            
        case JSON_ARRAY:
            StringBuilder_AppendChar(sb, L'[');
            for (int i = 0; i < value->data.array.count; i++) {
                if (i > 0) StringBuilder_AppendChar(sb, L',');
                MinifyValue(sb, value->data.array.items[i]);
            }
            StringBuilder_AppendChar(sb, L']');
            break;
            
        case JSON_OBJECT:
            StringBuilder_AppendChar(sb, L'{');
            for (int i = 0; i < value->data.object.count; i++) {
                if (i > 0) StringBuilder_AppendChar(sb, L',');
                AppendEscapedString(sb, value->data.object.keys[i]);
                StringBuilder_AppendChar(sb, L':');
                MinifyValue(sb, value->data.object.values[i]);
            }
            StringBuilder_AppendChar(sb, L'}');
            break;
    }
}

WCHAR* JsonMinify(JsonValue* value) {
    if (!value) return NULL;
    
    StringBuilder sb;
    if (!StringBuilder_Init(&sb, 1024)) return NULL;
    
    MinifyValue(&sb, value);
    
    return sb.buffer; /* Caller must free */
}

/* ============== Convenience Functions ============== */

WCHAR* JsonPrettyPrint(const WCHAR* input, int indentSpaces, BOOL* success, WCHAR* errorMsg, int errorMsgSize) {
    JsonParseResult* result = JsonParse(input);
    if (!result) {
        if (success) *success = FALSE;
        if (errorMsg) wcscpy_s(errorMsg, errorMsgSize, L"Out of memory");
        return NULL;
    }
    
    if (!result->success) {
        if (success) *success = FALSE;
        if (errorMsg) wcscpy_s(errorMsg, errorMsgSize, result->errorMessage);
        JsonFreeParseResult(result);
        return NULL;
    }
    
    WCHAR* formatted = JsonFormat(result->root, indentSpaces);
    JsonFreeParseResult(result);
    
    if (success) *success = (formatted != NULL);
    if (!formatted && errorMsg) {
        wcscpy_s(errorMsg, errorMsgSize, L"Out of memory during formatting");
    }
    
    return formatted;
}

WCHAR* JsonMinifyString(const WCHAR* input, BOOL* success, WCHAR* errorMsg, int errorMsgSize) {
    JsonParseResult* result = JsonParse(input);
    if (!result) {
        if (success) *success = FALSE;
        if (errorMsg) wcscpy_s(errorMsg, errorMsgSize, L"Out of memory");
        return NULL;
    }
    
    if (!result->success) {
        if (success) *success = FALSE;
        if (errorMsg) wcscpy_s(errorMsg, errorMsgSize, result->errorMessage);
        JsonFreeParseResult(result);
        return NULL;
    }
    
    WCHAR* minified = JsonMinify(result->root);
    JsonFreeParseResult(result);
    
    if (success) *success = (minified != NULL);
    if (!minified && errorMsg) {
        wcscpy_s(errorMsg, errorMsgSize, L"Out of memory during minifying");
    }
    
    return minified;
}

/* ============== Editor Integration ============== */

void FormatJsonInEditor(HWND hwndEdit) {
    if (!hwndEdit) return;
    
    /* Get text length */
    int len = GetWindowTextLengthW(hwndEdit);
    if (len == 0) {
        MessageBoxW(GetParent(hwndEdit), L"Document is empty", L"Format JSON", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    /* Allocate buffer and get text */
    WCHAR* text = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!text) {
        MessageBoxW(GetParent(hwndEdit), L"Out of memory", L"Format JSON", MB_OK | MB_ICONERROR);
        return;
    }
    
    GetWindowTextW(hwndEdit, text, len + 1);
    
    /* Format JSON */
    BOOL success;
    WCHAR errorMsg[256];
    WCHAR* formatted = JsonPrettyPrint(text, 4, &success, errorMsg, 256);
    
    HeapFree(GetProcessHeap(), 0, text);
    
    if (!success) {
        MessageBoxW(GetParent(hwndEdit), errorMsg, L"JSON Parse Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    /* Update editor content */
    SetWindowTextW(hwndEdit, formatted);
    HeapFree(GetProcessHeap(), 0, formatted);
    
    /* Mark document as modified */
    TabState* pTab = GetCurrentTabState();
    if (pTab) {
        pTab->bModified = TRUE;
        UpdateTabTitle(g_AppState.nCurrentTab);
        UpdateWindowTitle(g_AppState.hwndMain);
    }
}

void MinifyJsonInEditor(HWND hwndEdit) {
    if (!hwndEdit) return;
    
    /* Get text length */
    int len = GetWindowTextLengthW(hwndEdit);
    if (len == 0) {
        MessageBoxW(GetParent(hwndEdit), L"Document is empty", L"Minify JSON", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    /* Allocate buffer and get text */
    WCHAR* text = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!text) {
        MessageBoxW(GetParent(hwndEdit), L"Out of memory", L"Minify JSON", MB_OK | MB_ICONERROR);
        return;
    }
    
    GetWindowTextW(hwndEdit, text, len + 1);
    
    /* Minify JSON */
    BOOL success;
    WCHAR errorMsg[256];
    WCHAR* minified = JsonMinifyString(text, &success, errorMsg, 256);
    
    HeapFree(GetProcessHeap(), 0, text);
    
    if (!success) {
        MessageBoxW(GetParent(hwndEdit), errorMsg, L"JSON Parse Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    /* Update editor content */
    SetWindowTextW(hwndEdit, minified);
    HeapFree(GetProcessHeap(), 0, minified);
    
    /* Mark document as modified */
    TabState* pTab = GetCurrentTabState();
    if (pTab) {
        pTab->bModified = TRUE;
        UpdateTabTitle(g_AppState.nCurrentTab);
        UpdateWindowTitle(g_AppState.hwndMain);
    }
}
