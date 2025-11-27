# Makefile for XNote Application

# Compiler and tools
CC = gcc
RC = windres

# Directories
SRC_DIR = src

# Output
TARGET = xnote.exe

# Compiler flags for Win32 (optimized)
CFLAGS = -Wall -Wextra -O3 -DUNICODE -D_UNICODE
LDFLAGS = -mwindows -lcomctl32 -lcomdlg32 -lshell32 -lshlwapi -s

# Resource compiler flags (fix for paths with spaces)
RCFLAGS = "--preprocessor=gcc -E -xc -DRC_INVOKED"

# Source files
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
       $(SRC_DIR)/json_format.c \
       $(SRC_DIR)/settings.c \
       $(SRC_DIR)/multi_cursor.c \
       $(SRC_DIR)/dragdrop.c

# Object files
OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/file_ops.o $(SRC_DIR)/edit_ops.o $(SRC_DIR)/dialogs.o $(SRC_DIR)/line_numbers.o $(SRC_DIR)/statusbar.o $(SRC_DIR)/syntax.o $(SRC_DIR)/vim_mode.o $(SRC_DIR)/session.o $(SRC_DIR)/theme.o $(SRC_DIR)/json_format.o $(SRC_DIR)/settings.o $(SRC_DIR)/multi_cursor.o $(SRC_DIR)/dragdrop.o

# Resource files
RES_SRC = $(SRC_DIR)/notepad.rc
RES_OBJ = $(SRC_DIR)/notepad.o

# Default target
all: $(TARGET)
	@echo Build complete: $(TARGET)

# Link executable
$(TARGET): $(OBJS) $(RES_OBJ)
	$(CC) $(OBJS) $(RES_OBJ) -o $(TARGET) $(LDFLAGS)

# Compile C source files
$(SRC_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(SRC_DIR)/main.o

$(SRC_DIR)/file_ops.o: $(SRC_DIR)/file_ops.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/file_ops.c -o $(SRC_DIR)/file_ops.o

$(SRC_DIR)/edit_ops.o: $(SRC_DIR)/edit_ops.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/edit_ops.c -o $(SRC_DIR)/edit_ops.o

$(SRC_DIR)/dialogs.o: $(SRC_DIR)/dialogs.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dialogs.c -o $(SRC_DIR)/dialogs.o

$(SRC_DIR)/line_numbers.o: $(SRC_DIR)/line_numbers.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/line_numbers.c -o $(SRC_DIR)/line_numbers.o

$(SRC_DIR)/statusbar.o: $(SRC_DIR)/statusbar.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/statusbar.c -o $(SRC_DIR)/statusbar.o

$(SRC_DIR)/syntax.o: $(SRC_DIR)/syntax.c $(SRC_DIR)/notepad.h $(SRC_DIR)/syntax.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/syntax.c -o $(SRC_DIR)/syntax.o

$(SRC_DIR)/vim_mode.o: $(SRC_DIR)/vim_mode.c $(SRC_DIR)/vim_mode.h $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/vim_mode.c -o $(SRC_DIR)/vim_mode.o

$(SRC_DIR)/session.o: $(SRC_DIR)/session.c $(SRC_DIR)/session.h $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/session.c -o $(SRC_DIR)/session.o

$(SRC_DIR)/theme.o: $(SRC_DIR)/theme.c $(SRC_DIR)/theme.h $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/theme.c -o $(SRC_DIR)/theme.o

$(SRC_DIR)/json_format.o: $(SRC_DIR)/json_format.c $(SRC_DIR)/json_format.h $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/json_format.c -o $(SRC_DIR)/json_format.o

$(SRC_DIR)/settings.o: $(SRC_DIR)/settings.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/settings.c -o $(SRC_DIR)/settings.o

$(SRC_DIR)/multi_cursor.o: $(SRC_DIR)/multi_cursor.c $(SRC_DIR)/multi_cursor.h $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/multi_cursor.c -o $(SRC_DIR)/multi_cursor.o

$(SRC_DIR)/dragdrop.o: $(SRC_DIR)/dragdrop.c $(SRC_DIR)/notepad.h $(SRC_DIR)/resource.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dragdrop.c -o $(SRC_DIR)/dragdrop.o

# Compile resource file
$(RES_OBJ): $(RES_SRC) $(SRC_DIR)/resource.h
	$(RC) $(RCFLAGS) $(RES_SRC) -o $(RES_OBJ)

# Clean build artifacts
clean:
	@if exist $(SRC_DIR)\*.o del /Q $(SRC_DIR)\*.o
	@if exist $(TARGET) del /Q $(TARGET)
	@echo Clean complete

# Rebuild
rebuild: clean all

# Run the application
run: $(TARGET)
	$(TARGET)

.PHONY: all clean rebuild run
