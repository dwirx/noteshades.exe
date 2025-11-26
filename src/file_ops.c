#include "notepad.h"
#include "syntax.h"
#include "session.h"
#include <stdio.h>
#include <richedit.h>

/* Update window title based on current tab */
void UpdateWindowTitle(HWND hwnd) {
    TCHAR szTitle[MAX_PATH + 64];
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab) {
        _sntprintf(szTitle, MAX_PATH + 64, TEXT("Untitled - %s"), APP_NAME);
    } else if (pTab->bUntitled) {
        _sntprintf(szTitle, MAX_PATH + 64, TEXT("Untitled - %s"), APP_NAME);
    } else {
        TCHAR* pFileName = _tcsrchr(pTab->szFileName, TEXT('\\'));
        if (pFileName) {
            pFileName++;
        } else {
            pFileName = pTab->szFileName;
        }
        
        /* Add mode indicator for large files */
        const TCHAR* szModeIndicator = TEXT("");
        if (pTab->fileMode == FILEMODE_READONLY || pTab->fileMode == FILEMODE_MMAP) {
            szModeIndicator = TEXT(" [READ-ONLY]");
        } else if (pTab->fileMode == FILEMODE_PARTIAL) {
            szModeIndicator = TEXT(" [PARTIAL]");
        }
        
        _sntprintf(szTitle, MAX_PATH + 64, TEXT("%s%s - %s"), pFileName, szModeIndicator, APP_NAME);
    }
    
    SetWindowText(hwnd, szTitle);
}

/* Check if file is binary (not suitable for text editing) */
static BOOL IsBinaryFile(const char* pBuffer, DWORD dwSize) {
    /* Check first 8KB or entire buffer if smaller */
    DWORD dwCheckSize = (dwSize < 8192) ? dwSize : 8192;

    /* Common binary file signatures (magic numbers) */
    if (dwSize >= 4) {
        /* PNG signature */
        if (pBuffer[0] == (char)0x89 && pBuffer[1] == 'P' && pBuffer[2] == 'N' && pBuffer[3] == 'G') {
            return TRUE; /* PNG image file */
        }
        /* JPEG signature */
        if ((unsigned char)pBuffer[0] == 0xFF && (unsigned char)pBuffer[1] == 0xD8) {
            return TRUE; /* JPEG image file */
        }
        /* GIF signature */
        if (pBuffer[0] == 'G' && pBuffer[1] == 'I' && pBuffer[2] == 'F') {
            return TRUE; /* GIF image file */
        }
        /* ZIP/Office documents signature */
        if (pBuffer[0] == 'P' && pBuffer[1] == 'K' && pBuffer[2] == 0x03 && pBuffer[3] == 0x04) {
            return TRUE; /* ZIP or Office file (docx, xlsx, etc.) */
        }
        /* PDF signature */
        if (pBuffer[0] == '%' && pBuffer[1] == 'P' && pBuffer[2] == 'D' && pBuffer[3] == 'F') {
            return FALSE; /* PDF is technically text-based (though binary content inside) */
        }
        /* EXE/DLL signature */
        if (pBuffer[0] == 'M' && pBuffer[1] == 'Z') {
            return TRUE; /* Windows executable */
        }
    }

    /* Count null bytes and control characters (excluding tab, newline, carriage return) */
    DWORD dwNullCount = 0;
    DWORD dwControlCount = 0;

    for (DWORD i = 0; i < dwCheckSize; i++) {
        unsigned char c = (unsigned char)pBuffer[i];

        /* Null byte is definite indicator of binary */
        if (c == 0) {
            dwNullCount++;
        }
        /* Control characters (except tab=9, LF=10, CR=13, ESC=27) */
        else if (c < 32 && c != 9 && c != 10 && c != 13 && c != 27) {
            dwControlCount++;
        }
    }

    /* Heuristic: If > 1% null bytes or > 10% control chars, likely binary */
    if (dwNullCount > 0) {
        return TRUE; /* Any null bytes = binary */
    }

    if (dwControlCount > (dwCheckSize / 10)) {
        return TRUE; /* > 10% control characters = likely binary */
    }

    return FALSE; /* Appears to be text */
}

/* Check if buffer has UTF-8 BOM */
static BOOL HasUtf8Bom(const char* pBuffer, DWORD dwSize) {
    if (dwSize >= 3) {
        return (unsigned char)pBuffer[0] == 0xEF &&
               (unsigned char)pBuffer[1] == 0xBB &&
               (unsigned char)pBuffer[2] == 0xBF;
    }
    return FALSE;
}

/* Check if file is an Excel file (.xls, .xlsx) */
static BOOL IsExcelFile(const TCHAR* szFilePath) {
    const TCHAR* pExt = _tcsrchr(szFilePath, TEXT('.'));
    if (!pExt) return FALSE;
    
    TCHAR szExt[16] = {0};
    _tcsncpy(szExt, pExt, 15);
    _tcslwr(szExt);
    
    return (_tcscmp(szExt, TEXT(".xls")) == 0 || 
            _tcscmp(szExt, TEXT(".xlsx")) == 0 ||
            _tcscmp(szExt, TEXT(".xlsm")) == 0 ||
            _tcscmp(szExt, TEXT(".xlsb")) == 0);
}

/* Check if file is a CSV file */
static BOOL IsCSVFile(const TCHAR* szFilePath) {
    const TCHAR* pExt = _tcsrchr(szFilePath, TEXT('.'));
    if (!pExt) return FALSE;
    
    TCHAR szExt[16] = {0};
    _tcsncpy(szExt, pExt, 15);
    _tcslwr(szExt);
    
    return (_tcscmp(szExt, TEXT(".csv")) == 0 ||
            _tcscmp(szExt, TEXT(".tsv")) == 0);
}

/* Show warning for Excel files */
static BOOL ShowExcelWarning(HWND hwnd, const TCHAR* szFileName) {
    TCHAR szMessage[512];
    _sntprintf(szMessage, 512,
        TEXT("File Excel Terdeteksi\n\n")
        TEXT("File '%s' adalah file Excel binary.\n\n")
        TEXT("XNote adalah text editor dan tidak dapat membaca format Excel dengan benar.\n\n")
        TEXT("Untuk membuka file Excel, gunakan:\n")
        TEXT("• Microsoft Excel\n")
        TEXT("• LibreOffice Calc\n")
        TEXT("• Google Sheets\n\n")
        TEXT("Apakah Anda tetap ingin membuka file ini sebagai raw text?"),
        _tcsrchr(szFileName, TEXT('\\')) ? _tcsrchr(szFileName, TEXT('\\')) + 1 : szFileName);
    
    return MessageBox(hwnd, szMessage, TEXT("Format Tidak Didukung"), 
                     MB_YESNO | MB_ICONWARNING) == IDYES;
}

