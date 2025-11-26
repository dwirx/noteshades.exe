# Implementation Plan

## Large File Performance Optimization

- [x] 1. Setup core infrastructure and constants




  - [ ] 1.1 Add file mode threshold constants to notepad.h
    - Define THRESHOLD_PARTIAL (50MB), THRESHOLD_READONLY (200MB), THRESHOLD_MMAP (1GB)

    - Add INITIAL_CHUNK_SIZE (20MB), LOAD_MORE_CHUNK (20MB), PREVIEW_SIZE (10MB)
    - _Requirements: 1.1, 1.2, 1.3_

  - [ ] 1.2 Add progress dialog resource definitions to resource.h
    - Define IDD_PROGRESS, IDC_PROGRESS_BAR, IDC_PROGRESS_TEXT, IDC_PROGRESS_CANCEL
    - _Requirements: 2.1, 2.2_
  - [ ] 1.3 Create progress dialog template in notepad.rc
    - Add dialog with progress bar, text label, and cancel button




    - _Requirements: 2.1, 2.2, 2.3_
  - [x]* 1.4 Write property test for file mode selection

    - **Property 1: File Mode Selection Correctness**
    - **Validates: Requirements 1.1, 1.2, 1.3**

- [ ] 2. Implement file mode detection
  - [ ] 2.1 Implement DetectOptimalFileMode function in file_ops.c
    - Return appropriate FileModeType based on file size thresholds



    - Handle edge cases at threshold boundaries
    - _Requirements: 1.1, 1.2, 1.3_
  - [x] 2.2 Implement ShowLargeFileInfo dialog function

    - Display informative message about selected mode
    - Show file size and mode-specific details
    - _Requirements: 8.1_
  - [ ]* 2.3 Write unit tests for mode detection thresholds
    - Test boundary values: 49MB, 50MB, 199MB, 200MB, 1023MB, 1024MB
    - _Requirements: 1.1, 1.2, 1.3_




- [ ] 3. Implement progress dialog system
  - [ ] 3.1 Implement ProgressDlgProc dialog procedure
    - Handle WM_INITDIALOG, WM_TIMER, WM_COMMAND for cancel

    - Update progress bar and text periodically
    - _Requirements: 2.1, 2.2, 2.3, 2.4_
  - [x] 3.2 Implement progress calculation and display functions

    - UpdateProgressBar: Set progress bar position (0-100)
    - UpdateProgressText: Format "Loading... X% (Y MB / Z MB)"
    - _Requirements: 2.1, 2.2_

  - [ ]* 3.3 Write property test for progress tracking invariants
    - **Property 2: Progress Tracking Invariants**



    - **Validates: Requirements 2.1, 2.2**

- [ ] 4. Implement background thread file loading
  - [x] 4.1 Implement FileLoadThreadProc for background loading

    - Open file, read in chunks (256KB), update progress
    - Handle cancellation via bCancelled flag
    - Convert to UTF-16 after reading
    - _Requirements: 1.4, 2.1, 2.2_

  - [ ] 4.2 Implement MemoryStreamCallback for RichEdit streaming
    - Feed text to RichEdit control efficiently
    - Update progress during streaming phase

    - _Requirements: 1.4_
  - [ ] 4.3 Update ReadFileContent to use background thread
    - Create thread, show progress dialog, wait for completion
    - Handle modal vs modeless dialog based on file size
    - _Requirements: 1.1, 1.4, 2.1_




- [ ] 5. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.


- [ ] 6. Implement chunked loading mode (FILEMODE_PARTIAL)
  - [x] 6.1 Implement LoadFileChunked function

    - Load specified chunk size from file
    - Convert to UTF-16 and set in edit control
    - Return bytes loaded
    - _Requirements: 1.1, 3.2_
  - [ ] 6.2 Implement LoadMoreContent function
    - Read next chunk from current position



    - Append to existing content in edit control
    - Update dwLoadedSize in TabState
    - _Requirements: 3.2_

  - [ ] 6.3 Add F5 key handler for "Load More" functionality
    - Check if current file is in FILEMODE_PARTIAL
    - Call LoadMoreContent and update status bar

    - _Requirements: 3.2_
  - [x] 6.4 Update status bar to show partial loading info

    - Display "Loaded X MB of Y MB - Press F5 to load more"
    - Show "File fully loaded" when complete
    - _Requirements: 3.1, 3.3, 8.3_
  - [ ]* 6.5 Write property test for partial loading state
    - **Property 3: Partial Loading State Consistency**
    - **Validates: Requirements 3.1, 3.2**


