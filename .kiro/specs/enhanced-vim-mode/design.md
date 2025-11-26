# Design Document: Enhanced Vim Mode

## Overview

Dokumen ini menjelaskan desain teknis untuk peningkatan fitur Vim Mode pada XNote text editor. Peningkatan mencakup perbaikan bug, optimasi performa, dan penambahan fitur Vim yang lebih lengkap termasuk text objects, marks, dot repeat, dan ex commands tambahan.

## Architecture

### Current Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        main.c                                │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              EditSubclassProc                        │    │
│  │  - Intercepts WM_KEYDOWN, WM_CHAR                   │    │
│  │  - Calls ProcessVimKey() if vim enabled             │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      vim_mode.c                              │
│  ┌─────────────────┐  ┌─────────────────┐                   │
│  │   VimState      │  │  ProcessVimKey  │                   │
│  │  - mode         │  │  - Normal mode  │                   │
│  │  - repeatCount  │  │  - Insert mode  │                   │
│  │  - pendingOp    │  │  - Visual mode  │                   │
│  │  - register     │  │  - Command mode │                   │
│  └─────────────────┘  └─────────────────┘                   │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Motion Functions                        │    │
│  │  VimMoveLeft, VimMoveRight, VimMoveUp, VimMoveDown  │    │
│  │  VimMoveWordForward, VimMoveWordBackward, etc.      │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Edit Functions                          │    │
│  │  VimDeleteChar, VimDeleteLine, VimYankLine, etc.    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### Enhanced Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      vim_mode.c                              │
│                                                              │
│  ┌─────────────────┐  ┌─────────────────┐                   │
│  │   VimState      │  │  NEW: MarkState │                   │
│  │  + lastChange   │  │  - marks[26]    │                   │
│  │  + jumpList     │  │  - positions    │                   │
│  │  + jumpIndex    │  │                 │                   │
│  └─────────────────┘  └─────────────────┘                   │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           NEW: Text Object Functions                 │    │
│  │  VimSelectInnerWord, VimSelectAWord                 │    │
│  │  VimSelectInnerQuote, VimSelectAQuote               │    │
│  │  VimSelectInnerBracket, VimSelectABracket           │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           NEW: Additional Motions                    │    │
│  │  VimMoveWORDForward, VimMoveWORDBackward            │    │
│  │  VimMoveScreenTop, VimMoveScreenMiddle, etc.        │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           NEW: Dot Repeat System                     │    │
│  │  RecordChange, RepeatLastChange                     │    │
│  │  ChangeRecord structure                             │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           NEW: Substitute Command                    │    │
│  │  VimSubstitute, ParseSubstituteCommand              │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. Enhanced VimState Structure

```c
/* Mark storage - 26 local marks (a-z) */
typedef struct {
    DWORD dwPosition;       /* Character position */
    int nLine;              /* Line number */
    BOOL bSet;              /* Mark is set */
} VimMark;

/* Jump list entry */
typedef struct {
    DWORD dwPosition;       /* Character position */
    int nLine;              /* Line number */
} JumpEntry;

#define MAX_JUMP_LIST 100

/* Change record for dot repeat */
typedef struct {
    TCHAR chCommand;        /* Main command (d, c, r, etc.) */
    TCHAR chMotion;         /* Motion or text object */
    TCHAR chExtra;          /* Extra character (for r, f, t) */
    int nCount;             /* Repeat count */
    TCHAR szInsertText[4096]; /* Text inserted (for insert mode) */
    int nInsertLen;         /* Length of inserted text */
    BOOL bValid;            /* Record is valid */
} ChangeRecord;

/* Extended VimState */
typedef struct {
    /* Existing fields... */
    
    /* NEW: Marks */
    VimMark marks[26];      /* Local marks a-z */
    
    /* NEW: Jump list */
    JumpEntry jumpList[MAX_JUMP_LIST];
    int nJumpCount;
    int nJumpIndex;
    DWORD dwLastJumpPos;    /* Position before last jump */
    
    /* NEW: Dot repeat */
    ChangeRecord lastChange;
    BOOL bRecording;        /* Currently recording a change */
    
    /* NEW: Replace mode */
    BOOL bReplaceMode;      /* In replace mode (R) */
    
    /* NEW: Pending text object */
    TCHAR chPendingTextObj; /* Pending text object (i or a) */
    
    /* NEW: Cached text buffer for performance */
    TCHAR* pCachedBuffer;
    DWORD dwCachedLen;
    DWORD dwCacheTime;      /* Timestamp for cache invalidation */
} VimState;
```

### 2. Text Object Interface

```c
/* Text object selection result */
typedef struct {
    DWORD dwStart;          /* Selection start */
    DWORD dwEnd;            /* Selection end */
    BOOL bFound;            /* Object was found */
} TextObjectRange;

/* Text object functions */
TextObjectRange VimSelectInnerWord(HWND hwndEdit);
TextObjectRange VimSelectAWord(HWND hwndEdit);
TextObjectRange VimSelectInnerQuote(HWND hwndEdit, TCHAR chQuote);
TextObjectRange VimSelectAQuote(HWND hwndEdit, TCHAR chQuote);
TextObjectRange VimSelectInnerBracket(HWND hwndEdit, TCHAR chOpen, TCHAR chClose);
TextObjectRange VimSelectABracket(HWND hwndEdit, TCHAR chOpen, TCHAR chClose);
```

