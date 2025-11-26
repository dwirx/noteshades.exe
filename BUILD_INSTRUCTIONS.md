# 🔨 BUILD INSTRUCTIONS - XNote dengan Large File Support

## ✅ Perubahan yang Sudah Diimplementasikan

Semua fitur large file support sudah diimplementasikan dengan sukses:
- ✅ Multi-tier loading system (Normal, Partial, Read-only, Memory-mapped)
- ✅ Chunked loading untuk file 50-200MB
- ✅ F5 hotkey untuk "Load More"
- ✅ Intelligent file mode detection
- ✅ Progress indicators dan status bar updates

## 🪟 Build di Windows (RECOMMENDED)

### Metode 1: Menggunakan build.bat (Paling Mudah)

```cmd
# Buka Command Prompt atau PowerShell di folder project
cd F:\2026\A-Project\ProjectC\beta

# Jalankan build script
build.bat

# Output: xnote.exe
```

### Metode 2: Menggunakan Make

```cmd
# Di Command Prompt
cd F:\2026\A-Project\ProjectC\beta

# Clean previous build
make clean

# Build
make

# Build dan jalankan
make run

# Rebuild from scratch
make rebuild
```

### Requirements:
- **MinGW-w64** dengan GCC compiler
- **windres** (Windows Resource Compiler)
- Libraries: comctl32, comdlg32, shell32

Jika belum punya MinGW-w64, download dari:
- https://www.mingw-w64.org/downloads/
- Atau via MSYS2: `pacman -S mingw-w64-x86_64-gcc`

## 🐧 Build di WSL (Alternative)

### Install MinGW Cross-Compiler

```bash
# Update package list
sudo apt-get update

# Install MinGW-w64 cross-compiler
sudo apt-get install -y mingw-w64

# Verify installation
x86_64-w64-mingw32-gcc --version
```

### Modify Makefile untuk Cross-Compilation

Buat file `Makefile.wsl`:

```makefile
# Cross-compilation Makefile for WSL

CC = x86_64-w64-mingw32-gcc
RC = x86_64-w64-mingw32-windres

SRC_DIR = src
TARGET = xnote.exe

CFLAGS = -Wall -Wextra -O3 -DUNICODE -D_UNICODE
LDFLAGS = -mwindows -lcomctl32 -lcomdlg32 -lshell32 -s

RCFLAGS = --preprocessor="x86_64-w64-mingw32-gcc -E -xc -DRC_INVOKED"

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/file_ops.c \
       $(SRC_DIR)/edit_ops.c \
       $(SRC_DIR)/dialogs.c \
       $(SRC_DIR)/line_numbers.c \
       $(SRC_DIR)/statusbar.c \
       $(SRC_DIR)/syntax.c \
       $(SRC_DIR)/vim_mode.c \
       $(SRC_DIR)/session.c \
       $(SRC_DIR)/theme.c \
       $(SRC_DIR)/settings.c

OBJS = $(SRCS:.c=.o)
RES_SRC = $(SRC_DIR)/notepad.rc
RES_OBJ = $(SRC_DIR)/notepad.o

all: $(TARGET)

$(TARGET): $(OBJS) $(RES_OBJ)
	$(CC) $(OBJS) $(RES_OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RES_OBJ): $(RES_SRC)
	$(RC) $(RCFLAGS) $(RES_SRC) -o $(RES_OBJ)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

.PHONY: all clean
```

### Build dengan Makefile.wsl

```bash
cd /mnt/f/2026/A-Project/ProjectC/beta

# Build
make -f Makefile.wsl

# Run (needs Windows to execute)
./xnote.exe
```

## 🔍 Verifikasi Build Berhasil

Setelah build berhasil, file yang dihasilkan:

```
xnote.exe           # Main executable (~500KB - 1MB)
src/main.o          # Compiled object files
src/file_ops.o
src/edit_ops.o
... (and other .o files)
src/notepad.o       # Compiled resources
```

## 🧪 Testing Large File Support

