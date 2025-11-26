# Implementation Plan

- [x] 1. Add command line argument support to xNote




  - [x] 1.1 Modify WinMain to parse command line arguments

    - Parse lpCmdLine to extract file path
    - Handle quoted paths with spaces
    - Store file path for later use
    - _Requirements: 3.1, 3.2_

  - [x] 1.2 Implement file opening from command line


    - After window creation, check if file path was provided
    - Call FileOpen with the provided path
    - Handle errors gracefully
    - _Requirements: 3.1, 3.3_
  - [ ]* 1.3 Write property test for command line file opening
    - **Property 1: Command line file opening**
    - **Validates: Requirements 3.1**
  - [ ]* 1.4 Write unit tests for command line argument parsing
    - Test empty arguments
    - Test single file path
    - Test quoted paths with spaces
    - Test invalid paths
    - _Requirements: 3.1, 3.2, 3.3_

- [x] 2. Checkpoint - Ensure command line support works


  - Ensure all tests pass, ask the user if questions arise.


- [x] 3. Create NSIS installer script




  - [x] 3.1 Create installer directory and basic NSIS script


    - Create installer/ directory
    - Define installer metadata (name, version, publisher)
    - Set default installation directory to Program Files
    - _Requirements: 1.1_
  - [x] 3.2 Add file installation section

    - Copy xnote.exe to installation directory
    - Copy required DLLs if any
    - Set output directory
    - _Requirements: 1.1_

  - [x] 3.3 Add shortcut creation
    - Create Start Menu folder and shortcut
    - Add optional Desktop shortcut with checkbox
    - _Requirements: 1.2, 1.3_
  - [x] 3.4 Add uninstaller section

    - Create uninstaller executable
    - Remove installed files
    - Remove shortcuts
    - Remove Start Menu folder
    - _Requirements: 1.5_



- [x] 4. Add file association registration
  - [x] 4.1 Register ProgID for xNote
    - Create xNote.TextFile registry key
    - Set default icon
    - Set shell open command with "%1" parameter
    - _Requirements: 2.1, 2.2, 2.3_
  - [x] 4.2 Register file extensions
    - Register .txt, .log extensions
    - Register .ini, .cfg, .md, .json, .xml extensions
    - Register .html, .css, .js, .c, .h, .py extensions
    - _Requirements: 2.1, 2.2, 2.3_
  - [x] 4.3 Add "Open with xNote" context menu
    - Register under Applications\xnote.exe
    - Add shell\open\command entry
    - _Requirements: 2.5_
  - [x] 4.4 Add default program option
    - Add checkbox in installer for setting as default
    - Set default associations if selected
    - _Requirements: 2.4_

- [x] 5. Add Add/Remove Programs registration

  - [x] 5.1 Write registry entries for uninstall info

    - Add DisplayName, DisplayVersion, Publisher
    - Add UninstallString pointing to uninstaller
    - Add InstallLocation and DisplayIcon
    - _Requirements: 1.4_

  - [x] 5.2 Update uninstaller to clean registry
    - Remove file association entries
    - Remove uninstall registry entries
    - _Requirements: 1.5_

- [x] 6. Handle upgrade scenarios

  - [x] 6.1 Add running instance detection

    - Check if xnote.exe is running before install
    - Prompt user to close if running
    - _Requirements: 4.3_

  - [x] 6.2 Preserve user settings during upgrade
    - Check for existing installation
    - Skip settings files during upgrade
    - _Requirements: 4.1, 4.2_

- [x] 7. Create build script for installer


  - [x] 7.1 Create make_installer.bat script

    - Check if xnote.exe exists, build if not
    - Run NSIS compiler with script
    - Generate versioned installer filename
    - _Requirements: 5.1, 5.2, 5.3_

  - [x] 7.2 Add version extraction from source
    - Read version from header or resource file
    - Pass version to NSIS script
    - _Requirements: 5.3_

- [x] 8. Final Checkpoint - Verify complete installer
  - Ensure all tests pass, ask the user if questions arise.
