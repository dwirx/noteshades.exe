# Implementation Plan

- [x] 1. Bug Fixes and Core Improvements




  - [ ] 1.1 Fix VimPasteBefore function to paste at cursor without moving backward
    - Modify VimPasteBefore in vim_mode.c to insert at current position

    - Remove the pos-1 adjustment that causes incorrect behavior
    - _Requirements: 1.1_
  - [ ] 1.2 Fix find character commands (f/F/t/T)
    - Review VimFindCharForward and VimFindCharBackward implementations
    - Fix VimFindCharTillForward to stop one character before target
    - Fix VimFindCharTillBackward to stop one character after target
    - _Requirements: 1.2, 1.3_
  - [x]* 1.3 Write property test for find character correctness

    - **Property 1: Find Character Correctness**
    - **Property 2: Till Character Offset**
    - **Validates: Requirements 1.2, 1.3**
  - [ ] 1.4 Fix paragraph motion ('{' and '}')
    - Review VimMoveParagraphForward and VimMoveParagraphBackward
    - Fix empty line detection logic for paragraph boundaries

    - _Requirements: 1.4_
  - [ ]* 1.5 Write property test for paragraph motion
    - **Property 3: Paragraph Motion Boundary**
    - **Validates: Requirements 1.4**
  - [ ] 1.6 Fix bracket matching motion ('%')
    - Review VimMoveMatchingBracket implementation

    - Fix nested bracket handling to match at correct nesting level
    - _Requirements: 1.5_
  - [ ]* 1.7 Write property test for bracket matching
    - **Property 4: Bracket Matching Correctness**
    - **Validates: Requirements 1.5**
  - [ ] 1.8 Fix register handling for dd and yy
    - Implement proper register storage in VimDeleteLine
    - Implement proper register storage in VimYankLine
    - Add bRegisterIsLine flag handling for line-mode paste
    - _Requirements: 1.6, 1.7_
  - [ ]* 1.9 Write property test for delete/yank register round-trip
    - **Property 5: Delete Line Register Round-Trip**
    - **Validates: Requirements 1.6, 1.7**

- [ ] 2. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 3. Performance Optimizations
  - [ ] 3.1 Implement text buffer caching
    - Add pCachedBuffer, dwCachedLen, dwCacheTime to VimState
    - Modify GetTextBuffer to use cache when valid
    - Add cache invalidation on text changes
    - _Requirements: 2.4_
  - [ ] 3.2 Optimize word motion functions
    - Refactor VimMoveWordForward to reuse cached buffer
    - Refactor VimMoveWordBackward to reuse cached buffer
    - Refactor VimMoveWordEnd to reuse cached buffer

    - _Requirements: 2.5_

  - [ ] 3.3 Add efficient character classification
    - Create inline IsWordChar function
    - Create inline IsWORDChar function (whitespace check)
    - Avoid repeated IsCharAlphaNumeric calls
    - _Requirements: 2.5_

- [x] 4. Additional Vim Motions

  - [ ] 4.1 Implement WORD motions (W, B, E)
    - Add VimMoveWORDForward function (whitespace-delimited)
    - Add VimMoveWORDBackward function
    - Add VimMoveWORDEnd function

    - _Requirements: 3.1, 3.2, 3.3_
  - [ ]* 4.2 Write property test for WORD motions
    - **Property 6: WORD Motion Whitespace Boundary**
    - **Validates: Requirements 3.1, 3.2, 3.3**

  - [ ] 4.3 Implement ge and gE motions
    - Add VimMoveWordEndBackward function (ge)
    - Add VimMoveWORDEndBackward function (gE)
    - Add 'g' pending operator handling for ge/gE
    - _Requirements: 3.4, 3.5_
  - [ ] 4.4 Implement screen position motions (H, M, L)
    - Add VimMoveScreenTop function
    - Add VimMoveScreenMiddle function
    - Add VimMoveScreenBottom function
    - _Requirements: 3.6, 3.7, 3.8_



  - [ ] 4.5 Implement line motions (+, -, Enter)
    - Add VimMoveNextLineFirstNonBlank function
    - Add VimMovePrevLineFirstNonBlank function

    - Handle Enter key in Normal mode
    - _Requirements: 3.9, 3.10_
  - [ ]* 4.6 Write property test for line motions
    - **Property 7: Line Motion First Non-Blank**
    - **Validates: Requirements 3.9, 3.10**

