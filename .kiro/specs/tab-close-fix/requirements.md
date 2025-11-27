# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk memperbaiki bug close tab di XNote. Bug yang dilaporkan adalah tab tidak bisa ditutup dengan benar, baik melalui klik tombol close (X) maupun shortcut Ctrl+W. Perbaikan ini juga akan memastikan semua fitur close tab berfungsi dengan baik tanpa bug atau glitch.

## Glossary

- **Tab Control**: Komponen Windows yang menampilkan tab-tab dokumen
- **Close Button**: Tombol X pada setiap tab untuk menutup tab tersebut
- **TabSubclassProc**: Prosedur window yang di-subclass untuk menangani event mouse pada tab
- **Accelerator**: Shortcut keyboard yang terdaftar di accelerator table
- **Hit Test**: Proses menentukan area mana yang diklik oleh mouse

## Requirements

### Requirement 1

**User Story:** As a user, I want to close tabs using the close button (X), so that I can quickly close documents I no longer need.

#### Acceptance Criteria

1. WHEN a user clicks on the close button (X) area of a tab THEN the XNote SHALL close that specific tab
2. WHEN a user clicks outside the close button area but inside the tab THEN the XNote SHALL NOT close the tab but switch to it instead
3. WHEN the close button is clicked THEN the XNote SHALL properly detect the click within the correct X and Y boundaries
4. WHEN a tab is closed THEN the XNote SHALL properly clean up all resources associated with that tab

### Requirement 2

**User Story:** As a user, I want to close tabs using keyboard shortcut Ctrl+W, so that I can quickly close the current tab without using the mouse.

#### Acceptance Criteria

1. WHEN a user presses Ctrl+W THEN the XNote SHALL close the currently active tab
2. WHEN Ctrl+W is pressed and there are unsaved changes THEN the XNote SHALL prompt the user to save before closing
3. WHEN Ctrl+W is pressed and only one tab remains THEN the XNote SHALL create a new untitled tab after closing

### Requirement 3

**User Story:** As a user, I want the close button hover effect to work correctly, so that I can see visual feedback when hovering over the close button.

#### Acceptance Criteria

1. WHEN a user hovers over the close button area THEN the XNote SHALL display a red background highlight
2. WHEN a user moves the mouse away from the close button THEN the XNote SHALL remove the highlight
3. WHEN the mouse leaves the tab control area THEN the XNote SHALL reset all hover states

### Requirement 4

**User Story:** As a user, I want consistent tab closing behavior, so that I can rely on the close functionality working the same way every time.

#### Acceptance Criteria

1. WHEN closing a tab THEN the XNote SHALL switch to the appropriate adjacent tab
2. WHEN the last tab is closed THEN the XNote SHALL create a new untitled tab
3. WHEN multiple tabs exist and one is closed THEN the XNote SHALL maintain correct tab indices
4. WHEN a tab is closed THEN the XNote SHALL update the session state to reflect the change
