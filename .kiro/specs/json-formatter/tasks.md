# Implementation Plan

- [x] 1. Create JSON parser core module




  - [ ] 1.1 Create json_format.h with type definitions and function declarations
    - Define JsonValueType enum, JsonValue struct, JsonParseResult struct


    - Declare public API functions
    - _Requirements: 1.1, 1.2, 1.3_
  - [x] 1.2 Implement JSON lexer/tokenizer in json_format.c

    - Implement character scanning and token recognition
    - Handle whitespace, strings, numbers, keywords (true, false, null)
    - Track line and column for error reporting
    - _Requirements: 1.2_
  - [ ] 1.3 Implement JSON parser
    - Parse objects, arrays, strings, numbers, booleans, null
    - Handle nested structures recursively




    - Return parse errors with line/column information
    - _Requirements: 1.1, 1.2, 1.3_
  - [x]* 1.4 Write property test for round-trip consistency

    - **Property 1: Round-trip consistency**
    - **Validates: Requirements 1.3, 3.3, 4.1, 4.2, 4.3**

- [ ] 2. Implement JSON formatter (pretty print)
  - [ ] 2.1 Implement JsonFormat function
    - Format with 4-space indentation



    - Handle all JSON value types
    - Proper newlines and spacing
    - _Requirements: 1.1, 4.4_

  - [ ] 2.2 Implement JsonPrettyPrint convenience function
    - Parse input, format, return formatted string
    - Handle errors gracefully
    - _Requirements: 1.1, 1.2_
  - [ ]* 2.3 Write property test for indentation correctness
    - **Property 2: Indentation correctness**




    - **Validates: Requirements 1.1, 4.4**

- [ ] 3. Implement JSON minifier
  - [ ] 3.1 Implement JsonMinify function
    - Remove all unnecessary whitespace

    - Preserve string contents
    - _Requirements: 3.1, 3.3_
  - [x] 3.2 Implement JsonMinifyString convenience function




    - Parse input, minify, return minified string
    - Handle errors gracefully


    - _Requirements: 3.1, 3.2_
  - [ ]* 3.3 Write property test for minify whitespace removal
    - **Property 4: Minify removes all unnecessary whitespace**

    - **Validates: Requirements 3.1**

- [ ] 4. Implement error handling
  - [x] 4.1 Implement detailed error messages




    - Include line and column numbers
    - Descriptive error messages for each error type






    - _Requirements: 1.2, 3.2_
  - [ ]* 4.2 Write property test for invalid JSON error detection
    - **Property 3: Invalid JSON error detection**
    - **Validates: Requirements 1.2**

- [ ] 5. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 6. Integrate with XNote UI
  - [ ] 6.1 Add menu items to resource.h and notepad.rc
    - Add IDM_FORMAT_JSON, IDM_MINIFY_JSON, IDM_AUTO_FORMAT_JSON
    - Add menu items under Edit menu
    - _Requirements: 1.1, 3.1, 2.1_
  - [ ] 6.2 Add settings for auto-format JSON
    - Add g_bAutoFormatJson global variable
    - Implement LoadJsonFormatterSettings and SaveJsonFormatterSettings
    - Store in Windows Registry
    - _Requirements: 2.2, 2.3, 2.4_
  - [ ] 6.3 Handle menu commands in WndProc
    - Implement IDM_FORMAT_JSON handler
    - Implement IDM_MINIFY_JSON handler
    - Implement IDM_AUTO_FORMAT_JSON toggle handler
    - _Requirements: 1.1, 1.4, 3.1_

- [ ] 7. Integrate auto-format on save
  - [ ] 7.1 Modify FileSave to check for auto-format
    - Check if file is .json and auto-format is enabled
    - Format before saving
    - _Requirements: 2.1, 2.2_

- [ ] 8. Update build system
  - [ ] 8.1 Add json_format.c to Makefile and build.bat
    - Add compilation rules for new source file
    - _Requirements: All_

- [ ] 9. Final Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