### 3. Additional Motion Interface

```c
/* WORD motions (whitespace-delimited) */
void VimMoveWORDForward(HWND hwndEdit, int count);
void VimMoveWORDBackward(HWND hwndEdit, int count);
void VimMoveWORDEnd(HWND hwndEdit, int count);
void VimMoveWordEndBackward(HWND hwndEdit, int count);  /* ge */
void VimMoveWORDEndBackward(HWND hwndEdit, int count);  /* gE */

/* Screen position motions */
void VimMoveScreenTop(HWND hwndEdit);      /* H */
void VimMoveScreenMiddle(HWND hwndEdit);   /* M */
void VimMoveScreenBottom(HWND hwndEdit);   /* L */

/* Line motions */
void VimMoveNextLineFirstNonBlank(HWND hwndEdit, int count);  /* + */
void VimMovePrevLineFirstNonBlank(HWND hwndEdit, int count);  /* - */
```

### 4. Mark and Jump Interface

```c
/* Mark operations */
void VimSetMark(HWND hwndEdit, TCHAR chMark);
void VimJumpToMark(HWND hwndEdit, TCHAR chMark, BOOL bExact);
void VimJumpToLastPosition(HWND hwndEdit, BOOL bExact);

/* Jump list operations */
void VimAddJump(HWND hwndEdit);
void VimJumpOlder(HWND hwndEdit);   /* Ctrl-O */
void VimJumpNewer(HWND hwndEdit);   /* Ctrl-I */
```

### 5. Dot Repeat Interface

```c
/* Change recording */
void VimStartRecording(TCHAR chCommand, int nCount);
void VimRecordMotion(TCHAR chMotion);
void VimRecordInsertChar(TCHAR ch);
void VimEndRecording(void);

/* Repeat */
void VimRepeatLastChange(HWND hwndEdit, int nNewCount);
```

### 6. Substitute Command Interface

```c
/* Substitute command parsing */
typedef struct {
    TCHAR szPattern[256];
    TCHAR szReplacement[256];
    BOOL bGlobal;           /* /g flag */
    BOOL bConfirm;          /* /c flag */
    BOOL bWholeFile;        /* % range */
    int nStartLine;
    int nEndLine;
} SubstituteCmd;

BOOL ParseSubstituteCommand(const TCHAR* szCmd, SubstituteCmd* pCmd);
int VimSubstitute(HWND hwndEdit, const SubstituteCmd* pCmd);
```

## Data Models

### Mark Storage

Marks are stored as an array of 26 entries (a-z) in the VimState structure. Each mark contains:
- Character position (DWORD)
- Line number (int) for line-based jumps
- Valid flag (BOOL)

### Jump List

The jump list is a circular buffer of MAX_JUMP_LIST entries. Operations that cause "jumps" (searches, marks, gg, G, etc.) add entries to the list. Ctrl-O and Ctrl-I navigate through the list.

### Change Record

The change record stores the last text-changing operation for dot repeat:
- Command character (d, c, r, s, etc.)
- Motion or text object
- Count
- Inserted text (for insert mode operations)

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Find Character Correctness
*For any* line of text and any target character that exists on that line, when 'f' is pressed followed by that character, the cursor SHALL move to the first occurrence of that character after the current position on the same line.
**Validates: Requirements 1.2**

### Property 2: Till Character Offset
*For any* line of text and any target character, when 't' is pressed followed by that character, the final cursor position SHALL be exactly one character before the target character's position.
**Validates: Requirements 1.3**

### Property 3: Paragraph Motion Boundary
*For any* text with empty lines, when '}' is pressed, the cursor SHALL stop at the first empty line (or end of file) after the current position.
**Validates: Requirements 1.4**

### Property 4: Bracket Matching Correctness
*For any* text containing balanced brackets, when '%' is pressed on an opening bracket, the cursor SHALL move to the corresponding closing bracket at the same nesting level, and vice versa.
**Validates: Requirements 1.5**

### Property 5: Delete Line Register Round-Trip
*For any* line of text, after 'dd' is executed, the register SHALL contain the deleted line content, and pasting with 'p' SHALL restore that content.
**Validates: Requirements 1.6, 1.7**

### Property 6: WORD Motion Whitespace Boundary
*For any* text, when 'W' is pressed, the cursor SHALL move to the first character after the next whitespace sequence, treating any non-whitespace sequence as a single WORD.
**Validates: Requirements 3.1, 3.2, 3.3**

### Property 7: Line Motion First Non-Blank
*For any* text with indentation, when '+' is pressed, the cursor SHALL move to the first non-whitespace character of the next line.
**Validates: Requirements 3.9, 3.10**