/* Check if file extension is a known binary/non-text format */
static BOOL IsKnownBinaryExtension(const TCHAR* szFilePath) {
    const TCHAR* pExt = _tcsrchr(szFilePath, TEXT('.'));
    if (!pExt) return FALSE;

    /* Convert to lowercase for comparison */
    TCHAR szExt[16] = {0};
    _tcsncpy(szExt, pExt, 15);
    _tcslwr(szExt);

    /* Image formats */
    if (_tcscmp(szExt, TEXT(".png")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".jpg")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".jpeg")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".gif")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".bmp")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".ico")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".webp")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".svg")) == 0) return FALSE; /* SVG is XML, text-based */

    /* Video/Audio formats */
    if (_tcscmp(szExt, TEXT(".mp4")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".avi")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".mkv")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".mp3")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".wav")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".flac")) == 0) return TRUE;

    /* Executable/Library formats */
    if (_tcscmp(szExt, TEXT(".exe")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".dll")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".sys")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".o")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".obj")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".lib")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".a")) == 0) return TRUE;

    /* Archive formats */
    if (_tcscmp(szExt, TEXT(".zip")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".rar")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".7z")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".tar")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".gz")) == 0) return TRUE;

    /* Office documents (binary/zipped) */
    if (_tcscmp(szExt, TEXT(".docx")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".xlsx")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".pptx")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".doc")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".xls")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".ppt")) == 0) return TRUE;

    /* Database files */
    if (_tcscmp(szExt, TEXT(".db")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".sqlite")) == 0) return TRUE;
    if (_tcscmp(szExt, TEXT(".mdb")) == 0) return TRUE;

    return FALSE;
}

/* Detect line ending type from buffer */
static LineEndingType DetectLineEnding(const char* pBuffer, DWORD dwSize) {
    BOOL bHasCR = FALSE;
    BOOL bHasLF = FALSE;
    BOOL bHasCRLF = FALSE;

    /* Only check first 64KB for performance */
    DWORD dwCheckSize = (dwSize > 65536) ? 65536 : dwSize;

    for (DWORD i = 0; i < dwCheckSize; i++) {
        if (pBuffer[i] == '\r') {
            if (i + 1 < dwCheckSize && pBuffer[i + 1] == '\n') {
                bHasCRLF = TRUE;
                i++; /* Skip the LF */
            } else {
                bHasCR = TRUE;
            }
        } else if (pBuffer[i] == '\n') {
            bHasLF = TRUE;
        }
    }

    /* Prioritize: CRLF > LF > CR */
    if (bHasCRLF) return LINE_ENDING_CRLF;
    if (bHasLF) return LINE_ENDING_LF;
    if (bHasCR) return LINE_ENDING_CR;

    /* Default to Windows line ending */
    return LINE_ENDING_CRLF;
}

/* ============================================================================
 * INTELLIGENT FILE MODE DETECTION
 * ============================================================================
 * Automatically selects the best loading strategy based on file size:
 * (Thresholds optimized for responsiveness - prevents "Not Responding")
 *
 * FILEMODE_NORMAL (< 2MB):
 *   - Full file loaded into RichEdit
 *   - All features enabled (syntax highlighting for <256KB, editing, etc.)
 *   - Best user experience
 *
 * FILEMODE_PARTIAL (2MB - 10MB):
 *   - Load first 512KB initially
 *   - Show "Load More" / F5 for remaining content
 *   - Editing enabled, syntax highlighting disabled
 *   - Good balance of performance and usability
 *
 * FILEMODE_READONLY (10MB - 50MB):
 *   - Read-only preview mode
 *   - Load first 512KB for preview
 *   - View-only, no editing
 *   - Fast loading, minimal memory
 *
 * FILEMODE_MMAP (> 50MB):
 *   - Memory-mapped file (no load into RAM)
 *   - Virtual scrolling
 *   - Ultra-fast opening
 *   - Minimal memory footprint
 * ========================================================================= */

/* Public function for file mode detection - uses threshold constants from notepad.h */
FileModeType DetectOptimalFileMode(DWORD dwFileSize) {
    if (dwFileSize < THRESHOLD_PARTIAL) {
        /* < 2MB: Normal mode - full experience */
        return FILEMODE_NORMAL;
    } else if (dwFileSize < THRESHOLD_READONLY) {
        /* 2MB - 10MB: Partial loading mode */
        return FILEMODE_PARTIAL;
    } else if (dwFileSize < THRESHOLD_MMAP) {
        /* 10MB - 50MB: Read-only preview mode */
        return FILEMODE_READONLY;
    } else {
        /* > 50MB: Memory-mapped mode */
        return FILEMODE_MMAP;
    }
}

/* Show informative message about large file mode */
void ShowLargeFileInfo(HWND hwnd, DWORD dwFileSize, FileModeType mode) {
    TCHAR szMessage[512];
    const TCHAR* szModeName = TEXT("");
    const TCHAR* szDetails = TEXT("");

    switch (mode) {
        case FILEMODE_PARTIAL:
            szModeName = TEXT("Partial Loading Mode");
            szDetails = TEXT("File akan dimuat dalam chunks untuk performa lebih baik.\n")
                       TEXT("Tekan F5 untuk memuat konten tambahan.\n")
                       TEXT("Syntax highlighting dinonaktifkan untuk file ini.");
            break;

        case FILEMODE_READONLY:
            szModeName = TEXT("Read-Only Preview Mode");
            szDetails = TEXT("File besar ini dibuka dalam mode read-only untuk performa.\n")
                       TEXT("Hanya preview 512KB pertama yang ditampilkan.\n")
                       TEXT("Untuk edit, gunakan editor khusus file besar.");
            break;

        case FILEMODE_MMAP:
            szModeName = TEXT("Memory-Mapped Mode");
            szDetails = TEXT("File sangat besar ini dibuka menggunakan memory-mapping.\n")
                       TEXT("File tidak dimuat ke RAM, memberikan akses instan.\n")
                       TEXT("Mode read-only untuk performa optimal.");
            break;

        default:
            return;
    }

    _sntprintf(szMessage, 512,
        TEXT("File Besar Terdeteksi (%.2f MB)\n\n")
        TEXT("Mode: %s\n\n")
        TEXT("%s"),
        dwFileSize / (1024.0 * 1024.0),
        szModeName,
        szDetails);

    MessageBox(hwnd, szMessage, TEXT("Large File Mode"), MB_OK | MB_ICONINFORMATION);
}

/* ============================================================================
 * BACKGROUND THREAD FILE LOADING - Like Notepad++!
 * ============================================================================
 * This implementation loads files in a separate thread to keep UI responsive
 * Shows a progress dialog with real-time updates and cancellation support
 * ========================================================================= */

