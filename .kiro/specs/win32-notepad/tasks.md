# Implementation Plan

- [x] 1. Setup project structure dan resource files






  - [x] 1.1 Buat struktur direktori project (src/, tests/)

    - Buat folder src untuk source code
    - Buat folder tests untuk test files
    - _Requirements: 6.1_

  - [x] 1.2 Buat resource.h dengan definisi konstanta

    - Definisikan menu command IDs (IDM_FILE_NEW, IDM_FILE_OPEN, dll)
    - Definisikan control IDs (IDC_EDIT, IDC_STATUS)
    - _Requirements: 6.1_

  - [x] 1.3 Buat notepad.rc resource file

    - Definisikan menu structure (File, Edit, Help)
    - Definisikan accelerator table untuk keyboard shortcuts
    - _Requirements: 6.1, 6.4_

  - [x] 1.4 Buat Makefile untuk kompilasi

    - Setup compiler flags untuk Win32
    - Definisikan target untuk build dan clean
    - _Requirements: 6.1_

- [x] 2. Implementasi header dan data structures


  - [x] 2.1 Buat notepad.h dengan deklarasi dan structures


    - Definisikan FileState structure
    - Definisikan AppState structure
    - Deklarasikan function prototypes untuk semua komponen
    - _Requirements: 2.2, 3.2, 6.1_





- [x] 3. Implementasi main window dan entry point

  - [x] 3.1 Buat main.c dengan WinMain dan window registration

    - Implementasi WinMain entry point

    - Implementasi RegisterMainWindowClass
    - Implementasi CreateMainWindow dengan menu bar
    - Implementasi message loop dengan accelerator handling
    - _Requirements: 6.1, 6.4_
  - [x] 3.2 Implementasi WndProc untuk message handling

    - Handle WM_CREATE untuk inisialisasi controls



    - Handle WM_SIZE untuk resize edit control
    - Handle WM_COMMAND untuk menu commands
    - Handle WM_CLOSE untuk konfirmasi exit
    - Handle WM_DESTROY untuk cleanup
    - _Requirements: 6.1, 6.3, 5.1, 5.2_

- [x] 4. Implementasi file operations


  - [x] 4.1 Buat file_ops.c dengan fungsi file handling

    - Implementasi FileNew - reset state dan clear edit
    - Implementasi FileOpen - baca file ke edit control
    - Implementasi FileSave - simpan konten ke file
    - Implementasi FileSaveAs - dialog save dan simpan
    - Implementasi PromptSaveChanges - dialog konfirmasi



    - Implementasi UpdateWindowTitle - update judul jendela
    - _Requirements: 1.1, 1.2, 1.3, 2.1, 2.2, 2.3, 2.4, 3.1, 3.2, 3.3, 3.4, 3.5_
  - [ ]* 4.2 Write property test untuk file round-trip
    - **Property 1: File Round-Trip Consistency**
    - **Validates: Requirements 2.2, 3.2**
  - [ ]* 4.3 Write property test untuk save untitled behavior
    - **Property 2: Save Triggers Save As for Untitled Documents**
    - **Validates: Requirements 3.1**


- [x] 5. Implementasi edit operations

  - [x] 5.1 Buat edit_ops.c dengan fungsi edit handling
    - Implementasi EditUndo menggunakan EM_UNDO message
    - Implementasi EditCut menggunakan WM_CUT message
    - Implementasi EditCopy menggunakan WM_COPY message
    - Implementasi EditPaste menggunakan WM_PASTE message
    - Implementasi EditSelectAll menggunakan EM_SETSEL message
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_
  - [-]* 5.2 Write property test untuk clipboard operations


    - **Property 3: Cut Removes and Copies to Clipboard**
    - **Property 4: Copy Preserves Original and Copies to Clipboard**
    - **Property 5: Paste Inserts Clipboard Content**
    - **Validates: Requirements 4.1, 4.2, 4.3**
  - [ ]* 5.3 Write property test untuk select all
    - **Property 6: Select All Selects Entire Content**
    - **Validates: Requirements 4.4**
  - [ ]* 5.4 Write property test untuk undo operation
    - **Property 7: Undo Reverses Last Operation**
    - **Validates: Requirements 4.5**

- [x] 6. Implementasi dialog operations

  - [x] 6.1 Buat dialogs.c dengan fungsi dialog handling

    - Implementasi ShowOpenDialog menggunakan GetOpenFileName
    - Implementasi ShowSaveDialog menggunakan GetSaveFileName
    - Implementasi ShowConfirmSaveDialog menggunakan MessageBox
    - Implementasi ShowAboutDialog menggunakan MessageBox
    - Implementasi ShowErrorDialog untuk error messages
    - _Requirements: 2.1, 3.3, 5.2, 5.3, 5.4, 5.5, 7.1, 7.2_

- [x] 7. Integrasi dan wiring semua komponen

  - [x] 7.1 Integrasikan file operations ke WndProc

    - Wire IDM_FILE_NEW ke FileNew
    - Wire IDM_FILE_OPEN ke FileOpen
    - Wire IDM_FILE_SAVE ke FileSave
    - Wire IDM_FILE_SAVEAS ke FileSaveAs
    - Wire IDM_FILE_EXIT ke exit handling
    - _Requirements: 1.1, 2.1, 3.1, 3.3, 5.1_


  - [x] 7.2 Integrasikan edit operations ke WndProc
    - Wire IDM_EDIT_UNDO ke EditUndo
    - Wire IDM_EDIT_CUT ke EditCut
    - Wire IDM_EDIT_COPY ke EditCopy
    - Wire IDM_EDIT_PASTE ke EditPaste
    - Wire IDM_EDIT_SELECTALL ke EditSelectAll

    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_

  - [x] 7.3 Integrasikan dialog operations
    - Wire IDM_HELP_ABOUT ke ShowAboutDialog
    - Integrasikan error dialogs ke file operations
    - _Requirements: 7.1, 7.2, 2.4, 3.5_
  - [ ]* 7.4 Write property test untuk accelerator mapping
    - **Property 8: Accelerator Keys Map to Correct Commands**
    - **Validates: Requirements 6.4**

- [x] 8. Checkpoint - Pastikan semua tests passing

  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Handle modified state dan exit confirmation

  - [x] 9.1 Implementasi tracking modified state

    - Handle EN_CHANGE notification dari edit control
    - Update bModified flag saat konten berubah

    - Reset bModified setelah save
    - _Requirements: 1.2, 5.2_

  - [x] 9.2 Implementasi exit confirmation flow
    - Check bModified pada WM_CLOSE
    - Tampilkan dialog konfirmasi jika ada perubahan
    - Handle Yes/No/Cancel responses
    - _Requirements: 5.2, 5.3, 5.4, 5.5_


- [x] 10. Final Checkpoint - Pastikan semua tests passing

  - Ensure all tests pass, ask the user if questions arise.
