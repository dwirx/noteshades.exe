# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk fitur Multi-Cursor Editing dan Drag & Drop file pada XNote text editor. Fitur Multi-Cursor memungkinkan pengguna untuk mengedit beberapa lokasi secara bersamaan, meningkatkan produktivitas saat melakukan perubahan repetitif. Fitur Drag & Drop memungkinkan pengguna membuka file dengan menyeret file dari Windows Explorer ke jendela XNote.

## Glossary

- **XNote**: Aplikasi text editor Windows yang dikembangkan menggunakan Win32 API dan RichEdit control
- **Multi-Cursor**: Kemampuan untuk memiliki beberapa cursor aktif secara bersamaan dalam satu editor
- **Primary Cursor**: Cursor utama yang ditampilkan dengan warna standar
- **Secondary Cursor**: Cursor tambahan yang dibuat melalui Alt+Click atau Ctrl+D
- **Occurrence**: Kemunculan teks yang sama dengan teks yang sedang dipilih
- **Column Selection**: Mode seleksi vertikal yang memilih teks dalam bentuk kolom/rectangular
- **Drag & Drop**: Mekanisme untuk membuka file dengan menyeret dari file explorer ke aplikasi
- **RichEdit Control**: Windows control untuk editing teks dengan fitur formatting

## Requirements

### Requirement 1: Select Next Occurrence

**User Story:** As a user, I want to select the next occurrence of the currently selected text using Ctrl+D, so that I can quickly edit multiple instances of the same text.

#### Acceptance Criteria

1. WHEN a user has text selected and presses Ctrl+D, THEN XNote SHALL find and add a selection for the next occurrence of that text in the document
2. WHEN a user presses Ctrl+D multiple times, THEN XNote SHALL continue adding selections for subsequent occurrences
3. WHEN no more occurrences exist after the current position, THEN XNote SHALL wrap around and search from the beginning of the document
4. WHEN all occurrences are already selected, THEN XNote SHALL provide visual feedback that no more occurrences exist
5. WHEN the user types with multiple selections active, THEN XNote SHALL replace all selected text simultaneously with the typed characters

### Requirement 2: Multiple Cursors via Alt+Click

**User Story:** As a user, I want to add additional cursors by Alt+Clicking at different positions, so that I can edit multiple non-contiguous locations simultaneously.

#### Acceptance Criteria

1. WHEN a user holds Alt and clicks at a position, THEN XNote SHALL add a new cursor at that position while keeping existing cursors
2. WHEN a user Alt+Clicks at an existing cursor position, THEN XNote SHALL remove that cursor
3. WHEN multiple cursors exist and the user types, THEN XNote SHALL insert the typed text at all cursor positions simultaneously
4. WHEN multiple cursors exist and the user presses Escape, THEN XNote SHALL collapse to a single cursor at the primary cursor position
5. WHEN multiple cursors exist and the user uses arrow keys, THEN XNote SHALL move all cursors in the same direction

### Requirement 3: Column Selection Mode

**User Story:** As a user, I want to select text in a rectangular/column pattern using Alt+Shift+Arrow or Alt+Drag, so that I can edit vertically aligned text efficiently.

#### Acceptance Criteria

1. WHEN a user holds Alt+Shift and presses arrow keys, THEN XNote SHALL extend the selection in a rectangular column pattern
2. WHEN a user holds Alt and drags the mouse, THEN XNote SHALL create a rectangular column selection
3. WHEN column selection is active and the user types, THEN XNote SHALL insert the typed text at each row of the column selection
4. WHEN column selection spans multiple lines with varying lengths, THEN XNote SHALL handle lines shorter than the selection column gracefully by padding with spaces if needed
5. WHEN the user releases Alt during column selection, THEN XNote SHALL maintain the current column selection until a new selection is made

### Requirement 4: Multi-Cursor Visual Feedback

**User Story:** As a user, I want to see clear visual indicators for all active cursors and selections, so that I can understand where my edits will occur.

#### Acceptance Criteria

1. WHEN multiple cursors are active, THEN XNote SHALL display all cursors with visible blinking carets
2. WHEN multiple selections exist, THEN XNote SHALL highlight all selected regions with the selection color
3. WHEN the user hovers over the status bar, THEN XNote SHALL display the count of active cursors
4. WHEN column selection mode is active, THEN XNote SHALL display a visual indicator in the status bar

### Requirement 5: Drag and Drop File Opening

**User Story:** As a user, I want to open files by dragging them from Windows Explorer into XNote, so that I can quickly open files without using the File menu.

#### Acceptance Criteria

1. WHEN a user drags a file over the XNote window, THEN XNote SHALL display a visual indicator that the file can be dropped
2. WHEN a user drops a single file onto XNote, THEN XNote SHALL open that file in a new tab
3. WHEN a user drops multiple files onto XNote, THEN XNote SHALL open each file in separate tabs
4. WHEN a user drops a file that is already open, THEN XNote SHALL switch to the existing tab instead of opening a duplicate
5. WHEN a user drops a non-text file or unsupported file type, THEN XNote SHALL display an appropriate warning message

### Requirement 6: Multi-Cursor State Management

**User Story:** As a developer, I want the multi-cursor state to be properly managed, so that the feature works reliably across different editing scenarios.

#### Acceptance Criteria

1. WHEN the user performs undo with multiple cursors, THEN XNote SHALL undo the last action at all cursor positions as a single operation
2. WHEN the user switches tabs, THEN XNote SHALL preserve the multi-cursor state for each tab independently
3. WHEN the user performs find/replace, THEN XNote SHALL clear multi-cursor mode and use standard single-cursor behavior
4. WHEN the document is modified externally, THEN XNote SHALL reset to single-cursor mode to prevent cursor position inconsistencies
