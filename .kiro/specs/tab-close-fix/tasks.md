# Implementation Plan

- [x] 1. Fix mouse coordinate handling in TabSubclassProc
  - [x] 1.1 Add windowsx.h include for GET_X_LPARAM and GET_Y_LPARAM macros
    - Add `#include <windowsx.h>` at the top of main.c
    - _Requirements: 1.3_
  - [x] 1.2 Fix WM_LBUTTONUP handler to use proper coordinate extraction
    - Replace `LOWORD(lParam)` with `GET_X_LPARAM(lParam)`
    - Replace `HIWORD(lParam)` with `GET_Y_LPARAM(lParam)`
    - _Requirements: 1.1, 1.3_
  - [x] 1.3 Fix WM_MOUSEMOVE handler to use proper coordinate extraction
    - Replace `LOWORD(lParam)` with `GET_X_LPARAM(lParam)`
    - Replace `HIWORD(lParam)` with `GET_Y_LPARAM(lParam)`
    - _Requirements: 3.1, 3.2_

- [x] 2. Fix close button area detection
  - [x] 2.1 Update WM_LBUTTONUP to check both X and Y bounds using PtInRect
    - Calculate close button rectangle with proper X and Y bounds
    - Use PtInRect for accurate hit detection
    - _Requirements: 1.1, 1.2, 1.3_
  - [x] 2.2 Update WM_MOUSEMOVE hover detection to check both X and Y bounds
    - Calculate close button rectangle for hover detection
    - Use PtInRect for accurate hover detection
    - _Requirements: 3.1, 3.2_

- [x] 3. Remove duplicate close button handler in WndProc
  - [x] 3.1 Remove or simplify the backup WM_LBUTTONUP handler in WndProc
    - The TabSubclassProc already handles close button clicks
    - Remove duplicate code to prevent potential conflicts
    - _Requirements: 1.1, 4.1_

- [x] 4. Verify and test fixes
  - [x] 4.1 Build the application and verify no compilation errors
    - Run build.bat to compile
    - Fix any compilation errors
    - _Requirements: 1.1, 2.1_
  - [ ] 4.2 Manual testing of close button functionality
    - Test clicking close button on various tabs
    - Test clicking near but outside close button
    - Test Ctrl+W shortcut
    - _Requirements: 1.1, 1.2, 2.1, 2.2_

- [ ] 5. Final Checkpoint - Ensure all functionality works correctly
  - Ensure all tests pass, ask the user if questions arise.