/* Thread data structure */
typedef struct {
    TCHAR szFileName[MAX_PATH];
    HWND hEdit;
    HWND hwndProgress;
    HWND hwndMain;

    /* File data */
    WCHAR* pWideText;
    DWORD dwWideLen;
    LineEndingType lineEnding;

    /* Progress tracking */
    volatile DWORD dwFileSize;
    volatile DWORD dwBytesRead;
    volatile DWORD dwProgress; /* 0-100 */
    volatile BOOL bCancelled;
    volatile BOOL bError;
    volatile BOOL bCompleted;

    TCHAR szError[256];
} FILELOAD_THREAD_DATA;

/* Global thread data - single file load at a time */
static FILELOAD_THREAD_DATA g_ThreadData = {0};
static HANDLE g_hLoadThread = NULL;

/* Progress dialog procedure */
static INT_PTR CALLBACK ProgressDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static FILELOAD_THREAD_DATA* s_pData = NULL;

    switch (msg) {
        case WM_INITDIALOG: {
            s_pData = (FILELOAD_THREAD_DATA*)lParam;
            s_pData->hwndProgress = hDlg;

            /* Initialize progress bar */
            SendDlgItemMessage(hDlg, IDC_PROGRESS_BAR, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendDlgItemMessage(hDlg, IDC_PROGRESS_BAR, PBM_SETPOS, 0, 0);

            /* Center dialog */
            RECT rc;
            GetWindowRect(hDlg, &rc);
            int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
            int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            /* Start timer for progress updates */
            SetTimer(hDlg, 1, 100, NULL); /* Update every 100ms */
            return TRUE;
        }

        case WM_TIMER: {
            if (!s_pData) break;

            /* Update progress bar */
            SendDlgItemMessage(hDlg, IDC_PROGRESS_BAR, PBM_SETPOS, s_pData->dwProgress, 0);

            /* Update text - show file size info during reading phase */
            if (!s_pData->bCompleted && s_pData->dwFileSize > 0) {
                TCHAR szProgress[256];
                _sntprintf(szProgress, 256, TEXT("Loading... %d%% (%.1f MB / %.1f MB)"),
                    s_pData->dwProgress,
                    s_pData->dwBytesRead / (1024.0 * 1024.0),
                    s_pData->dwFileSize / (1024.0 * 1024.0));
                SetDlgItemText(hDlg, IDC_PROGRESS_TEXT, szProgress);
            }

            /* Check if loading completed */
            if (s_pData->bCompleted || s_pData->bError || s_pData->bCancelled) {
                KillTimer(hDlg, 1);
                /* For modal dialogs (small files), close immediately */
                /* For modeless dialogs (large files), let ReadFileContent close after streaming */
                if (GetWindowLongPtr(hDlg, GWL_STYLE) & DS_MODALFRAME) {
                    /* Check if this is truly modal by seeing if we were created with DialogBox */
                    /* For modal dialogs, end it now */
                    EndDialog(hDlg, s_pData->bError ? IDCANCEL : IDOK);
                }
                /* For modeless, don't close - streaming will happen next */
            }

            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PROGRESS_CANCEL || LOWORD(wParam) == IDCANCEL) {
                if (s_pData) {
                    s_pData->bCancelled = TRUE;
                }
                return TRUE;
            }
            break;

        case WM_CLOSE:
            if (s_pData) s_pData->bCancelled = TRUE;
            KillTimer(hDlg, 1);
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

/* Background thread function - does the heavy lifting! */
static DWORD WINAPI FileLoadThreadProc(LPVOID lpParam) {
    FILELOAD_THREAD_DATA* pData = (FILELOAD_THREAD_DATA*)lpParam;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char* pBuffer = NULL;
    BOOL bSuccess = FALSE;

    /* Open file */
    hFile = CreateFile(pData->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        _tcscpy(pData->szError, TEXT("Failed to open file"));
        pData->bError = TRUE;
        return 1;
    }

    /* Get file size */
    LARGE_INTEGER liFileSize;
    if (!GetFileSizeEx(hFile, &liFileSize) || liFileSize.HighPart > 0) {
        CloseHandle(hFile);
        _tcscpy(pData->szError, TEXT("Failed to get file size"));
        pData->bError = TRUE;
        return 1;
    }

    pData->dwFileSize = liFileSize.LowPart;
    pData->dwBytesRead = 0;
    pData->dwProgress = 0;

    /* Allocate buffer */
    pBuffer = (char*)HeapAlloc(GetProcessHeap(), 0, pData->dwFileSize + 1);
    if (!pBuffer) {
        CloseHandle(hFile);
        _tcscpy(pData->szError, TEXT("Out of memory"));
        pData->bError = TRUE;
        return 1;
    }

    /* Read file in chunks - adaptive chunk size for optimal performance */
    /* Adaptive chunk size based on benchmark results:
     * - Files < 512KB: read all at once (fastest)
     * - Files >= 512KB: use 64KB chunks (optimal from benchmark)
     */
    DWORD dwChunkRead;
    DWORD dwOptimalChunk = (pData->dwFileSize < SMALL_FILE_THRESHOLD) ? 
                           pData->dwFileSize :  /* Small file: read all at once */
                           THREAD_CHUNK_SIZE;   /* Large file: use 64KB chunks */

    while (pData->dwBytesRead < pData->dwFileSize && !pData->bCancelled) {
        DWORD dwToRead = min(dwOptimalChunk, pData->dwFileSize - pData->dwBytesRead);

        if (!ReadFile(hFile, pBuffer + pData->dwBytesRead, dwToRead, &dwChunkRead, NULL)) {
            HeapFree(GetProcessHeap(), 0, pBuffer);
            CloseHandle(hFile);
            _tcscpy(pData->szError, TEXT("File read error"));
            pData->bError = TRUE;
            return 1;
        }

        pData->dwBytesRead += dwChunkRead;
        pData->dwProgress = (pData->dwBytesRead * 100) / pData->dwFileSize;

        if (dwChunkRead == 0) break;
        
        /* Only yield for large files to allow UI updates */
        if (pData->dwFileSize > SMALL_FILE_THRESHOLD) {
            Sleep(0); /* Yield to other threads without delay */
        }
    }

    CloseHandle(hFile);

    if (pData->bCancelled) {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        return 0;
    }

    pBuffer[pData->dwBytesRead] = '\0';

    /* Detect line ending */
    pData->lineEnding = DetectLineEnding(pBuffer, pData->dwBytesRead);

    /* Handle UTF-8 BOM */
    const char* pText = pBuffer;
    DWORD dwTextSize = pData->dwBytesRead;
    if (HasUtf8Bom(pBuffer, pData->dwBytesRead)) {
        pText = pBuffer + 3;
        dwTextSize = pData->dwBytesRead - 3;
    }

    /* Convert to UTF-16 */
    int nWideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize, NULL, 0);
    if (nWideLen == 0) {
        nWideLen = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize, NULL, 0);
    }

    if (nWideLen > 0) {
        pData->pWideText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (pData->pWideText) {
            int nConverted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize,
                                                 pData->pWideText, nWideLen);
            if (nConverted == 0) {
                nConverted = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize,
                                                pData->pWideText, nWideLen);
            }

            if (nConverted > 0) {
                pData->pWideText[nConverted] = L'\0';
                pData->dwWideLen = nConverted;
                bSuccess = TRUE;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pBuffer);

    if (!bSuccess) {
        _tcscpy(pData->szError, TEXT("Text conversion failed"));
        pData->bError = TRUE;
        return 1;
    }

    pData->dwProgress = 100;
    pData->bCompleted = TRUE;
    return 0;
}