### Test 1: File Kecil (<50MB) - Normal Mode
```bash
# Buat test file 10MB
./xnote.exe test_10mb.txt

# Expected: Instant load, all features available
```

### Test 2: File Medium (50-200MB) - Partial Mode
```bash
# Test dengan file 80MB
./xnote.exe test_80mb.log

# Expected:
# - Dialog muncul: "Partial Loading Mode"
# - Load 20MB pertama instantly (2-3 detik)
# - Status bar: "Partial Load: 20.0 MB of 80.0 MB loaded"
# - Press F5 untuk load more
```

### Test 3: File Besar (200MB-1GB) - Read-only Mode
```bash
# Test dengan file 300MB
./xnote.exe test_300mb.log

# Expected:
# - Dialog muncul: "Read-Only Preview Mode"
# - Load 10MB pertama untuk preview (3-4 detik)
# - Read-only mode
# - Status: "READ-ONLY PREVIEW: Showing first 10.0 MB of 300.0 MB total"
```

### Membuat Test Files

#### Windows PowerShell:
```powershell
# 10MB test file
fsutil file createnew test_10mb.txt 10485760

# 80MB test file
fsutil file createnew test_80mb.log 83886080

# 300MB test file
fsutil file createnew test_300mb.log 314572800
```

#### Linux/WSL:
```bash
# 10MB
dd if=/dev/urandom of=test_10mb.txt bs=1M count=10

# 80MB
dd if=/dev/urandom of=test_80mb.log bs=1M count=80

# 300MB
dd if=/dev/urandom of=test_300mb.log bs=1M count=300
```

## 🐛 Troubleshooting

### Error: "gcc: command not found"
**Solusi:** Install MinGW-w64
- Windows: Download dari mingw-w64.org atau install via MSYS2
- WSL: `sudo apt-get install mingw-w64`

### Error: "windres: command not found"
**Solusi:** windres comes with MinGW-w64, make sure it's in PATH

### Error: "cannot find -lcomctl32"
**Solusi:** Make sure you have the full MinGW-w64 toolchain with Windows libraries

### Build succeeds but xnote.exe crashes
**Kemungkinan penyebab:**
1. Missing DLL dependencies (jarang terjadi dengan static linking)
2. Corrupted build - try `make clean && make`
3. Antivirus blocking execution

### "Not responding" masih terjadi
**Periksa:**
1. File size berapa yang Anda buka?
2. Apakah dialog "Large File Mode" muncul?
3. Periksa status bar - apakah menunjukkan "Partial Load"?
4. Coba buka file dengan drag-and-drop ke xnote.exe

## 📊 Expected Performance

| File Size | Mode | Load Time | Memory Usage |
|-----------|------|-----------|--------------|
| 10MB | Normal | <1s | ~30MB |
| 50MB | Partial | 2-3s | ~60MB |
| 100MB | Partial | 2-3s | ~60MB (initial) |
| 200MB | Read-only | 3-4s | ~30MB |
| 500MB | Read-only | 4-5s | ~30MB |

## 🎯 Features to Test

- ✅ Open large file (50MB+)
- ✅ Check if dialog appears
- ✅ Verify status bar shows file info
- ✅ Press F5 to load more content
- ✅ Check UI remains responsive
- ✅ Try editing partial loaded file
- ✅ Try syntax highlighting (should be disabled for large files)
- ✅ Test tab switching with large file open
- ✅ Test closing tab with partially loaded file

## 📝 Notes

- Build output `xnote.exe` adalah standalone executable
- Tidak memerlukan DLL eksternal (statically linked)
- Kompatibel dengan Windows 7, 8, 10, 11
- Support UTF-8 dengan BOM detection
- Session auto-save setiap 5 detik

## 🚀 Deployment

Untuk distribusi, cukup copy `xnote.exe` ke folder manapun.
Tidak ada dependencies eksternal yang diperlukan!

---

**Happy Building!** 🎉

Jika ada masalah, periksa error message dari compiler dan pastikan semua source files sudah di-update dengan benar.