- [ ] 5. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 6. Text Objects Implementation
  - [x] 6.1 Add TextObjectRange structure and helpers
    - Define TextObjectRange struct in vim_mode.h
    - Add FindMatchingBracket helper function
    - Add FindQuoteBoundary helper function
    - _Requirements: 4.1-4.12_
  - [x] 6.2 Implement word text objects (iw, aw)
    - Add VimSelectInnerWord function
    - Add VimSelectAWord function
    - _Requirements: 4.1, 4.2_
  - [ ]* 6.3 Write property test for word text objects
    - **Property 8: Inner Word Selection**
    - **Property 9: A Word Selection**
    - **Validates: Requirements 4.1, 4.2**
  - [x] 6.4 Implement quote text objects
    - Add VimSelectInnerQuote function with quote char parameter
    - Add VimSelectAQuote function with quote char parameter
    - _Requirements: 4.3, 4.4_
  - [ ]* 6.5 Write property test for quote text objects
    - **Property 11: Quote Text Object Selection**
    - **Validates: Requirements 4.3, 4.4**
  - [-] 6.6 Implement bracket text objects

    - Add VimSelectInnerBracket function for (), {}, [], <>
    - Add VimSelectABracket function for (), {}, [], <>
    - Handle nested brackets correctly
    - _Requirements: 4.5-4.12_
  - [ ]* 6.7 Write property test for bracket text objects
    - **Property 10: Bracket Text Object Selection**
    - **Validates: Requirements 4.5-4.12**
  - [x] 6.8 Integrate text objects with operators
    - Add chPendingTextObj to VimState
    - Handle 'i' and 'a' after pending operator (d, c, y)
    - Process text object character after i/a
    - _Requirements: 4.1-4.12_

- [ ] 7. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 8. Additional Edit Commands
  - [x] 8.1 Implement replace character (r)


    - Add VimReplaceChar function
    - Handle 'r' key to set pending replace state
    - Process next character as replacement
    - _Requirements: 5.1_
  - [ ]* 8.2 Write property test for replace character
    - **Property 12: Replace Character Correctness**
    - **Validates: Requirements 5.1**
  - [ ] 8.3 Implement Replace mode (R)
    - Add VIM_MODE_REPLACE to VimModeState enum

    - Add VimEnterReplaceMode function
    - Handle character input in Replace mode (overwrite)
    - _Requirements: 5.2_
  - [ ] 8.4 Implement s, S, C commands
    - Add VimSubstituteChar function (s - delete char, enter insert)
    - Add VimSubstituteLine function (S/cc - delete line content, enter insert)
    - Add VimChangeToEnd function (C - delete to EOL, enter insert)
    - _Requirements: 5.3, 5.4, 5.5_
  - [ ] 8.5 Implement D and Y commands
    - Verify VimDeleteToEnd (D) works correctly
    - Add VimYankToEnd function or alias Y to yy
    - _Requirements: 5.6, 5.7_
  - [ ] 8.6 Implement case toggle (~)
    - Add VimToggleCase function
    - Handle '~' key in Normal mode
    - _Requirements: 5.9_
  - [ ]* 8.7 Write property test for case toggle
    - **Property 13: Case Toggle Correctness**
    - **Validates: Requirements 5.9**
  - [ ] 8.8 Implement case conversion (gu, gU)
    - Add VimConvertToLower function with motion
    - Add VimConvertToUpper function with motion
    - Handle 'gu' and 'gU' pending operators
    - _Requirements: 5.10, 5.11_
  - [ ]* 8.9 Write property test for case conversion
    - **Property 14: Case Conversion Correctness**
    - **Validates: Requirements 5.10, 5.11**