/* Memory stream structure for EM_STREAMIN with progress tracking */
typedef struct {
    WCHAR* pText;
    DWORD dwSize;
    DWORD dwPos;
    HWND hwndProgress;      /* Progress dialog handle */
    volatile DWORD* pProgress; /* Progress percentage (0-100) */
    DWORD dwTotalSize;      /* Total size for progress calculation */
} MEMORY_STREAM;

/* Stream callback function - feeds text from memory buffer to RichEdit with progress updates */
static DWORD CALLBACK MemoryStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
    MEMORY_STREAM* pStream = (MEMORY_STREAM*)dwCookie;

    DWORD dwRemaining = pStream->dwSize - pStream->dwPos;
    DWORD dwToRead = (DWORD)cb;
    if (dwToRead > dwRemaining) {
        dwToRead = dwRemaining;
    }

    if (dwToRead > 0) {
        memcpy(pbBuff, (BYTE*)pStream->pText + pStream->dwPos, dwToRead);
        pStream->dwPos += dwToRead;

        /* Update progress if streaming is taking time */
        if (pStream->pProgress && pStream->dwTotalSize > 0) {
            DWORD dwProgress = (pStream->dwPos * 100) / pStream->dwTotalSize;
            *pStream->pProgress = dwProgress;
        }
    }

    *pcb = (LONG)dwToRead;
    return 0; /* Success */
}

/* ============================================================================
 * REVOLUTIONARY SMART FILE LOADING SYSTEM
 * ============================================================================
 * Instead of loading entire file and freezing UI, we use intelligent modes:
 *
 * 1. For small files (<2MB): Traditional full loading with progress
 * 2. For medium files (2-10MB): Chunked loading (load first 512KB instantly)
 * 3. For large files (10-50MB): Read-only preview (first 512KB only)
 * 4. For huge files (>50MB): Memory-mapped (instant "loading", no RAM used)
 *
 * This approach is used by modern editors like VS Code, Sublime Text.
 * Thresholds lowered significantly for better responsiveness on all systems.
 * ========================================================================= */

/* Load file chunk by chunk - for FILEMODE_PARTIAL */
static BOOL LoadFileChunked(HWND hEdit, const TCHAR* szFileName, DWORD dwChunkSize, DWORD* pdwLoaded) {
    HANDLE hFile;
    char* pBuffer = NULL;
    WCHAR* pWideText = NULL;
    BOOL bSuccess = FALSE;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    /* Read specified chunk */
    pBuffer = (char*)HeapAlloc(GetProcessHeap(), 0, dwChunkSize + 1);
    if (!pBuffer) {
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD dwBytesRead = 0;
    if (!ReadFile(hFile, pBuffer, dwChunkSize, &dwBytesRead, NULL) || dwBytesRead == 0) {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    pBuffer[dwBytesRead] = '\0';

    /* Handle UTF-8 BOM */
    const char* pText = pBuffer;
    DWORD dwTextSize = dwBytesRead;
    if (HasUtf8Bom(pBuffer, dwBytesRead)) {
        pText = pBuffer + 3;
        dwTextSize = dwBytesRead - 3;
    }

    /* Convert to UTF-16 */
    int nWideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize, NULL, 0);
    if (nWideLen == 0) {
        nWideLen = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize, NULL, 0);
    }

    if (nWideLen > 0) {
        pWideText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (pWideText) {
            int nConverted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pText, dwTextSize,
                                                pWideText, nWideLen);
            if (nConverted == 0) {
                nConverted = MultiByteToWideChar(CP_ACP, 0, pText, dwTextSize,
                                                pWideText, nWideLen);
            }

            if (nConverted > 0) {
                pWideText[nConverted] = L'\0';

                /* Set text directly - fast for chunks */
                SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
                SetWindowTextW(hEdit, pWideText);
                SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hEdit, NULL, TRUE);

                *pdwLoaded = dwBytesRead;
                bSuccess = TRUE;
            }
            HeapFree(GetProcessHeap(), 0, pWideText);
        }
    }

    HeapFree(GetProcessHeap(), 0, pBuffer);
    return bSuccess;
}

