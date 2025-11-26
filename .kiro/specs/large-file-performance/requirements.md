# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk optimasi performa aplikasi XNote dalam menangani file besar. Saat ini aplikasi mengalami lag, delay, dan "not responding" ketika membuka atau mengedit file berukuran besar (bahkan file 2-10MB sudah menyebabkan masalah). Optimasi ini bertujuan untuk memungkinkan pengguna membuka dan bekerja dengan file besar secara lancar tanpa gangguan performa.

**Masalah Utama yang Harus Diselesaikan:**
1. Aplikasi freeze/not responding saat loading file >2MB
2. Scrolling lag pada file dengan banyak baris (>30,000 lines)
3. Syntax highlighting memperlambat rendering
4. Memory usage tinggi untuk file besar
5. UI tidak responsif selama operasi file

## Glossary

- **XNote**: Aplikasi editor teks berbasis Win32 dengan fitur syntax highlighting dan multi-tab
- **Small File**: File teks dengan ukuran kurang dari 1MB (loading normal tanpa progress)
- **Medium File**: File teks dengan ukuran 1-2MB (loading normal dengan progress dialog)
- **Large File**: File teks dengan ukuran 2-10MB (partial loading - 512KB pertama)
- **Very Large File**: File teks dengan ukuran 10-50MB (read-only preview - 512KB)
- **Huge File**: File teks dengan ukuran lebih dari 50MB (memory-mapped)
- **Memory-Mapped File**: Teknik untuk mengakses file dengan memetakan langsung ke memori virtual tanpa memuat seluruh file ke RAM
- **Chunked Loading**: Teknik memuat file secara bertahap dalam potongan-potongan kecil
- **Virtual Scrolling**: Teknik menampilkan hanya bagian yang terlihat dari dokumen besar
- **Background Thread**: Thread terpisah yang menjalankan operasi tanpa memblokir UI
- **Progress Dialog**: Dialog yang menampilkan kemajuan operasi yang sedang berjalan
- **RichEdit Control**: Kontrol Windows untuk menampilkan dan mengedit teks dengan formatting
- **Debouncing**: Teknik untuk membatasi frekuensi eksekusi fungsi untuk mencegah lag

## Requirements

### Requirement 1

**User Story:** Sebagai pengguna, saya ingin membuka file besar tanpa aplikasi menjadi "not responding", sehingga saya dapat bekerja dengan file log atau data besar.

#### Acceptance Criteria

1. WHEN pengguna membuka file berukuran lebih dari 2MB THEN XNote SHALL menampilkan progress dialog dengan persentase kemajuan loading
2. WHEN pengguna membuka file berukuran lebih dari 2MB THEN XNote SHALL memuat file dalam mode partial loading dengan hanya menampilkan 512KB pertama
3. WHEN pengguna membuka file berukuran lebih dari 10MB THEN XNote SHALL memuat file dalam mode read-only preview dengan 512KB pertama
4. WHEN pengguna membuka file berukuran lebih dari 50MB THEN XNote SHALL menggunakan memory-mapped file untuk akses tanpa memuat ke RAM
5. WHILE file sedang dimuat THEN XNote SHALL tetap responsif dan memungkinkan pengguna membatalkan operasi dengan tombol Cancel
6. IF loading file gagal karena memori tidak cukup THEN XNote SHALL menampilkan pesan error yang informatif dan menyarankan mode alternatif
7. WHEN file sedang dimuat THEN XNote SHALL memproses Windows messages setiap 64KB untuk mencegah "Not Responding"

### Requirement 2

**User Story:** Sebagai pengguna, saya ingin melihat kemajuan loading file besar, sehingga saya tahu berapa lama harus menunggu.

#### Acceptance Criteria

1. WHEN file berukuran lebih dari 2MB sedang dimuat THEN XNote SHALL menampilkan progress bar dengan persentase 0-100%
2. WHEN file sedang dimuat THEN XNote SHALL menampilkan informasi ukuran file yang sudah dimuat dan total ukuran dalam format "X.X MB / Y.Y MB"
3. WHEN pengguna mengklik tombol Cancel pada progress dialog THEN XNote SHALL membatalkan operasi loading dalam waktu kurang dari 500ms
4. WHEN loading selesai THEN XNote SHALL menutup progress dialog secara otomatis
5. WHILE progress dialog ditampilkan THEN XNote SHALL memperbarui progress setiap 100ms untuk feedback visual yang smooth

### Requirement 3

**User Story:** Sebagai pengguna, saya ingin dapat memuat lebih banyak konten dari file yang dimuat sebagian, sehingga saya dapat melihat bagian lain dari file.

#### Acceptance Criteria

1. WHEN file dimuat dalam mode partial THEN XNote SHALL menampilkan status bar dengan informasi "Loaded X MB of Y MB"
2. WHEN pengguna menekan F5 pada file yang dimuat sebagian THEN XNote SHALL memuat chunk berikutnya (1MB)
3. WHEN seluruh file sudah dimuat THEN XNote SHALL menampilkan pesan "File fully loaded"
4. WHILE chunk tambahan sedang dimuat THEN XNote SHALL tetap responsif

### Requirement 4

