# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk aplikasi Notepad sederhana yang dibangun menggunakan bahasa C dengan Win32 API untuk antarmuka grafis (GUI). Aplikasi ini akan menyediakan fungsionalitas dasar pengeditan teks seperti membuat, membuka, menyimpan file teks, serta fitur edit standar.

## Glossary

- **Notepad**: Aplikasi editor teks sederhana untuk membuat dan mengedit file teks
- **Win32 API**: Application Programming Interface untuk pengembangan aplikasi Windows
- **Text Editor**: Komponen antarmuka untuk input dan manipulasi teks
- **Menu Bar**: Baris menu horizontal di bagian atas jendela aplikasi
- **File Dialog**: Dialog sistem untuk memilih file untuk dibuka atau disimpan
- **Clipboard**: Area penyimpanan sementara sistem untuk operasi cut, copy, paste

## Requirements

### Requirement 1

**User Story:** Sebagai pengguna, saya ingin membuat dokumen teks baru, sehingga saya dapat mulai menulis konten dari awal.

#### Acceptance Criteria

1. WHEN pengguna memilih menu File > New THEN Notepad SHALL membersihkan area teks dan mengatur judul jendela menjadi "Untitled - Notepad"
2. WHEN pengguna memilih File > New dan terdapat perubahan yang belum disimpan THEN Notepad SHALL menampilkan dialog konfirmasi untuk menyimpan perubahan
3. WHEN pengguna mengkonfirmasi penyimpanan pada dialog THEN Notepad SHALL menyimpan file sebelum membuat dokumen baru

### Requirement 2

**User Story:** Sebagai pengguna, saya ingin membuka file teks yang sudah ada, sehingga saya dapat melihat dan mengedit kontennya.

#### Acceptance Criteria

1. WHEN pengguna memilih menu File > Open THEN Notepad SHALL menampilkan dialog Open File dengan filter untuk file teks (.txt)
2. WHEN pengguna memilih file dan mengklik Open THEN Notepad SHALL memuat konten file ke area teks
3. WHEN file berhasil dibuka THEN Notepad SHALL menampilkan nama file pada judul jendela
4. IF file tidak dapat dibaca THEN Notepad SHALL menampilkan pesan error yang menjelaskan masalah

### Requirement 3

**User Story:** Sebagai pengguna, saya ingin menyimpan dokumen teks, sehingga saya dapat menyimpan pekerjaan saya untuk digunakan nanti.

#### Acceptance Criteria

1. WHEN pengguna memilih menu File > Save pada dokumen baru THEN Notepad SHALL menampilkan dialog Save As
2. WHEN pengguna memilih menu File > Save pada dokumen yang sudah ada THEN Notepad SHALL menyimpan konten ke file yang sama
3. WHEN pengguna memilih menu File > Save As THEN Notepad SHALL menampilkan dialog Save As untuk memilih lokasi dan nama file
4. WHEN file berhasil disimpan THEN Notepad SHALL memperbarui judul jendela dengan nama file baru
5. IF file tidak dapat disimpan THEN Notepad SHALL menampilkan pesan error yang menjelaskan masalah

### Requirement 4

**User Story:** Sebagai pengguna, saya ingin mengedit teks dengan operasi standar, sehingga saya dapat memanipulasi konten dengan mudah.

#### Acceptance Criteria

1. WHEN pengguna memilih menu Edit > Cut THEN Notepad SHALL memindahkan teks yang dipilih ke clipboard
2. WHEN pengguna memilih menu Edit > Copy THEN Notepad SHALL menyalin teks yang dipilih ke clipboard
3. WHEN pengguna memilih menu Edit > Paste THEN Notepad SHALL menyisipkan konten clipboard pada posisi kursor
4. WHEN pengguna memilih menu Edit > Select All THEN Notepad SHALL memilih seluruh teks dalam area editor
5. WHEN pengguna memilih menu Edit > Undo THEN Notepad SHALL membatalkan operasi edit terakhir

### Requirement 5

**User Story:** Sebagai pengguna, saya ingin keluar dari aplikasi dengan aman, sehingga saya tidak kehilangan pekerjaan yang belum disimpan.

#### Acceptance Criteria

1. WHEN pengguna memilih menu File > Exit THEN Notepad SHALL menutup aplikasi
2. WHEN pengguna menutup aplikasi dan terdapat perubahan yang belum disimpan THEN Notepad SHALL menampilkan dialog konfirmasi
3. WHEN pengguna memilih "Yes" pada dialog konfirmasi THEN Notepad SHALL menyimpan file sebelum menutup
4. WHEN pengguna memilih "No" pada dialog konfirmasi THEN Notepad SHALL menutup tanpa menyimpan
5. WHEN pengguna memilih "Cancel" pada dialog konfirmasi THEN Notepad SHALL membatalkan operasi keluar

### Requirement 6

**User Story:** Sebagai pengguna, saya ingin antarmuka yang familiar dan mudah digunakan, sehingga saya dapat bekerja dengan efisien.

#### Acceptance Criteria

1. WHEN aplikasi dimulai THEN Notepad SHALL menampilkan jendela dengan menu bar, area teks, dan status bar
2. WHEN pengguna mengetik di area teks THEN Notepad SHALL menampilkan karakter yang diketik secara real-time
3. WHEN pengguna mengubah ukuran jendela THEN Notepad SHALL menyesuaikan ukuran area teks secara proporsional
4. WHEN pengguna menggunakan keyboard shortcut (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Z, Ctrl+X, Ctrl+C, Ctrl+V, Ctrl+A) THEN Notepad SHALL menjalankan fungsi yang sesuai

### Requirement 7

**User Story:** Sebagai pengguna, saya ingin melihat informasi tentang aplikasi, sehingga saya dapat mengetahui versi dan pembuat aplikasi.

#### Acceptance Criteria

1. WHEN pengguna memilih menu Help > About THEN Notepad SHALL menampilkan dialog About dengan informasi aplikasi
2. WHEN dialog About ditampilkan THEN Notepad SHALL menunjukkan nama aplikasi dan versi