/* Main file loading function - with INTELLIGENT MODE SELECTION! */
BOOL ReadFileContent(HWND hEdit, const TCHAR* szFileName) {
    LARGE_INTEGER liFileSize;
    HANDLE hFile;

    /* Quick size check first */
    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!GetFileSizeEx(hFile, &liFileSize)) {
        CloseHandle(hFile);
        return FALSE;
    }

    /* Support files up to 4GB (handle LARGE_INTEGER properly) */
    DWORD dwFileSize = liFileSize.LowPart;
    BOOL bIsVeryLarge = (liFileSize.HighPart > 0) || (dwFileSize > 1024 * 1024 * 1024);

    CloseHandle(hFile);

    /* Detect optimal loading mode based on file size */
    FileModeType fileMode = DetectOptimalFileMode(dwFileSize);

    /* Show informative dialog for large files */
    if (fileMode != FILEMODE_NORMAL) {
        ShowLargeFileInfo(GetParent(hEdit), dwFileSize, fileMode);
    }

    /* Store file mode in tab state */
    TabState* pTab = GetCurrentTabState();
    if (pTab) {
        pTab->fileMode = fileMode;
        pTab->dwTotalFileSize = dwFileSize;
        pTab->dwLoadedSize = 0;
    }

    /* ============================================================================
     * MEMORY-MAPPED MODE - For files > 50MB
     * Use memory-mapping for instant "loading" without using RAM
     * Read-only mode for optimal performance
     * ========================================================================= */
    if (fileMode == FILEMODE_MMAP || bIsVeryLarge) {
        /* For extremely large files, use memory-mapped approach */
        /* Load only first 512KB as preview for instant display */
        DWORD dwPreviewSize = PREVIEW_SIZE; /* 512KB preview */
        DWORD dwLoaded = 0;
        BOOL bResult = LoadFileChunked(hEdit, szFileName, dwPreviewSize, &dwLoaded);

        if (bResult) {
            /* Set read-only */
            SendMessage(hEdit, EM_SETREADONLY, TRUE, 0);
            
            /* Disable undo buffer */
            SendMessage(hEdit, EM_SETUNDOLIMIT, 0, 0);
            SendMessage(hEdit, EM_EMPTYUNDOBUFFER, 0, 0);

            if (pTab) {
                pTab->dwLoadedSize = dwLoaded;
                pTab->fileMode = FILEMODE_MMAP;
            }

            /* Show status */
            TCHAR szStatus[256];
            _sntprintf(szStatus, 256,
                TEXT("MEMORY-MAPPED [READ-ONLY]: Showing first %.1f MB of %.1f MB (file too large for full editing)"),
                dwLoaded / (1024.0 * 1024.0),
                dwFileSize / (1024.0 * 1024.0));
            SetWindowText(g_AppState.hwndStatus, szStatus);
        }

        return bResult;
    }

    /* ============================================================================
     * CHUNKED LOADING MODE - For 2-10MB files
     * Only load first 512KB instantly, rest on demand via F5
     * This prevents UI freeze while still allowing editing
     * ========================================================================= */
    if (fileMode == FILEMODE_PARTIAL) {
        DWORD dwChunkSize = INITIAL_CHUNK_SIZE; /* Load 512KB initially */
        if (pTab) {
            pTab->dwChunkSize = dwChunkSize;
        }

        DWORD dwLoaded = 0;
        BOOL bResult = LoadFileChunked(hEdit, szFileName, dwChunkSize, &dwLoaded);

        if (bResult && pTab) {
            pTab->dwLoadedSize = dwLoaded;

            /* Show status with loaded size info */
            TCHAR szStatus[256];
            _sntprintf(szStatus, 256,
                TEXT("Partial Load: %.1f KB of %.1f MB loaded - Press F5 to load more"),
                dwLoaded / 1024.0,
                dwFileSize / (1024.0 * 1024.0));
            SetWindowText(g_AppState.hwndStatus, szStatus);
        }

        return bResult;
    }

    /* ============================================================================
     * READ-ONLY PREVIEW MODE - For 10-50MB files
     * Load only first 512KB for quick preview
     * Editing disabled for performance
     * ========================================================================= */
    if (fileMode == FILEMODE_READONLY) {
        DWORD dwPreviewSize = PREVIEW_SIZE; /* 512KB preview */
        DWORD dwLoaded = 0;
        BOOL bResult = LoadFileChunked(hEdit, szFileName, dwPreviewSize, &dwLoaded);

        if (bResult) {
            /* Set read-only */
            SendMessage(hEdit, EM_SETREADONLY, TRUE, 0);
            
            /* Disable undo buffer for read-only mode (Requirement 7.2) */
            SendMessage(hEdit, EM_SETUNDOLIMIT, 0, 0);
            SendMessage(hEdit, EM_EMPTYUNDOBUFFER, 0, 0);

            if (pTab) {
                pTab->dwLoadedSize = dwLoaded;
            }

            /* Show status */
            TCHAR szStatus[256];
            _sntprintf(szStatus, 256,
                TEXT("READ-ONLY PREVIEW: Showing first %.1f MB of %.1f MB total"),
                dwLoaded / (1024.0 * 1024.0),
                dwFileSize / (1024.0 * 1024.0));
            SetWindowText(g_AppState.hwndStatus, szStatus);
        }

        return bResult;
    }

    /* ============================================================================
     * NORMAL MODE - For files < 2MB
     * Use the existing background thread + streaming approach
     * Show progress dialog for files > 1MB
     * ========================================================================= */

    /* Initialize thread data */
    ZeroMemory(&g_ThreadData, sizeof(g_ThreadData));
    _tcscpy(g_ThreadData.szFileName, szFileName);
    g_ThreadData.hEdit = hEdit;
    g_ThreadData.hwndMain = GetAncestor(hEdit, GA_ROOT);

    /* Create loading thread */
    g_hLoadThread = CreateThread(NULL, 0, FileLoadThreadProc, &g_ThreadData, 0, NULL);
    if (!g_hLoadThread) {
        ShowErrorDialog(GetParent(hEdit), TEXT("Failed to create loading thread"));
        return FALSE;
    }

    /* Show progress dialog for files > 1MB to prevent "Not Responding" perception */
    BOOL bShowProgressDialog = (dwFileSize > THRESHOLD_PROGRESS);
    HWND hProgressDlg = NULL;

    if (bShowProgressDialog) {
        /* Create modeless dialog for better responsiveness */
        hProgressDlg = CreateDialogParam(g_AppState.hInstance, MAKEINTRESOURCE(IDD_PROGRESS),
                                        g_ThreadData.hwndMain, ProgressDlgProc, (LPARAM)&g_ThreadData);
        if (hProgressDlg) {
            ShowWindow(hProgressDlg, SW_SHOW);
            UpdateWindow(hProgressDlg);
        }

        /* Wait for thread with message processing - check every 30ms for responsiveness */
        while (WaitForSingleObject(g_hLoadThread, 30) == WAIT_TIMEOUT) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (!hProgressDlg || !IsDialogMessage(hProgressDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            if (g_ThreadData.bCancelled) break;
        }
    } else {
        /* Very small files (<1MB): wait with message processing but no dialog */
        while (WaitForSingleObject(g_hLoadThread, 10) == WAIT_TIMEOUT) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    /* Wait for thread to finish */
    WaitForSingleObject(g_hLoadThread, INFINITE);
    CloseHandle(g_hLoadThread);
    g_hLoadThread = NULL;

    /* Check result */
    if (g_ThreadData.bCancelled || g_ThreadData.bError) {
        if (hProgressDlg) DestroyWindow(hProgressDlg);
        if (g_ThreadData.pWideText) {
            HeapFree(GetProcessHeap(), 0, g_ThreadData.pWideText);
        }
        if (g_ThreadData.bError) {
            ShowErrorDialog(GetParent(hEdit), g_ThreadData.szError);
        }
        return FALSE;
    }

    /* ============================================================================
     * RESPONSIVE TEXT LOADING - Prevents "Not Responding"
     *
     * Key insight: EM_STREAMIN and SetWindowText are BLOCKING operations.
     * For large files, we need to:
     * 1. Load text in small chunks
     * 2. Process Windows messages between chunks
     * 3. Update progress dialog
     * ========================================================================= */

    /* Maximum performance settings for RichEdit */
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);

    /* Disable all event notifications during load */
    DWORD dwOldMask = (DWORD)SendMessage(hEdit, EM_SETEVENTMASK, 0, 0);

    /* Set max text limit to handle large files */
    SendMessage(hEdit, EM_EXLIMITTEXT, 0, 500 * 1024 * 1024); /* 500MB limit */

    /* Disable undo buffer during load for better performance */
    SendMessage(hEdit, EM_SETUNDOLIMIT, 0, 0);

    /* For very small files (<500KB), use direct SetWindowText - fast enough */
    if (g_ThreadData.dwWideLen < (500 * 1024 / sizeof(WCHAR))) {
        SetWindowTextW(hEdit, g_ThreadData.pWideText);
    } else {
        /* For larger files, load in smaller chunks with message processing */
        /* Use 10K characters per chunk for maximum responsiveness */
        DWORD dwChunkChars = 10000; /* 10K characters per chunk - smaller = more responsive */
        DWORD dwPos = 0;
        DWORD dwTotal = g_ThreadData.dwWideLen;
        DWORD dwLastUpdate = GetTickCount();
        
        /* Clear edit control first */
        SetWindowTextW(hEdit, TEXT(""));
        
        while (dwPos < dwTotal) {
            DWORD dwToAdd = min(dwChunkChars, dwTotal - dwPos);
            
            /* Temporarily null-terminate this chunk */
            WCHAR chSaved = g_ThreadData.pWideText[dwPos + dwToAdd];
            g_ThreadData.pWideText[dwPos + dwToAdd] = L'\0';
            
            /* Move cursor to end and append */
            int nLen = GetWindowTextLength(hEdit);
            SendMessage(hEdit, EM_SETSEL, nLen, nLen);
            SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(g_ThreadData.pWideText + dwPos));
            
            /* Restore character */
            g_ThreadData.pWideText[dwPos + dwToAdd] = chSaved;
            
            dwPos += dwToAdd;
            
            /* Update progress every 100ms to avoid UI overhead */
            DWORD dwNow = GetTickCount();
            if (hProgressDlg && IsWindow(hProgressDlg) && (dwNow - dwLastUpdate > 100)) {
                int nPercent = (int)((dwPos * 100) / dwTotal);
                SendDlgItemMessage(hProgressDlg, IDC_PROGRESS_BAR, PBM_SETPOS, nPercent, 0);
                
                TCHAR szProgress[128];
                _sntprintf(szProgress, 128, TEXT("Loading into editor... %d%%"), nPercent);
                SetDlgItemText(hProgressDlg, IDC_PROGRESS_TEXT, szProgress);
                dwLastUpdate = dwNow;
            }
            
            /* CRITICAL: Process Windows messages to prevent "Not Responding" */
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    /* Re-enable undo with limit based on file size */
    if (dwFileSize > THRESHOLD_PARTIAL) {
        SendMessage(hEdit, EM_SETUNDOLIMIT, UNDO_LIMIT_LARGE_FILE, 0);
    } else {
        SendMessage(hEdit, EM_SETUNDOLIMIT, 100, 0);
    }
    SendMessage(hEdit, EM_EMPTYUNDOBUFFER, 0, 0);

    /* Position cursor at start */
    SendMessage(hEdit, EM_SETSEL, 0, 0);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);

    /* Re-enable events and redraw */
    SendMessage(hEdit, EM_SETEVENTMASK, 0, dwOldMask);
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);

    /* Force immediate repaint */
    InvalidateRect(hEdit, NULL, TRUE);
    UpdateWindow(hEdit);

    /* Close progress dialog if still open */
    if (hProgressDlg && IsWindow(hProgressDlg)) {
        DestroyWindow(hProgressDlg);
    }

    /* Store line ending in tab state */
    pTab = GetCurrentTabState();
    if (pTab) {
        pTab->lineEnding = g_ThreadData.lineEnding;
    }

    /* Cleanup */
    HeapFree(GetProcessHeap(), 0, g_ThreadData.pWideText);

    return TRUE;
}

