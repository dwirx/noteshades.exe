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

- [ ] 3. Performance optimizations (completed in large-file-performance spec)
  - [x] 3.1 Adaptive chunk size for file reading
    - Small files: read all at once
    - Large files: use small chunks
    - _Requirements: 3.1_
  - [x] 3.2 Optimized Sleep behavior
    - Only yield for large files
    - _Requirements: 3.3_