- [x] 7. Implement read-only preview mode (FILEMODE_READONLY)



  - [ ] 7.1 Implement read-only preview loading
    - Load first 10MB only
    - Set edit control to read-only mode

    - _Requirements: 1.2_
  - [x] 7.2 Update title bar for read-only mode

    - Append "READ-ONLY" indicator to window title
    - _Requirements: 8.2_
  - [x] 7.3 Disable undo buffer for read-only mode

    - Clear undo buffer after loading
    - Prevent undo operations
    - _Requirements: 7.2_
  - [ ]* 7.4 Write property test for mode indicator display
    - **Property 6: Mode Indicator Display**
    - **Validates: Requirements 8.2, 8.3**




- [ ] 8. Implement memory-mapped mode (FILEMODE_MMAP)
  - [ ] 8.1 Implement MMapOpenFile function
    - Create file mapping with CreateFileMapping
    - Map initial view with MapViewOfFile

    - _Requirements: 1.3_
  - [ ] 8.2 Implement MMapReadView function
    - Unmap previous view, map new view at offset

    - Handle view size and alignment requirements
    - _Requirements: 1.3, 4.2_
  - [ ] 8.3 Implement MMapCloseFile cleanup function
    - Unmap view, close mapping handle, close file handle
    - _Requirements: 1.3_

  - [ ] 8.4 Integrate memory-mapped mode with scroll handling
    - Update view when user scrolls to new region



    - Only load visible portion into memory
    - _Requirements: 4.2_
  - [x]* 8.5 Write property test for memory-mapped view bounds

    - **Property 4: Memory-Mapped View Bounds**
    - **Validates: Requirements 4.2**




- [ ] 9. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.


- [ ] 10. Implement large file feature restrictions
  - [ ] 10.1 Auto-disable syntax highlighting for files > 1MB
    - Check file size before applying syntax highlighting
    - Set language to LANG_NONE for display purposes

    - _Requirements: 5.1_
  - [ ] 10.2 Show notification when syntax highlighting disabled
    - Update status bar with "Syntax highlighting disabled for large file"
    - _Requirements: 5.2_
  - [x] 10.3 Add warning dialog for manual syntax enable on large files



    - Show performance warning when user tries to enable syntax
    - Allow user to proceed or cancel
    - _Requirements: 5.3_
  - [ ] 10.4 Limit undo buffer for large files
    - Set undo limit to 100 operations for files > 50MB
    - Use EM_SETUNDOLIMIT message for RichEdit
    - _Requirements: 7.1_
  - [ ]* 10.5 Write property test for large file feature restrictions
    - **Property 5: Large File Feature Restrictions**
    - **Validates: Requirements 5.1, 7.1, 7.2**

- [ ] 11. Implement chunked file saving
  - [ ] 11.1 Implement FileSaveThreadProc for background saving
    - Write to temporary file first
    - Write in chunks to maintain responsiveness
    - Update progress during save
    - _Requirements: 6.1, 6.2_
  - [ ] 11.2 Implement safe file replacement
    - Only replace original after successful temp write
    - Delete temp file on failure, keep original
    - _Requirements: 6.3_
  - [ ] 11.3 Update WriteFileContent to use background thread for large files
    - Show progress dialog for files > 50MB
    - Use chunked writing for all large files
    - _Requirements: 6.1, 6.2_
  - [ ] 11.4 Show save confirmation in status bar
    - Display "File saved successfully" message
    - _Requirements: 6.4_
  - [ ]* 11.5 Write property test for save operation file preservation
    - **Property 7: Save Operation File Preservation**
    - **Validates: Requirements 6.3**

- [ ] 12. Implement scroll optimization
  - [ ] 12.1 Implement scroll debouncing
    - Use timer to delay scroll handling
    - Prevent excessive rendering during fast scroll
    - _Requirements: 4.3_
  - [ ] 12.2 Optimize RichEdit rendering for large content
    - Disable redraw during bulk operations
    - Use WM_SETREDRAW FALSE/TRUE pattern
    - _Requirements: 4.1_

- [ ] 13. Final integration and cleanup
  - [ ] 13.1 Update UpdateWindowTitle for all file modes
    - Include mode indicator (READ-ONLY, PARTIAL, etc.)
    - _Requirements: 8.2_
  - [ ] 13.2 Update UpdateStatusBar for large file info
    - Show file mode, loaded size, total size
    - Show "Load More" instruction for partial mode
    - _Requirements: 3.1, 8.3_
  - [ ] 13.3 Handle tab close cleanup for memory-mapped files
    - Properly close file mapping handles
    - Free mapped views
    - _Requirements: 1.3_
  - [ ]* 13.4 Write integration tests for complete workflow
    - Test open, edit, save cycle for each file mode
    - Test mode transitions and error handling
    - _Requirements: All_

- [ ] 14. Final Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