**User Story:** Sebagai pengguna, saya ingin scrolling pada file besar tetap lancar, sehingga saya dapat menavigasi dokumen dengan nyaman.

#### Acceptance Criteria

1. WHILE pengguna melakukan scroll pada file besar THEN XNote SHALL mempertahankan frame rate minimal 30 FPS
2. WHEN file dimuat dalam mode memory-mapped THEN XNote SHALL hanya memuat bagian yang terlihat ke memori
3. WHEN pengguna scroll cepat THEN XNote SHALL menggunakan teknik debouncing dengan interval 16ms untuk menghindari rendering berlebihan
4. WHEN file memiliki lebih dari 10000 baris THEN XNote SHALL menonaktifkan line number sync saat scrolling cepat
5. WHILE scrolling THEN XNote SHALL menonaktifkan WM_SETREDRAW untuk mencegah flicker

### Requirement 5

**User Story:** Sebagai pengguna, saya ingin syntax highlighting dinonaktifkan otomatis untuk file besar, sehingga performa tetap optimal.

#### Acceptance Criteria

1. WHEN file berukuran lebih dari 256KB dimuat THEN XNote SHALL menonaktifkan syntax highlighting secara otomatis
2. WHEN syntax highlighting dinonaktifkan otomatis THEN XNote SHALL menampilkan notifikasi di status bar dengan pesan "Syntax highlighting disabled for large file"
3. WHERE pengguna ingin mengaktifkan syntax highlighting pada file besar THEN XNote SHALL menampilkan peringatan tentang dampak performa
4. WHEN file memiliki lebih dari 5000 baris THEN XNote SHALL menonaktifkan syntax highlighting terlepas dari ukuran file

### Requirement 6

**User Story:** Sebagai pengguna, saya ingin menyimpan file besar tanpa aplikasi freeze, sehingga saya tidak kehilangan pekerjaan.

#### Acceptance Criteria

1. WHEN pengguna menyimpan file berukuran lebih dari 5MB THEN XNote SHALL menampilkan progress dialog
2. WHILE file sedang disimpan THEN XNote SHALL menulis dalam chunks 1MB untuk menjaga responsivitas
3. IF penyimpanan gagal THEN XNote SHALL menampilkan pesan error dan tidak menghapus file asli (safe file replacement)
4. WHEN penyimpanan selesai THEN XNote SHALL menampilkan konfirmasi "File saved successfully" di status bar
5. WHILE file sedang disimpan THEN XNote SHALL memproses Windows messages untuk mencegah "Not Responding"

### Requirement 7

**User Story:** Sebagai pengguna, saya ingin undo/redo tetap berfungsi dengan baik pada file besar, sehingga saya dapat membatalkan perubahan.

#### Acceptance Criteria

1. WHEN file besar dimuat THEN XNote SHALL membatasi undo buffer ke 100 operasi untuk menghemat memori
2. WHEN file dimuat dalam mode read-only THEN XNote SHALL menonaktifkan undo buffer
3. WHEN pengguna melakukan undo pada file besar THEN XNote SHALL menyelesaikan operasi dalam waktu kurang dari 500ms

### Requirement 8

**User Story:** Sebagai pengguna, saya ingin mengetahui mode loading yang digunakan untuk file, sehingga saya memahami batasan yang berlaku.

#### Acceptance Criteria

1. WHEN file besar dibuka THEN XNote SHALL menampilkan dialog informasi tentang mode loading yang dipilih
2. WHEN file dalam mode read-only THEN XNote SHALL menampilkan indikator "READ-ONLY" di title bar
3. WHEN file dalam mode partial loading THEN XNote SHALL menampilkan tombol "Load More" atau instruksi F5 di status bar

### Requirement 9

**User Story:** Sebagai pengguna, saya ingin aplikasi tetap responsif saat melakukan operasi apapun, sehingga saya tidak perlu menunggu atau melihat "Not Responding".

#### Acceptance Criteria

1. WHILE operasi file berlangsung THEN XNote SHALL memproses Windows messages minimal setiap 50ms
2. WHEN operasi memakan waktu lebih dari 500ms THEN XNote SHALL menampilkan indikator loading di status bar
3. WHEN pengguna melakukan typing pada file besar THEN XNote SHALL membatasi syntax re-highlighting ke visible area saja
4. WHILE RichEdit control sedang diupdate THEN XNote SHALL menggunakan WM_SETREDRAW FALSE untuk mencegah flicker
5. WHEN file loading selesai THEN XNote SHALL menampilkan konten dalam waktu kurang dari 100ms setelah data tersedia

### Requirement 10

**User Story:** Sebagai pengguna, saya ingin memory usage tetap terkontrol saat membuka file besar, sehingga sistem tidak menjadi lambat.

#### Acceptance Criteria

1. WHEN file dalam mode partial loading THEN XNote SHALL membatasi memory usage ke ukuran chunk yang dimuat saja
2. WHEN tab ditutup THEN XNote SHALL membebaskan semua memory yang dialokasikan termasuk memory-mapped handles
3. WHEN file dalam mode read-only preview THEN XNote SHALL hanya menyimpan preview size di memory
4. WHILE multiple tabs terbuka THEN XNote SHALL membatasi total memory usage untuk konten file