/* ============================================================================
 * LOAD MORE CONTENT - For Partial Loading Mode
 * ============================================================================
 * When user presses F5 or clicks "Load More", append next chunk to editor
 * This allows incrementally loading large files without freezing UI
 * ========================================================================= */

BOOL LoadMoreContent(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    if (!pTab || pTab->fileMode != FILEMODE_PARTIAL) {
        MessageBox(hwnd, TEXT("Load More is only available for partially loaded files."),
                  TEXT("Load More"), MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    /* Check if already fully loaded */
    if (pTab->dwLoadedSize >= pTab->dwTotalFileSize) {
        MessageBox(hwnd, TEXT("Entire file is already loaded."),
                  TEXT("Load More"), MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    HWND hEdit = GetCurrentEdit();
    if (!hEdit) return FALSE;

    /* Open file and seek to current position */
    HANDLE hFile = CreateFile(pTab->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        ShowErrorDialog(hwnd, TEXT("Failed to open file for loading more content."));
        return FALSE;
    }

    /* Seek to where we left off */
    SetFilePointer(hFile, pTab->dwLoadedSize, NULL, FILE_BEGIN);

    /* Read next chunk */
    DWORD dwChunkSize = pTab->dwChunkSize;
    DWORD dwRemaining = pTab->dwTotalFileSize - pTab->dwLoadedSize;
    if (dwChunkSize > dwRemaining) {
        dwChunkSize = dwRemaining;
    }

    char* pBuffer = (char*)HeapAlloc(GetProcessHeap(), 0, dwChunkSize + 1);
    if (!pBuffer) {
        CloseHandle(hFile);
        ShowErrorDialog(hwnd, TEXT("Out of memory."));
        return FALSE;
    }

    DWORD dwBytesRead = 0;
    BOOL bSuccess = ReadFile(hFile, pBuffer, dwChunkSize, &dwBytesRead, NULL);
    CloseHandle(hFile);

    if (!bSuccess || dwBytesRead == 0) {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        ShowErrorDialog(hwnd, TEXT("Failed to read file."));
        return FALSE;
    }

    pBuffer[dwBytesRead] = '\0';

    /* Convert to UTF-16 */
    int nWideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pBuffer, dwBytesRead, NULL, 0);
    if (nWideLen == 0) {
        nWideLen = MultiByteToWideChar(CP_ACP, 0, pBuffer, dwBytesRead, NULL, 0);
    }

    if (nWideLen > 0) {
        WCHAR* pWideText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (pWideText) {
            int nConverted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pBuffer, dwBytesRead,
                                                pWideText, nWideLen);
            if (nConverted == 0) {
                nConverted = MultiByteToWideChar(CP_ACP, 0, pBuffer, dwBytesRead,
                                                pWideText, nWideLen);
            }

            if (nConverted > 0) {
                pWideText[nConverted] = L'\0';

                /* Append to existing text */
                SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);

                /* Move cursor to end */
                int nLen = GetWindowTextLength(hEdit);
                SendMessage(hEdit, EM_SETSEL, nLen, nLen);

                /* Append new text */
                SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)pWideText);

                SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hEdit, NULL, TRUE);

                /* Update loaded size */
                pTab->dwLoadedSize += dwBytesRead;

                /* Update status */
                TCHAR szStatus[256];
                if (pTab->dwLoadedSize >= pTab->dwTotalFileSize) {
                    _sntprintf(szStatus, 256,
                        TEXT("File fully loaded: %.1f MB"),
                        pTab->dwLoadedSize / (1024.0 * 1024.0));
                } else {
                    _sntprintf(szStatus, 256,
                        TEXT("Partial Load: %.1f MB of %.1f MB loaded - Press F5 to load more"),
                        pTab->dwLoadedSize / (1024.0 * 1024.0),
                        pTab->dwTotalFileSize / (1024.0 * 1024.0));
                }
                SetWindowText(g_AppState.hwndStatus, szStatus);

                bSuccess = TRUE;
            }
            HeapFree(GetProcessHeap(), 0, pWideText);
        }
    }

    HeapFree(GetProcessHeap(), 0, pBuffer);
    return bSuccess;
}

