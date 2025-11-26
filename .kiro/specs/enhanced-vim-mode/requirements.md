# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk peningkatan fitur Vim Mode pada XNote text editor. Peningkatan mencakup tiga area utama: perbaikan bug yang teridentifikasi, optimasi performa, dan penambahan fitur Vim yang lebih lengkap untuk memberikan pengalaman editing yang lebih baik bagi pengguna yang terbiasa dengan Vim.

## Glossary

- **XNote**: Aplikasi text editor berbasis Windows yang sedang dikembangkan
- **Vim Mode**: Mode editing yang meniru keybinding dan behavior dari editor Vim
- **Normal Mode**: Mode navigasi default di Vim untuk pergerakan cursor dan perintah
- **Insert Mode**: Mode untuk memasukkan teks
- **Visual Mode**: Mode untuk memilih teks secara karakter
- **Visual Line Mode**: Mode untuk memilih teks per baris
- **Command Mode**: Mode untuk menjalankan perintah ex (dimulai dengan :)
- **Search Mode**: Mode untuk mencari teks (dimulai dengan / atau ?)
- **Motion**: Perintah pergerakan cursor di Vim
- **Operator**: Perintah yang bekerja dengan motion (d, y, c)
- **Text Object**: Objek teks seperti word, sentence, paragraph, atau blok dalam tanda kurung
- **Register**: Tempat penyimpanan teks yang di-yank atau delete
- **Macro**: Rekaman sequence keystroke yang dapat diputar ulang
- **Mark**: Penanda posisi dalam dokumen

## Requirements

### Requirement 1: Bug Fixes

**User Story:** As a user, I want the Vim mode to work correctly without bugs, so that I can edit text reliably.

#### Acceptance Criteria

1. WHEN a user presses 'P' (paste before) in Normal mode THEN the XNote SHALL paste content before the current cursor position without moving cursor backward first
2. WHEN a user uses 'f' or 'F' find character command THEN the XNote SHALL correctly find and move to the character on the current line
3. WHEN a user uses 't' or 'T' till character command THEN the XNote SHALL stop one character before/after the target character
4. WHEN a user uses paragraph motion ('{' or '}') THEN the XNote SHALL correctly identify paragraph boundaries by empty lines
5. WHEN a user uses matching bracket motion ('%') on nested brackets THEN the XNote SHALL correctly match the corresponding bracket at the same nesting level
6. WHEN a user uses 'dd' to delete a line THEN the XNote SHALL store the deleted content in the register for later paste operations
7. WHEN a user uses 'yy' to yank a line THEN the XNote SHALL store the yanked content with line-mode flag for correct paste behavior

### Requirement 2: Performance Optimization

**User Story:** As a user, I want Vim mode operations to be fast and responsive, so that I can edit efficiently even in large files.

#### Acceptance Criteria

1. WHEN a user performs navigation commands in files larger than 1MB THEN the XNote SHALL complete the operation within 50 milliseconds
2. WHEN a user performs search operations THEN the XNote SHALL use optimized search algorithms to minimize memory allocation
3. WHEN a user scrolls through a large file THEN the XNote SHALL use debounced updates to maintain smooth scrolling
4. WHEN GetTextBuffer is called multiple times in sequence THEN the XNote SHALL cache the buffer to avoid repeated memory allocations
5. WHEN a user performs word motions (w, b, e) THEN the XNote SHALL use efficient character classification without repeated buffer allocations

### Requirement 3: Additional Vim Motions

**User Story:** As a Vim user, I want more motion commands available, so that I can navigate text more efficiently.

#### Acceptance Criteria

1. WHEN a user presses 'W' in Normal mode THEN the XNote SHALL move to the next WORD (whitespace-delimited)
2. WHEN a user presses 'B' in Normal mode THEN the XNote SHALL move to the previous WORD (whitespace-delimited)
3. WHEN a user presses 'E' in Normal mode THEN the XNote SHALL move to the end of the current/next WORD
4. WHEN a user presses 'ge' in Normal mode THEN the XNote SHALL move to the end of the previous word
5. WHEN a user presses 'gE' in Normal mode THEN the XNote SHALL move to the end of the previous WORD
6. WHEN a user presses 'H' in Normal mode THEN the XNote SHALL move cursor to the top of the visible screen
7. WHEN a user presses 'M' in Normal mode THEN the XNote SHALL move cursor to the middle of the visible screen
8. WHEN a user presses 'L' in Normal mode THEN the XNote SHALL move cursor to the bottom of the visible screen
9. WHEN a user presses '+' or Enter in Normal mode THEN the XNote SHALL move to the first non-blank character of the next line
10. WHEN a user presses '-' in Normal mode THEN the XNote SHALL move to the first non-blank character of the previous line

### Requirement 4: Text Objects

**User Story:** As a Vim user, I want text object support, so that I can select and operate on logical text units.

#### Acceptance Criteria

