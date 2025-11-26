# Requirements Document

## Introduction

Fitur ini memungkinkan xNote untuk diinstal di Windows sebagai aplikasi yang proper dengan installer, dan dapat dijadikan aplikasi default untuk membuka file teks (.txt, .log, dll). Installer akan menangani registrasi file association, penambahan ke Start Menu, dan opsi uninstall yang bersih.

## Glossary

- **xNote**: Aplikasi text editor berbasis Scintilla yang sedang dikembangkan
- **Installer**: Program setup yang menginstal aplikasi ke sistem Windows
- **File Association**: Pengaturan Windows yang menghubungkan ekstensi file dengan aplikasi tertentu
- **Registry**: Database konfigurasi Windows yang menyimpan pengaturan sistem dan aplikasi
- **NSIS**: Nullsoft Scriptable Install System, tool untuk membuat installer Windows
- **ProgID**: Programmatic Identifier, identifier unik untuk aplikasi di Windows Registry

## Requirements

### Requirement 1

**User Story:** As a user, I want to install xNote using a standard Windows installer, so that I can easily set up the application on my computer.

#### Acceptance Criteria

1. WHEN a user runs the installer THEN the Installer SHALL copy xNote executable and required DLLs to the Program Files directory
2. WHEN installation completes THEN the Installer SHALL create a Start Menu shortcut for xNote
3. WHEN installation completes THEN the Installer SHALL create a Desktop shortcut if the user opts in
4. WHEN installation completes THEN the Installer SHALL register xNote in Windows Add/Remove Programs
5. WHEN a user runs the uninstaller THEN the Installer SHALL remove all installed files, shortcuts, and registry entries

### Requirement 2

**User Story:** As a user, I want to set xNote as my default text editor, so that text files automatically open with xNote.

#### Acceptance Criteria

1. WHEN installation completes THEN the Installer SHALL register xNote as an available handler for .txt files
2. WHEN installation completes THEN the Installer SHALL register xNote as an available handler for .log files
3. WHEN installation completes THEN the Installer SHALL register xNote as an available handler for common text file extensions (.ini, .cfg, .md, .json, .xml, .html, .css, .js, .c, .h, .py)
4. WHEN a user chooses to set xNote as default THEN the Installer SHALL configure Windows to open registered file types with xNote
5. WHEN a user right-clicks a text file THEN Windows SHALL show "Open with xNote" in the context menu

### Requirement 3

**User Story:** As a user, I want xNote to open files passed as command line arguments, so that file associations work correctly.

#### Acceptance Criteria

1. WHEN xNote is launched with a file path argument THEN xNote SHALL open that file automatically
2. WHEN xNote is launched with multiple file path arguments THEN xNote SHALL open the first file
3. WHEN xNote is launched with an invalid file path THEN xNote SHALL display an error message and start with an empty document
4. WHEN a user double-clicks an associated file THEN Windows SHALL launch xNote with the file path as argument

### Requirement 4

**User Story:** As a user, I want the installer to handle updates cleanly, so that I can upgrade xNote without losing settings.

#### Acceptance Criteria

1. WHEN a user runs the installer over an existing installation THEN the Installer SHALL preserve user settings
2. WHEN upgrading THEN the Installer SHALL replace only application files without affecting user data
3. WHEN the installer detects a running instance of xNote THEN the Installer SHALL prompt the user to close xNote before proceeding

### Requirement 5

**User Story:** As a developer, I want a build script that creates the installer, so that releases can be automated.

#### Acceptance Criteria

1. WHEN a developer runs the installer build script THEN the Build System SHALL compile xNote if not already built
2. WHEN a developer runs the installer build script THEN the Build System SHALL package all required files into a single installer executable
3. WHEN the installer is built THEN the Build System SHALL include version information in the installer filename