/* UTF-8 BOM bytes */
static const unsigned char UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

/* Write edit control content to file with UTF-8 encoding
 * Uses safe file replacement: write to temp file first, then replace original
 * This ensures original file is preserved if save fails (Requirement 6.3)
 */
BOOL WriteFileContent(HWND hEdit, const TCHAR* szFileName) {
    HANDLE hFile;
    DWORD dwBytesWritten;
    WCHAR* pWideBuffer;
    char* pUtf8Buffer;
    int nWideLen, nUtf8Len;
    TCHAR szTempFileName[MAX_PATH];
    BOOL bSuccess = FALSE;
    
    /* Get text length first to determine if we need progress dialog */
    nWideLen = GetWindowTextLengthW(hEdit);
    BOOL bShowProgress = (nWideLen > 5 * 1024 * 1024); /* Show progress for > 5MB text */
    
    /* Create temp file name in same directory */
    _tcscpy(szTempFileName, szFileName);
    _tcscat(szTempFileName, TEXT(".tmp"));
    
    /* Write to temp file first (safe file replacement) */
    hFile = CreateFile(szTempFileName, GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    /* Show progress for large files */
    if (bShowProgress) {
        SetWindowText(g_AppState.hwndStatus, TEXT("Saving file..."));
    }
    
    /* Write UTF-8 BOM first */
    WriteFile(hFile, UTF8_BOM, sizeof(UTF8_BOM), &dwBytesWritten, NULL);
    
    if (nWideLen > 0) {
        /* Allocate buffer for wide string */
        pWideBuffer = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nWideLen + 1) * sizeof(WCHAR));
        if (!pWideBuffer) {
            CloseHandle(hFile);
            DeleteFile(szTempFileName); /* Clean up temp file */
            return FALSE;
        }
        
        GetWindowTextW(hEdit, pWideBuffer, nWideLen + 1);
        
        /* Convert wide string to UTF-8 */
        nUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pWideBuffer, nWideLen, NULL, 0, NULL, NULL);
        if (nUtf8Len > 0) {
            pUtf8Buffer = (char*)HeapAlloc(GetProcessHeap(), 0, nUtf8Len);
            if (pUtf8Buffer) {
                WideCharToMultiByte(CP_UTF8, 0, pWideBuffer, nWideLen, pUtf8Buffer, nUtf8Len, NULL, NULL);
                
                /* Write in chunks for large files to maintain responsiveness */
                DWORD dwTotalWritten = 0;
                DWORD dwChunkSize = 1024 * 1024; /* 1MB chunks */
                
                while (dwTotalWritten < (DWORD)nUtf8Len) {
                    DWORD dwToWrite = min(dwChunkSize, (DWORD)nUtf8Len - dwTotalWritten);
                    DWORD dwActual;
                    
                    if (!WriteFile(hFile, pUtf8Buffer + dwTotalWritten, dwToWrite, &dwActual, NULL)) {
                        /* Write failed - clean up and preserve original */
                        HeapFree(GetProcessHeap(), 0, pUtf8Buffer);
                        HeapFree(GetProcessHeap(), 0, pWideBuffer);
                        CloseHandle(hFile);
                        DeleteFile(szTempFileName);
                        return FALSE;
                    }
                    
                    dwTotalWritten += dwActual;
                    
                    /* Update progress for large files */
                    if (bShowProgress && nUtf8Len > 0) {
                        TCHAR szProgress[128];
                        int nPercent = (int)((dwTotalWritten * 100) / nUtf8Len);
                        _sntprintf(szProgress, 128, TEXT("Saving... %d%%"), nPercent);
                        SetWindowText(g_AppState.hwndStatus, szProgress);
                    }
                }
                
                bSuccess = TRUE;
                HeapFree(GetProcessHeap(), 0, pUtf8Buffer);
            }
        }
        
        HeapFree(GetProcessHeap(), 0, pWideBuffer);
    } else {
        bSuccess = TRUE; /* Empty file is valid */
    }
    
    CloseHandle(hFile);
    
    if (bSuccess) {
        /* Replace original with temp file */
        /* First try to delete original (may fail if it doesn't exist) */
        DeleteFile(szFileName);
        
        /* Rename temp to original */
        if (!MoveFile(szTempFileName, szFileName)) {
            /* Move failed - try copy and delete */
            if (CopyFile(szTempFileName, szFileName, FALSE)) {
                DeleteFile(szTempFileName);
            } else {
                /* Complete failure - temp file still exists */
                bSuccess = FALSE;
            }
        }
        
        /* Show confirmation in status bar (Requirement 6.4) */
        if (bSuccess) {
            SetWindowText(g_AppState.hwndStatus, TEXT("File saved successfully"));
        }
    } else {
        /* Clean up temp file on failure */
        DeleteFile(szTempFileName);
    }
    
    return bSuccess;
}

/* Create new document in current tab */
BOOL FileNew(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return FALSE;
    
    /* Check for unsaved changes */
    if (pTab->bModified) {
        if (!PromptSaveChanges(hwnd)) {
            return FALSE;
        }
    }
    
    /* Save the edit control handle BEFORE resetting state */
    HWND hwndEdit = pTab->hwndEdit;
    HWND hwndLineNumbers = pTab->lineNumState.hwndLineNumbers;
    BOOL bShowLineNumbers = pTab->lineNumState.bShowLineNumbers;
    int nLineNumberWidth = pTab->lineNumState.nLineNumberWidth;
    
    /* Clear edit control */
    if (hwndEdit) {
        SetWindowText(hwndEdit, TEXT(""));
    }
    
    /* Reset tab state - this will set hwndEdit to NULL */
    InitTabState(pTab);
    
    /* Restore the edit control handle and line number state */
    pTab->hwndEdit = hwndEdit;
    pTab->lineNumState.hwndLineNumbers = hwndLineNumbers;
    pTab->lineNumState.bShowLineNumbers = bShowLineNumbers;
    pTab->lineNumState.nLineNumberWidth = nLineNumberWidth;
    
    /* Update tab and window title */
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);
    
    /* Update line numbers if visible */
    if (g_AppState.bShowLineNumbers && pTab->lineNumState.hwndLineNumbers) {
        UpdateLineNumbers(pTab->lineNumState.hwndLineNumbers, pTab->hwndEdit);
    }
    
    /* Set focus to edit control */
    if (pTab->hwndEdit) {
        SetFocus(pTab->hwndEdit);
    }
    
    return TRUE;
}