- [ ] 9. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 10. Marks and Jump List
  - [ ] 10.1 Add mark storage to VimState
    - Add VimMark structure definition
    - Add marks[26] array to VimState
    - Initialize marks in InitVimMode
    - _Requirements: 6.1_
  - [ ] 10.2 Implement mark set and jump
    - Add VimSetMark function
    - Add VimJumpToMark function with bExact parameter
    - Handle 'm' key followed by letter
    - Handle backtick and single quote followed by letter
    - _Requirements: 6.1, 6.2, 6.3_
  - [ ]* 10.3 Write property test for mark set/jump
    - **Property 15: Mark Set and Jump Round-Trip**
    - **Property 16: Mark Line Jump**
    - **Validates: Requirements 6.1, 6.2, 6.3**
  - [ ] 10.4 Add jump list to VimState
    - Add JumpEntry structure definition
    - Add jumpList array and indices to VimState
    - Add VimAddJump function
    - _Requirements: 6.6, 6.7_
  - [ ] 10.5 Implement jump list navigation
    - Add VimJumpOlder function (Ctrl-O)
    - Add VimJumpNewer function (Ctrl-I)
    - Handle Ctrl-O and Ctrl-I in ProcessVimKey
    - _Requirements: 6.6, 6.7_
  - [ ]* 10.6 Write property test for jump list
    - **Property 17: Jump List Navigation**
    - **Validates: Requirements 6.6, 6.7**
  - [ ] 10.7 Implement last position jump
    - Add dwLastJumpPos to VimState
    - Add VimJumpToLastPosition function
    - Handle backtick-backtick and quote-quote
    - _Requirements: 6.4, 6.5_

- [ ] 11. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 12. Dot Repeat System
  - [ ] 12.1 Add ChangeRecord structure
    - Define ChangeRecord struct in vim_mode.h
    - Add lastChange to VimState
    - Add bRecording flag
    - _Requirements: 7.1_
  - [ ] 12.2 Implement change recording
    - Add VimStartRecording function
    - Add VimRecordMotion function
    - Add VimRecordInsertChar function
    - Add VimEndRecording function
    - _Requirements: 7.3, 7.4_
  - [ ] 12.3 Integrate recording with edit commands
    - Call VimStartRecording before d, c, r, s operations
    - Record insert mode text in VimRecordInsertChar
    - Call VimEndRecording when returning to Normal mode
    - _Requirements: 7.3, 7.4_
  - [ ] 12.4 Implement dot repeat
    - Add VimRepeatLastChange function
    - Handle '.' key in Normal mode
    - Support count override with dot command
    - _Requirements: 7.1, 7.2_
  - [ ]* 12.5 Write property test for dot repeat
    - **Property 18: Dot Repeat Correctness**
    - **Validates: Requirements 7.1, 7.2, 7.3, 7.4**

- [ ] 13. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 14. Additional Ex Commands
  - [x] 14.1 Add SubstituteCmd structure

    - Define SubstituteCmd struct
    - Add ParseSubstituteCommand function
    - _Requirements: 8.1, 8.2, 8.3_
  - [x] 14.2 Implement substitute command



    - Add VimSubstitute function
    - Handle :s/pattern/replacement/ syntax
    - Handle /g flag for global replacement
    - Handle % range for whole file
    - _Requirements: 8.1, 8.2, 8.3_
  - [ ]* 14.3 Write property test for substitute
    - **Property 19: Substitute Single Occurrence**
    - **Property 20: Substitute Global**
    - **Property 21: Substitute Whole File**
    - **Validates: Requirements 8.1, 8.2, 8.3**
  - [ ] 14.4 Add buffer navigation commands
    - Add :bn/:bnext command handling
    - Add :bp/:bprev command handling
    - Add :bd/:bdelete command handling
    - _Requirements: 8.7, 8.8, 8.9_
  - [ ] 14.5 Add utility commands
    - Add :noh/:nohlsearch command (clear search highlight)
    - Add :help command (show help dialog)
    - Add :sp/:vs commands (show not supported message)
    - _Requirements: 8.4, 8.5, 8.6, 8.10_

- [ ] 15. Update vim_mode.h with new declarations
  - Add all new function declarations
  - Add new structures (VimMark, JumpEntry, ChangeRecord, etc.)
  - Add new enum values (VIM_MODE_REPLACE)
  - Update VimState structure
  - _Requirements: All_

- [ ] 16. Final Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
