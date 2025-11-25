#include "notepad.h"

/* File filter for text files */
static const TCHAR szFilter[] = TEXT("Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0");

/* Show Open File dialog */
BOOL ShowOpenDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile) {
    OPENFILENAME ofn = {0};
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nMaxFile;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("txt");
    
    return GetOpenFileName(&ofn);
}

/* Show Save File dialog */
BOOL ShowSaveDialog(HWND hwnd, TCHAR* szFileName, DWORD nMaxFile) {
    OPENFILENAME ofn = {0};
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nMaxFile;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = TEXT("txt");
    
    return GetSaveFileName(&ofn);
}

/* Show confirmation dialog for saving changes */
int ShowConfirmSaveDialog(HWND hwnd) {
    return MessageBox(
        hwnd,
        TEXT("Do you want to save changes?"),
        APP_NAME,
        MB_YESNOCANCEL | MB_ICONQUESTION
    );
}

/* Show About dialog */
void ShowAboutDialog(HWND hwnd) {
    TCHAR szMessage[512];
    _sntprintf(szMessage, 512, 
        TEXT("%s Version %s\n\n")
        TEXT("A fast and lightweight text editor.\n\n")
        TEXT("Features:\n")
        TEXT("  - Multiple tabs with close button\n")
        TEXT("  - Line numbers\n")
        TEXT("  - Large file support\n")
        TEXT("  - UTF-8 encoding\n")
        TEXT("  - Word wrap toggle\n\n")
        TEXT("Shortcuts:\n")
        TEXT("  Ctrl+T: New Tab\n")
        TEXT("  Ctrl+W: Close Tab\n")
        TEXT("  Ctrl+N: New Document"),
        APP_NAME, APP_VERSION);
    
    MessageBox(
        hwnd,
        szMessage,
        TEXT("About XNote"),
        MB_OK | MB_ICONINFORMATION
    );
}

/* Show error dialog */
void ShowErrorDialog(HWND hwnd, const TCHAR* szMessage) {
    MessageBox(
        hwnd,
        szMessage,
        TEXT("Error"),
        MB_OK | MB_ICONERROR
    );
}
