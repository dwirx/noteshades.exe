#ifndef JSON_FORMAT_H
#define JSON_FORMAT_H

#include <windows.h>

/* JSON value types */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonValueType;

/* Forward declaration */
typedef struct JsonValue JsonValue;

/* JSON value structure */
struct JsonValue {
    JsonValueType type;
    union {
        BOOL boolValue;
        double numberValue;
        WCHAR* stringValue;
        struct {
            JsonValue** items;
            int count;
            int capacity;
        } array;
        struct {
            WCHAR** keys;
            JsonValue** values;
            int count;
            int capacity;
        } object;
    } data;
};

/* Parse result */
typedef struct {
    BOOL success;
    JsonValue* root;
    int errorLine;
    int errorColumn;
    WCHAR errorMessage[256];
} JsonParseResult;

/* Public API - Core functions */
JsonParseResult* JsonParse(const WCHAR* input);
WCHAR* JsonFormat(JsonValue* value, int indentSpaces);
WCHAR* JsonMinify(JsonValue* value);
void JsonFreeValue(JsonValue* value);
void JsonFreeParseResult(JsonParseResult* result);

/* Public API - Convenience functions */
WCHAR* JsonPrettyPrint(const WCHAR* input, int indentSpaces, BOOL* success, WCHAR* errorMsg, int errorMsgSize);
WCHAR* JsonMinifyString(const WCHAR* input, BOOL* success, WCHAR* errorMsg, int errorMsgSize);

/* Editor integration functions */
void FormatJsonInEditor(HWND hwndEdit);
void MinifyJsonInEditor(HWND hwndEdit);

#endif /* JSON_FORMAT_H */