### Property 8: Inner Word Selection
*For any* cursor position within a word, 'diw' SHALL delete exactly the word characters without surrounding whitespace.
**Validates: Requirements 4.1**

### Property 9: A Word Selection
*For any* cursor position within a word, 'daw' SHALL delete the word plus trailing whitespace (or leading if at end of line).
**Validates: Requirements 4.2**

### Property 10: Bracket Text Object Selection
*For any* text with balanced brackets (parentheses, braces, or square brackets), 'di{bracket}' SHALL select exactly the content between the innermost matching brackets, and 'da{bracket}' SHALL include the brackets themselves.
**Validates: Requirements 4.5, 4.6, 4.7, 4.8, 4.9, 4.10, 4.11, 4.12**

### Property 11: Quote Text Object Selection
*For any* text with quoted strings, 'di"' SHALL select exactly the content between the quotes, and 'da"' SHALL include the quote characters.
**Validates: Requirements 4.3, 4.4**

### Property 12: Replace Character Correctness
*For any* character at the cursor position, after 'r' followed by a new character, the character at that position SHALL be the new character and the cursor SHALL remain at the same position.
**Validates: Requirements 5.1**

### Property 13: Case Toggle Correctness
*For any* alphabetic character, after '~' is pressed, lowercase characters SHALL become uppercase and uppercase characters SHALL become lowercase.
**Validates: Requirements 5.9**

### Property 14: Case Conversion Correctness
*For any* text range, 'gU' followed by a motion SHALL convert all alphabetic characters in that range to uppercase, and 'gu' SHALL convert them to lowercase.
**Validates: Requirements 5.10, 5.11**

### Property 15: Mark Set and Jump Round-Trip
*For any* cursor position, after setting a mark with 'm{letter}' and moving elsewhere, jumping with '`{letter}' SHALL return the cursor to the exact saved position.
**Validates: Requirements 6.1, 6.2**

### Property 16: Mark Line Jump
*For any* marked position, jumping with "'{letter}" SHALL move the cursor to the first non-blank character of the marked line.
**Validates: Requirements 6.3**

### Property 17: Jump List Navigation
*For any* sequence of jumps, Ctrl-O SHALL move backward through the jump history, and Ctrl-I SHALL move forward, maintaining the correct order.
**Validates: Requirements 6.6, 6.7**

### Property 18: Dot Repeat Correctness
*For any* text-changing command, pressing '.' SHALL produce the same text change as the original command (with optional new count).
**Validates: Requirements 7.1, 7.2, 7.3, 7.4**

### Property 19: Substitute Single Occurrence
*For any* line containing a pattern, ':s/pattern/replacement/' SHALL replace only the first occurrence of the pattern on that line.
**Validates: Requirements 8.1**

### Property 20: Substitute Global
*For any* line containing multiple occurrences of a pattern, ':s/pattern/replacement/g' SHALL replace all occurrences on that line.
**Validates: Requirements 8.2**

### Property 21: Substitute Whole File
*For any* file containing a pattern, ':%s/pattern/replacement/g' SHALL replace all occurrences throughout the entire file.
**Validates: Requirements 8.3**

## Error Handling

### Invalid Operations
- Text objects on non-existent structures (e.g., 'di(' when not inside parentheses): No operation, cursor unchanged
- Marks that are not set: Display message "Mark not set"
- Jump list empty: No operation
- Substitute with invalid regex: Display error message

### Edge Cases
- Empty file: Most operations are no-ops
- Single character file: Handle boundary conditions
- Very long lines: Use efficient algorithms to avoid performance issues
- Nested brackets: Correctly match at the same nesting level

## Testing Strategy

### Dual Testing Approach

Testing akan menggunakan kombinasi unit tests dan property-based tests:

1. **Unit Tests**: Untuk kasus spesifik dan edge cases
2. **Property-Based Tests**: Untuk memverifikasi properti universal

### Property-Based Testing Framework

Karena ini adalah aplikasi C/Windows, kita akan menggunakan **theft** library untuk property-based testing (https://github.com/silentbicycle/theft).

Alternatif: Implementasi sederhana dengan random input generation untuk testing.

### Test Configuration
- Minimum 100 iterations per property test
- Random seed untuk reproducibility
- Shrinking untuk menemukan minimal failing case

### Test File Structure
```
tests/
  vim_mode_test.c       - Unit tests
  vim_mode_pbt.c        - Property-based tests
  test_helpers.c        - Test utilities
  test_main.c           - Test runner
```

### Property Test Annotations
Setiap property-based test HARUS di-tag dengan format:
```c
/* **Feature: enhanced-vim-mode, Property {number}: {property_text}** */
```

### Unit Test Coverage
- Mode transitions (Normal → Insert → Normal)
- Command parsing
- Edge cases (empty file, single char, etc.)
- Error conditions

### Property Test Coverage
- All 21 correctness properties defined above
- Each property tested with minimum 100 random inputs
