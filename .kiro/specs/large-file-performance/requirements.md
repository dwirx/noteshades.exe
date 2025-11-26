# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk optimasi performa aplikasi XNote dalam menangani file besar (>500MB). Saat ini aplikasi mengalami lag, delay, dan "not responding" ketika membuka atau mengedit file berukuran besar. Optimasi ini bertujuan untuk memungkinkan pengguna membuka dan bekerja dengan file besar secara lancar tanpa gangguan performa.

## Glossary

- **XNote**: Aplikasi editor teks berbasis Win32 dengan fitur syntax highlighting dan multi-tab
- **Large File**: File teks dengan ukuran lebih dari 50MB
- **Very Large File**: File teks dengan ukuran lebih dari 200MB
- **Huge File**: File teks dengan ukuran lebih dari 500MB
- **Memory-Mapped File**: Teknik untuk mengakses file dengan memetakan langsung ke memori virtual tanpa memuat seluruh file ke RAM
- **Chunked Loading**: Teknik memuat file secara bertahap dalam potongan-potongan kecil
- **Virtual Scrolling**: Teknik menampilkan hanya bagian yang terlihat dari dokumen besar
- **Background Thread**: Thread terpisah yang menjalankan operasi tanpa memblokir UI
- **Progress Dialog**: Dialog yang menampilkan kemajuan operasi yang sedang berjalan
- **RichEdit Control**: Kontrol Windows untuk menampilkan dan mengedit teks dengan formatting

## Requirements

### Requirement 1

**User Story:** Sebagai pengguna, saya ingin membuka file besar (>500MB) tanpa aplikasi menjadi "not responding", sehingga saya dapat bekerja dengan file log atau data besar.

#### Acceptance Criteria

1. WHEN pengguna membuka file berukuran lebih dari 50MB THEN XNote SHALL menampilkan progress dialog dengan persentase kemajuan loading
2. WHEN pengguna membuka file berukuran lebih dari 200MB THEN XNote SHALL memuat file dalam mode read-only preview dengan hanya menampilkan 10MB pertama
3. WHEN pengguna membuka file berukuran lebih dari 500MB THEN XNote SHALL menggunakan memory-mapped file untuk akses tanpa memuat ke RAM
4. WHILE file sedang dimuat THEN XNote SHALL tetap responsif dan memungkinkan pengguna membatalkan operasi
5. IF loading file gagal karena memori tidak cukup THEN XNote SHALL menampilkan pesan error yang informatif dan menyarankan mode alternatif

### Requirement 2

**User Story:** Sebagai pengguna, saya ingin melihat kemajuan loading file besar, sehingga saya tahu berapa lama harus menunggu.

#### Acceptance Criteria

1. WHEN file sedang dimuat THEN XNote SHALL menampilkan progress bar dengan persentase 0-100%
2. WHEN file sedang dimuat THEN XNote SHALL menampilkan informasi ukuran file yang sudah dimuat dan total ukuran
3. WHEN pengguna mengklik tombol Cancel pada progress dialog THEN XNote SHALL membatalkan operasi loading dalam waktu kurang dari 1 detik
4. WHEN loading selesai THEN XNote SHALL menutup progress dialog secara otomatis

### Requirement 3

**User Story:** Sebagai pengguna, saya ingin dapat memuat lebih banyak konten dari file yang dimuat sebagian, sehingga saya dapat melihat bagian lain dari file.

#### Acceptance Criteria

1. WHEN file dimuat dalam mode partial THEN XNote SHALL menampilkan status bar dengan informasi "Loaded X MB of Y MB"
2. WHEN pengguna menekan F5 pada file yang dimuat sebagian THEN XNote SHALL memuat chunk berikutnya (20MB)
3. WHEN seluruh file sudah dimuat THEN XNote SHALL menampilkan pesan "File fully loaded"
4. WHILE chunk tambahan sedang dimuat THEN XNote SHALL tetap responsif

### Requirement 4

**User Story:** Sebagai pengguna, saya ingin scrolling pada file besar tetap lancar, sehingga saya dapat menavigasi dokumen dengan nyaman.

#### Acceptance Criteria

1. WHILE pengguna melakukan scroll pada file besar THEN XNote SHALL mempertahankan frame rate minimal 30 FPS
2. WHEN file dimuat dalam mode memory-mapped THEN XNote SHALL hanya memuat bagian yang terlihat ke memori
3. WHEN pengguna scroll cepat THEN XNote SHALL menggunakan teknik debouncing untuk menghindari rendering berlebihan

### Requirement 5

**User Story:** Sebagai pengguna, saya ingin syntax highlighting dinonaktifkan otomatis untuk file besar, sehingga performa tetap optimal.

#### Acceptance Criteria

1. WHEN file berukuran lebih dari 1MB dimuat THEN XNote SHALL menonaktifkan syntax highlighting secara otomatis
2. WHEN syntax highlighting dinonaktifkan otomatis THEN XNote SHALL menampilkan notifikasi di status bar
3. WHERE pengguna ingin mengaktifkan syntax highlighting pada file besar THEN XNote SHALL menampilkan peringatan tentang dampak performa

### Requirement 6

**User Story:** Sebagai pengguna, saya ingin menyimpan file besar tanpa aplikasi freeze, sehingga saya tidak kehilangan pekerjaan.

#### Acceptance Criteria

1. WHEN pengguna menyimpan file berukuran lebih dari 50MB THEN XNote SHALL menampilkan progress dialog
2. WHILE file sedang disimpan THEN XNote SHALL menulis dalam chunks untuk menjaga responsivitas
3. IF penyimpanan gagal THEN XNote SHALL menampilkan pesan error dan tidak menghapus file asli
4. WHEN penyimpanan selesai THEN XNote SHALL menampilkan konfirmasi di status bar

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