1. WHEN a user types 'iw' after an operator THEN the XNote SHALL select the inner word (word without surrounding whitespace)
2. WHEN a user types 'aw' after an operator THEN the XNote SHALL select a word including surrounding whitespace
3. WHEN a user types 'i"' or "i'" after an operator THEN the XNote SHALL select text inside quotes
4. WHEN a user types 'a"' or "a'" after an operator THEN the XNote SHALL select text including the quotes
5. WHEN a user types 'i(' or 'i)' or 'ib' after an operator THEN the XNote SHALL select text inside parentheses
6. WHEN a user types 'a(' or 'a)' or 'ab' after an operator THEN the XNote SHALL select text including parentheses
7. WHEN a user types 'i{' or 'i}' or 'iB' after an operator THEN the XNote SHALL select text inside curly braces
8. WHEN a user types 'a{' or 'a}' or 'aB' after an operator THEN the XNote SHALL select text including curly braces
9. WHEN a user types 'i[' or 'i]' after an operator THEN the XNote SHALL select text inside square brackets
10. WHEN a user types 'a[' or 'a]' after an operator THEN the XNote SHALL select text including square brackets
11. WHEN a user types 'i<' or 'i>' after an operator THEN the XNote SHALL select text inside angle brackets
12. WHEN a user types 'a<' or 'a>' after an operator THEN the XNote SHALL select text including angle brackets

### Requirement 5: Additional Edit Commands

**User Story:** As a Vim user, I want more editing commands, so that I can modify text more efficiently.

#### Acceptance Criteria

1. WHEN a user presses 'r' followed by a character in Normal mode THEN the XNote SHALL replace the character under cursor with the typed character
2. WHEN a user presses 'R' in Normal mode THEN the XNote SHALL enter Replace mode where typing overwrites existing characters
3. WHEN a user presses 's' in Normal mode THEN the XNote SHALL delete the character under cursor and enter Insert mode
4. WHEN a user presses 'S' or 'cc' in Normal mode THEN the XNote SHALL delete the entire line content and enter Insert mode
5. WHEN a user presses 'C' in Normal mode THEN the XNote SHALL delete from cursor to end of line and enter Insert mode
6. WHEN a user presses 'D' in Normal mode THEN the XNote SHALL delete from cursor to end of line
7. WHEN a user presses 'Y' in Normal mode THEN the XNote SHALL yank the entire current line
8. WHEN a user presses 'xp' in Normal mode THEN the XNote SHALL transpose (swap) two characters
9. WHEN a user presses '~' in Normal mode THEN the XNote SHALL toggle the case of the character under cursor
10. WHEN a user presses 'gU' followed by a motion THEN the XNote SHALL convert text to uppercase
11. WHEN a user presses 'gu' followed by a motion THEN the XNote SHALL convert text to lowercase

### Requirement 6: Marks and Jumps

**User Story:** As a Vim user, I want mark and jump functionality, so that I can quickly navigate to saved positions.

#### Acceptance Criteria

1. WHEN a user presses 'm' followed by a letter (a-z) THEN the XNote SHALL save the current cursor position as a local mark
2. WHEN a user presses backtick (`) followed by a mark letter THEN the XNote SHALL jump to the exact position of that mark
3. WHEN a user presses single quote (') followed by a mark letter THEN the XNote SHALL jump to the first non-blank character of the marked line
4. WHEN a user presses backtick-backtick (``) THEN the XNote SHALL jump to the position before the last jump
5. WHEN a user presses single quote-single quote ('') THEN the XNote SHALL jump to the line before the last jump
6. WHEN a user presses Ctrl-O THEN the XNote SHALL jump to the previous position in the jump list
7. WHEN a user presses Ctrl-I THEN the XNote SHALL jump to the next position in the jump list

### Requirement 7: Repeat and Dot Command

**User Story:** As a Vim user, I want the dot command to repeat the last change, so that I can efficiently repeat edits.

#### Acceptance Criteria

1. WHEN a user presses '.' in Normal mode THEN the XNote SHALL repeat the last text-changing command
2. WHEN a user uses a count with '.' command THEN the XNote SHALL use the new count instead of the original
3. WHEN a user performs insert mode edits THEN the XNote SHALL record the entire insert session for dot repeat
4. WHEN a user performs delete, change, or replace operations THEN the XNote SHALL record the operation for dot repeat

### Requirement 8: Additional Ex Commands

**User Story:** As a Vim user, I want more ex commands available, so that I can perform common operations from command mode.

#### Acceptance Criteria

1. WHEN a user types ':s/pattern/replacement/' THEN the XNote SHALL substitute the first occurrence on the current line
2. WHEN a user types ':s/pattern/replacement/g' THEN the XNote SHALL substitute all occurrences on the current line
3. WHEN a user types ':%s/pattern/replacement/g' THEN the XNote SHALL substitute all occurrences in the entire file
4. WHEN a user types ':noh' or ':nohlsearch' THEN the XNote SHALL clear search highlighting
5. WHEN a user types ':sp' or ':split' THEN the XNote SHALL display a message that split is not supported (graceful degradation)
6. WHEN a user types ':vs' or ':vsplit' THEN the XNote SHALL display a message that vsplit is not supported (graceful degradation)
7. WHEN a user types ':bn' or ':bnext' THEN the XNote SHALL switch to the next buffer (tab)
8. WHEN a user types ':bp' or ':bprev' THEN the XNote SHALL switch to the previous buffer (tab)
9. WHEN a user types ':bd' or ':bdelete' THEN the XNote SHALL close the current buffer (tab)
10. WHEN a user types ':help' THEN the XNote SHALL display a help dialog with available Vim commands

