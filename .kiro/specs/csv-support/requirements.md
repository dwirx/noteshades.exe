# Requirements Document

## Introduction

Dokumen ini mendefinisikan requirements untuk menambahkan dukungan file CSV dan perbaikan performa tambahan pada XNote. CSV (Comma-Separated Values) adalah format teks yang umum digunakan untuk data tabular. Fitur ini akan memungkinkan pengguna membuka dan melihat file CSV dengan format yang lebih mudah dibaca.

**Catatan Penting:**
- File XLS dan XLSX adalah format binary Microsoft Excel yang memerlukan library khusus
- XNote sebagai text editor akan fokus pada CSV yang merupakan format teks
- Untuk XLS/XLSX, pengguna disarankan menggunakan Excel atau LibreOffice

## Glossary

- **CSV**: Comma-Separated Values - format file teks untuk data tabular
- **Delimiter**: Karakter pemisah antar kolom (comma, semicolon, tab)
- **Column Alignment**: Penyesuaian lebar kolom agar data terlihat rapi
- **Preview Mode**: Mode tampilan khusus untuk file CSV

## Requirements

### Requirement 1

**User Story:** Sebagai pengguna, saya ingin membuka file CSV dan melihat datanya dengan format yang rapi, sehingga saya dapat membaca data tabular dengan mudah.

#### Acceptance Criteria

1. WHEN pengguna membuka file dengan ekstensi .csv THEN XNote SHALL mendeteksi delimiter secara otomatis (comma, semicolon, atau tab)
2. WHEN file CSV dibuka THEN XNote SHALL menampilkan data dengan kolom yang ter-align
3. WHEN file CSV memiliki header row THEN XNote SHALL menampilkan header dengan highlight berbeda
4. WHEN pengguna mengedit file CSV THEN XNote SHALL mempertahankan format delimiter asli saat menyimpan

### Requirement 2

**User Story:** Sebagai pengguna, saya ingin XNote memberikan peringatan saat membuka file Excel binary, sehingga saya tahu bahwa format tersebut tidak didukung.

#### Acceptance Criteria

1. WHEN pengguna mencoba membuka file .xls atau .xlsx THEN XNote SHALL menampilkan dialog peringatan
2. WHEN dialog peringatan ditampilkan THEN XNote SHALL menyarankan menggunakan Excel atau LibreOffice
3. WHERE pengguna tetap ingin membuka file Excel THEN XNote SHALL membuka sebagai binary/raw text

### Requirement 3

**User Story:** Sebagai pengguna, saya ingin performa loading file lebih cepat lagi, sehingga saya tidak perlu menunggu lama.

#### Acceptance Criteria

1. WHEN file dimuat THEN XNote SHALL menggunakan buffered I/O untuk kecepatan maksimal
2. WHEN file besar dimuat THEN XNote SHALL menampilkan estimasi waktu loading
3. WHEN loading berlangsung THEN XNote SHALL memproses messages setiap 16KB untuk responsivitas
4. WHEN file selesai dimuat THEN XNote SHALL langsung menampilkan konten tanpa delay

