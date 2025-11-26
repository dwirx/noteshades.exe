/* XNote Performance Benchmark Tool
 * Measures file loading performance at various sizes
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

/* Generate test file with specified size */
BOOL CreateTestFile(const char* szPath, DWORD dwSize) {
    HANDLE hFile = CreateFileA(szPath, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    
    /* Write pattern data - realistic text content */
    const char* pattern = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\r\n"
                         "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n"
                         "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.\r\n";
    DWORD patternLen = (DWORD)strlen(pattern);
    
    DWORD dwWritten = 0;
    while (dwWritten < dwSize) {
        DWORD dwToWrite = (dwSize - dwWritten < patternLen) ? (dwSize - dwWritten) : patternLen;
        DWORD dwActual;
        WriteFile(hFile, pattern, dwToWrite, &dwActual, NULL);
        dwWritten += dwActual;
    }
    
    CloseHandle(hFile);
    return TRUE;
}

/* Measure file read time */
double MeasureReadTime(const char* szPath) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    QueryPerformanceCounter(&start);
    
    HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1.0;
    
    DWORD dwSize = GetFileSize(hFile, NULL);
    char* buffer = (char*)malloc(dwSize + 1);
    if (!buffer) {
        CloseHandle(hFile);
        return -1.0;
    }
    
    DWORD dwRead;
    ReadFile(hFile, buffer, dwSize, &dwRead, NULL);
    buffer[dwRead] = '\0';
    
    CloseHandle(hFile);
    free(buffer);
    
    QueryPerformanceCounter(&end);
    
    return (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0; /* ms */
}

/* Measure chunked read time (simulating XNote behavior) */
double MeasureChunkedReadTime(const char* szPath, DWORD dwChunkSize) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    QueryPerformanceCounter(&start);
    
    HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1.0;
    
    DWORD dwSize = GetFileSize(hFile, NULL);
    char* buffer = (char*)malloc(dwSize + 1);
    if (!buffer) {
        CloseHandle(hFile);
        return -1.0;
    }
    
    DWORD dwTotalRead = 0;
    while (dwTotalRead < dwSize) {
        DWORD dwToRead = (dwSize - dwTotalRead < dwChunkSize) ? (dwSize - dwTotalRead) : dwChunkSize;
        DWORD dwRead;
        ReadFile(hFile, buffer + dwTotalRead, dwToRead, &dwRead, NULL);
        dwTotalRead += dwRead;
        if (dwRead == 0) break;
    }
    buffer[dwTotalRead] = '\0';
    
    CloseHandle(hFile);
    free(buffer);
    
    QueryPerformanceCounter(&end);
    
    return (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0; /* ms */
}

int main() {
    printf("=== XNote Performance Benchmark ===\n\n");
    
    /* Test sizes */
    DWORD sizes[] = {
        100 * 1024,         /* 100 KB */
        256 * 1024,         /* 256 KB */
        512 * 1024,         /* 512 KB */
        1 * 1024 * 1024,    /* 1 MB */
        2 * 1024 * 1024,    /* 2 MB */
        5 * 1024 * 1024,    /* 5 MB */
        10 * 1024 * 1024    /* 10 MB */
    };
    int numSizes = sizeof(sizes) / sizeof(sizes[0]);
    
    const char* testFile = "benchmark_test.txt";
    
    printf("File Size\t\tFull Read\tChunked 32KB\tChunked 64KB\n");
    printf("------------------------------------------------------------------------\n");
    
    for (int i = 0; i < numSizes; i++) {
        DWORD size = sizes[i];
        
        /* Create test file */
        if (!CreateTestFile(testFile, size)) {
            printf("Failed to create test file\n");
            continue;
        }
        
        /* Run benchmarks (average of 3 runs) */
        double fullRead = 0, chunk32 = 0, chunk64 = 0;
        for (int j = 0; j < 3; j++) {
            fullRead += MeasureReadTime(testFile);
            chunk32 += MeasureChunkedReadTime(testFile, 32 * 1024);
            chunk64 += MeasureChunkedReadTime(testFile, 64 * 1024);
        }
        fullRead /= 3;
        chunk32 /= 3;
        chunk64 /= 3;
        
        /* Print results */
        if (size >= 1024 * 1024) {
            printf("%d MB\t\t\t%.2f ms\t\t%.2f ms\t\t%.2f ms\n", 
                   size / (1024 * 1024), fullRead, chunk32, chunk64);
        } else {
            printf("%d KB\t\t\t%.2f ms\t\t%.2f ms\t\t%.2f ms\n", 
                   size / 1024, fullRead, chunk32, chunk64);
        }
        
        /* Cleanup */
        DeleteFileA(testFile);
    }
    
    printf("\n=== Benchmark Complete ===\n");
    printf("\nRecommendations based on results:\n");
    printf("- Files < 512KB: Use full read for best performance\n");
    printf("- Files 512KB-2MB: Use 64KB chunks with progress\n");
    printf("- Files > 2MB: Use 32KB chunks for responsiveness\n");
    
    return 0;
}
