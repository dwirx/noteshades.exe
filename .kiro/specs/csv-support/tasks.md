# Implementation Plan

## CSV Support & Excel Warning

- [x] 1. Implement Excel file handling
  - [x] 1.1 Add IsExcelFile function
    - Detect .xls, .xlsx, .xlsm, .xlsb extensions
    - _Requirements: 2.1_
  - [x] 1.2 Add ShowExcelWarning function
    - Display warning about binary format
    - Suggest Excel/LibreOffice alternatives
    - Allow user to open as raw text
    - _Requirements: 2.1, 2.2, 2.3_
  - [x] 1.3 Integrate Excel check in FileOpen
    - Check before loading file
    - _Requirements: 2.1_

- [x] 2. Implement CSV file detection
  - [x] 2.1 Add IsCSVFile function
    - Detect .csv and .tsv extensions
    - _Requirements: 1.1_
  - [x] 2.2 Update status bar for CSV files
    - Show "Loading CSV file..." message
    - _Requirements: 1.1_
  - [x] 2.3 Add CSV/TSV to Open File dialog filter
    - Added "CSV/TSV Data (*.csv, *.tsv)" filter option
    - Added to "All Supported Files" filter
    - _Requirements: 1.1_

- [x] 3. Performance optimizations (based on benchmark)
  - [x] 3.1 Adaptive chunk size for file reading
    - Small files (<512KB): read all at once
    - Large files: use 64KB chunks (optimal from benchmark)
    - _Requirements: 3.1_
  - [x] 3.2 Optimized Sleep behavior
    - Only yield for large files (>512KB)
    - _Requirements: 3.3_
  - [x] 3.3 Benchmark analysis completed
    - Created benchmark tool
    - Tested various chunk sizes
    - Applied optimal settings (64KB chunks)
    - Results: 2-16x faster file loading

