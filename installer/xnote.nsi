; xNote Installer Script
; NSIS (Nullsoft Scriptable Install System)
; 
; This script creates a Windows installer for xNote text editor
; with file associations and Start Menu integration.

;--------------------------------
; Includes

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

;--------------------------------
; General Configuration

!define PRODUCT_NAME "xNote"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "xNote Team"
!define PRODUCT_WEB_SITE "https://github.com/xnote"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\xnote.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; ProgID for file associations
!define PRODUCT_PROGID "xNote.TextFile"

; Installer attributes
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "xnote-setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin

;--------------------------------
; MUI Settings

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page (optional - uncomment if you have a license file)
; !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"

; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Components page
!insertmacro MUI_PAGE_COMPONENTS

; Install files page
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\xnote.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch xNote"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Indonesian"


;--------------------------------
; Installer Sections

Section "xNote Core Files (required)" SEC_CORE
    SectionIn RO ; Read-only, always installed
    
    ; Check if xNote is running
    FindWindow $0 "NotepadMainWindow" ""
    ${If} $0 != 0
        MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
            "xNote is currently running. Please close it before continuing.$\n$\nClick OK to continue anyway or Cancel to abort." \
            IDOK continue_install
        Abort
        continue_install:
    ${EndIf}
    
    ; Set output path to installation directory
    SetOutPath "$INSTDIR"
    
    ; Copy main executable
    File "..\xnote.exe"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    ; Write App Paths registry key
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\xnote.exe"
    
    ; Write uninstall information to registry
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\xnote.exe,0"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
    
    ; Get installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Start Menu Shortcuts" SEC_STARTMENU
    ; Create Start Menu folder
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    
    ; Create shortcuts
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\xnote.exe" "" "$INSTDIR\xnote.exe" 0
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Desktop Shortcut" SEC_DESKTOP
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\xnote.exe" "" "$INSTDIR\xnote.exe" 0
SectionEnd


Section "File Associations" SEC_FILEASSOC
    ; Register ProgID
    WriteRegStr HKCR "${PRODUCT_PROGID}" "" "Text File"
    WriteRegStr HKCR "${PRODUCT_PROGID}\DefaultIcon" "" "$INSTDIR\xnote.exe,0"
    WriteRegStr HKCR "${PRODUCT_PROGID}\shell" "" "open"
    WriteRegStr HKCR "${PRODUCT_PROGID}\shell\open" "" "Open with xNote"
    WriteRegStr HKCR "${PRODUCT_PROGID}\shell\open\command" "" '"$INSTDIR\xnote.exe" "%1"'
    
    ; Register under Applications for "Open with" menu
    WriteRegStr HKCR "Applications\xnote.exe" "" ""
    WriteRegStr HKCR "Applications\xnote.exe\shell\open\command" "" '"$INSTDIR\xnote.exe" "%1"'
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".txt" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".log" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".ini" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".cfg" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".md" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".json" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".xml" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".html" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".css" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".js" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".c" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".h" ""
    WriteRegStr HKCR "Applications\xnote.exe\SupportedTypes" ".py" ""
    
    ; Register file extensions with OpenWithProgids
    WriteRegStr HKCR ".txt\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".log\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".ini\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".cfg\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".md\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".json\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".xml\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".html\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".css\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".js\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".c\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".h\OpenWithProgids" "${PRODUCT_PROGID}" ""
    WriteRegStr HKCR ".py\OpenWithProgids" "${PRODUCT_PROGID}" ""
    
    ; Notify shell of changes
    System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd

Section "Set as Default Text Editor" SEC_DEFAULT
    ; Set xNote as default handler for text files
    WriteRegStr HKCR ".txt" "" "${PRODUCT_PROGID}"
    WriteRegStr HKCR ".log" "" "${PRODUCT_PROGID}"
    
    ; Notify shell of changes
    System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd


;--------------------------------
; Section Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE} "Core xNote application files (required)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} "Create Start Menu shortcuts"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "Create Desktop shortcut"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_FILEASSOC} "Register xNote for text file types (enables 'Open with xNote' in context menu)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DEFAULT} "Set xNote as the default application for .txt and .log files"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section

Section "Uninstall"
    ; Remove files
    Delete "$INSTDIR\xnote.exe"
    Delete "$INSTDIR\uninstall.exe"
    
    ; Remove installation directory (if empty)
    RMDir "$INSTDIR"
    
    ; Remove Start Menu shortcuts
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    
    ; Remove Desktop shortcut
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    
    ; Remove registry keys
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    
    ; Remove ProgID
    DeleteRegKey HKCR "${PRODUCT_PROGID}"
    
    ; Remove Applications registration
    DeleteRegKey HKCR "Applications\xnote.exe"
    
    ; Remove OpenWithProgids entries (don't delete the extension keys, just our entries)
    DeleteRegValue HKCR ".txt\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".log\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".ini\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".cfg\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".md\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".json\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".xml\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".html\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".css\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".js\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".c\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".h\OpenWithProgids" "${PRODUCT_PROGID}"
    DeleteRegValue HKCR ".py\OpenWithProgids" "${PRODUCT_PROGID}"
    
    ; Reset default associations if they were set to xNote
    ReadRegStr $0 HKCR ".txt" ""
    ${If} $0 == "${PRODUCT_PROGID}"
        DeleteRegValue HKCR ".txt" ""
    ${EndIf}
    
    ReadRegStr $0 HKCR ".log" ""
    ${If} $0 == "${PRODUCT_PROGID}"
        DeleteRegValue HKCR ".log" ""
    ${EndIf}
    
    ; Notify shell of changes
    System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
SectionEnd

;--------------------------------
; Installer Functions

Function .onInit
    ; Check for existing installation
    ReadRegStr $0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation"
    ${If} $0 != ""
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "xNote is already installed at:$\n$0$\n$\nDo you want to upgrade/reinstall?" \
            IDYES continue_init
        Abort
        continue_init:
        StrCpy $INSTDIR $0
    ${EndIf}
FunctionEnd

Function un.onInit
    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Are you sure you want to uninstall ${PRODUCT_NAME}?" \
        IDYES continue_uninit
    Abort
    continue_uninit:
FunctionEnd