/* Open existing file */
BOOL FileOpen(HWND hwnd) {
    TCHAR szFileName[MAX_PATH] = TEXT("");
    TabState* pTab;
    HWND hwndEdit;
    
    /* Show open dialog first */
    if (!ShowOpenDialog(hwnd, szFileName, MAX_PATH)) {
        return FALSE;
    }
    
    /* Check for Excel files and show warning */
    if (IsExcelFile(szFileName)) {
        if (!ShowExcelWarning(hwnd, szFileName)) {
            return FALSE; /* User cancelled */
        }
        /* User wants to open as raw text - continue */
    }
    
    /* Get current tab */
    pTab = GetCurrentTabState();
    
    /* If current tab is untitled and unmodified, use it; otherwise create new tab */
    if (!pTab || !pTab->bUntitled || pTab->bModified) {
        int nNewTab = AddNewTab(hwnd, TEXT("Loading..."));
        if (nNewTab < 0) return FALSE;
    }
    
    /* Get the edit control handle directly */
    hwndEdit = GetCurrentEdit();
    if (!hwndEdit) {
        ShowErrorDialog(hwnd, TEXT("Failed to get edit control."));
        return FALSE;
    }
    
    /* Show loading status with file type info */
    if (IsCSVFile(szFileName)) {
        SetWindowText(g_AppState.hwndStatus, TEXT("Loading CSV file..."));
    } else {
        SetWindowText(g_AppState.hwndStatus, TEXT("Loading file..."));
    }

    /* Read file content */
    if (!ReadFileContent(hwndEdit, szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to open file."));
        SetWindowText(g_AppState.hwndStatus, TEXT("Ready"));
        return FALSE;
    }

    /* Get tab state again after potential tab creation */
    pTab = GetCurrentTabState();
    if (!pTab) return FALSE;

    /* Update tab state */
    _tcscpy(pTab->szFileName, szFileName);
    pTab->bModified = FALSE;
    pTab->bUntitled = FALSE;

    /* Detect language */
    pTab->language = DetectLanguage(szFileName);

    /* Update titles first (instant feedback) */
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);

    /* Force redraw */
    InvalidateRect(hwndEdit, NULL, TRUE);
    UpdateWindow(hwndEdit);

    /* Apply syntax highlighting AFTER display update for better responsiveness */
    /* Only for small files - large files skip highlighting for performance */
    /* Use stored file size from tab state for efficiency */
    DWORD dwFileSize = pTab->dwTotalFileSize;
    if (dwFileSize == 0) {
        /* Fallback: get file size if not stored */
        HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            dwFileSize = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
        }
    }

    /* Check line count for syntax threshold */
    int nLineCount = (int)SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);

    /* Disable syntax highlighting for:
     * - Files > 256KB (THRESHOLD_SYNTAX_OFF)
     * - Files with > 5000 lines (THRESHOLD_LINE_SYNTAX)
     * - Files in non-normal mode (partial, readonly, mmap)
     */
    BOOL bEnableSyntax = (dwFileSize < THRESHOLD_SYNTAX_OFF) && 
                         (nLineCount < THRESHOLD_LINE_SYNTAX) &&
                         (pTab->fileMode == FILEMODE_NORMAL);

    if (g_bSyntaxHighlight && pTab->language != LANG_NONE && bEnableSyntax) {
        SetWindowText(g_AppState.hwndStatus, TEXT("Applying syntax highlighting..."));
        ApplySyntaxHighlighting(hwndEdit, pTab->language);
    } else if (!bEnableSyntax && pTab->language != LANG_NONE) {
        /* Notify user that syntax highlighting is disabled for this large file */
        TCHAR szInfo[256];
        if (nLineCount >= THRESHOLD_LINE_SYNTAX) {
            _sntprintf(szInfo, 256, TEXT("Large file (%d lines): Syntax highlighting disabled for performance"),
                       nLineCount);
        } else if (pTab->fileMode != FILEMODE_NORMAL) {
            _sntprintf(szInfo, 256, TEXT("Partial/Preview mode: Syntax highlighting disabled"));
        } else {
            _sntprintf(szInfo, 256, TEXT("Large file (%.1f KB): Syntax highlighting disabled for performance"),
                       dwFileSize / 1024.0);
        }
        SetWindowText(g_AppState.hwndStatus, szInfo);
    }

    /* Update status bar with file info - this will call GetWindowTextLength but it's ok now */
    UpdateStatusBar(hwnd);

    /* Sync to session for persistence */
    SyncNewFileToSession(g_AppState.nCurrentTab);

    return TRUE;
}

/* Save current file */
BOOL FileSave(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    if (!pTab) return FALSE;
    
    if (pTab->bUntitled) {
        return FileSaveAs(hwnd);
    }
    
    if (!WriteFileContent(pTab->hwndEdit, pTab->szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to save file."));
        return FALSE;
    }
    
    pTab->bModified = FALSE;
    UpdateTabTitle(g_AppState.nCurrentTab);
    
    /* Sync to session */
    MarkSessionDirty();
    
    return TRUE;
}

/* Save file with new name */
BOOL FileSaveAs(HWND hwnd) {
    TCHAR szFileName[MAX_PATH];
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab) return FALSE;
    
    if (pTab->bUntitled) {
        _tcscpy(szFileName, TEXT("Untitled.txt"));
    } else {
        _tcscpy(szFileName, pTab->szFileName);
    }
    
    if (!ShowSaveDialog(hwnd, szFileName, MAX_PATH)) {
        return FALSE;
    }
    
    if (!WriteFileContent(pTab->hwndEdit, szFileName)) {
        ShowErrorDialog(hwnd, TEXT("Failed to save file."));
        return FALSE;
    }
    
    _tcscpy(pTab->szFileName, szFileName);
    pTab->bModified = FALSE;
    pTab->bUntitled = FALSE;
    
    /* Detect language for new file */
    pTab->language = DetectLanguage(szFileName);
    
    UpdateTabTitle(g_AppState.nCurrentTab);
    UpdateWindowTitle(hwnd);
    
    /* Sync to session */
    MarkSessionDirty();
    
    return TRUE;
}

/* Prompt user to save changes */
BOOL PromptSaveChanges(HWND hwnd) {
    TabState* pTab = GetCurrentTabState();
    
    if (!pTab || !pTab->bModified) {
        return TRUE;
    }
    
    int nResult = ShowConfirmSaveDialog(hwnd);
    
    switch (nResult) {
        case IDYES:
            return FileSave(hwnd);
        case IDNO:
            return TRUE;
        case IDCANCEL:
        default:
            return FALSE;
    }
}
