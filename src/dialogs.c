#include "notepad.h"

/* File filter for various file types */
static const TCHAR szFilter[] = 
    TEXT("All Supported Files\0*.txt;*.md;*.json;*.xml;*.html;*.htm;*.css;*.js;*.ts;*.py;*.c;*.cpp;*.h;*.hpp;*.java;*.go;*.rs;*.sql;*.yaml;*.yml;*.sh;*.bat;*.ps1;*.php;*.rb;*.log;*.ini;*.cfg;*.conf\0")
    TEXT("Text Files (*.txt)\0*.txt\0")
    TEXT("Markdown (*.md)\0*.md\0")
    TEXT("JSON (*.json)\0*.json\0")
    TEXT("XML (*.xml)\0*.xml\0")
    TEXT("HTML (*.html, *.htm)\0*.html;*.htm\0")
    TEXT("CSS (*.css)\0*.css\0")
    TEXT("JavaScript (*.js)\0*.js\0")
    TEXT("TypeScript (*.ts)\0*.ts\0")
    TEXT("Python (*.py)\0*.py\0")
    TEXT("C/C++ (*.c, *.cpp, *.h, *.hpp)\0*.c;*.cpp;*.h;*.hpp\0")
    TEXT("Java (*.java)\0*.java\0")
    TEXT("Go (*.go)\0*.go\0")
    TEXT("Rust (*.rs)\0*.rs\0")
    TEXT("SQL (*.sql)\0*.sql\0")
    TEXT("YAML (*.yaml, *.yml)\0*.yaml;*.yml\0")
    TEXT("Shell Script (*.sh)\0*.sh\0")
    TEXT("Batch (*.bat, *.cmd)\0*.bat;*.cmd\0")
    TEXT("PowerShell (*.ps1)\0*.ps1\0")
    TEXT("PHP (*.php)\0*.php\0")
    TEXT("Ruby (*.rb)\0*.rb\0")
    TEXT("Log Files (*.log)\0*.log\0")
    TEXT("Config Files (*.ini, *.cfg, *.conf)\0*.ini;*.cfg;*.conf\0")
    TEXT("All Files (*.*)\0*.*\0\0");

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
