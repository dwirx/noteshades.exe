# Makefile for Win32 Notepad Application

# Compiler and tools
CC = gcc
RC = windres

# Directories
SRC_DIR = src

# Output
TARGET = notepad.exe

# Compiler flags for Win32
CFLAGS = -Wall -Wextra -O2 -DUNICODE -D_UNICODE
LDFLAGS = -mwindows -lcomctl32 -lcomdlg32

# Resource compiler flags (fix for paths with spaces)
RCFLAGS = "--preprocessor=gcc -E -xc -DRC_INVOKED"

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/file_ops.c \
       $(SRC_DIR)/edit_ops.c \
       $(SRC_DIR)/dialogs.c

# Object files
OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/file_ops.o $(SRC_DIR)/edit_ops.o $(SRC_DIR)/dialogs.o

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
