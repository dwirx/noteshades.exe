# XNote - Lightweight Text Editor for Windows

A fast, lightweight text editor built with Win32 API featuring syntax highlighting, multiple tabs, line numbers, and Vim-style navigation.

## Features

- Multiple tabs support
- Syntax highlighting for 20+ languages
- Line numbers (absolute or relative)
- Vim mode navigation (toggleable)
- Find and Replace
- Word wrap toggle
- Status bar with file info

## Keyboard Shortcuts

### File Operations
| Shortcut | Action |
|----------|--------|
| Ctrl+N | New document |
| Ctrl+T | New tab |
| Ctrl+O | Open file |
| Ctrl+S | Save file |
| Ctrl+Shift+S | Save As |
| Ctrl+W | Close tab |

### Tab Navigation
| Shortcut | Action |
|----------|--------|
| Ctrl+Tab | Next tab |
| Ctrl+Shift+Tab | Previous tab |
| Ctrl+1-9 | Go to tab 1-9 |

### Edit Operations
| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo |
| Ctrl+X | Cut |
| Ctrl+C | Copy |
| Ctrl+V | Paste |
| Ctrl+A | Select all |
| Ctrl+D | Duplicate line |
| Ctrl+Shift+K | Delete line |
| Alt+Up | Move line up |
| Alt+Down | Move line down |
| Ctrl+/ | Toggle comment |
| Tab | Indent |
| Shift+Tab | Unindent |

### Find & Navigation
| Shortcut | Action |
|----------|--------|
| Ctrl+F | Find |
| Ctrl+H | Find and Replace |
| F3 | Find next |
| Ctrl+G | Go to line |

### View Options
| Shortcut | Action |
|----------|--------|
| Ctrl+L | Toggle line numbers |
| Ctrl++ | Zoom in |
| Ctrl+- | Zoom out |
| Ctrl+0 | Reset zoom |
| Alt+Z | Toggle word wrap |
| Ctrl+Shift+V | Toggle Vim mode |
| F1 | Help / Shortcuts |

## Vim Mode

Enable via **View → Vim Mode**. Status bar shows current mode (NORMAL/INSERT/VISUAL/V-LINE).

### Normal Mode - Navigation
| Key | Action |
|-----|--------|
| h / ← | Move left |
| j / ↓ | Move down |
| k / ↑ | Move up |
| l / → | Move right |
| w / W | Next word |
| b / B | Previous word |
| e | End of word |
| 0 | Line start |
| $ | Line end |
| ^ | First non-blank character |
| gg | First line |
| G | Last line |
| {n}G | Go to line n |
| Ctrl+D | Half page down |
| Ctrl+U | Half page up |
| Page Up/Down | Scroll page |

### Normal Mode - Editing
| Key | Action |
|-----|--------|
| x | Delete character |
| X | Delete char before |
| s | Delete char and insert |
| S | Delete line and insert |
| dd | Delete line |
| dw | Delete word |
| d$ | Delete to end of line |
| d0 | Delete to start of line |
| D | Delete to end of line |
| cc | Change line |
| cw | Change word |
| C | Change to end of line |
| yy | Yank (copy) line |
| yw | Yank word |
| Y | Yank line |
| p | Paste after |
| P | Paste before |
| u | Undo |
| Ctrl+R | Redo |
| J | Join lines |
| >> | Indent line |
| << | Unindent line |

### Mode Transitions
| Key | Action |
|-----|--------|
| i | Insert mode (at cursor) |
| a | Insert mode (after cursor) |
| I | Insert at first non-blank |
| A | Insert at line end |
| o | New line below |
| O | New line above |
| v | Visual mode (character) |
| V | Visual Line mode |
| Esc | Return to Normal mode |

### Visual Mode (v)
| Key | Action |
|-----|--------|
| h/j/k/l | Extend selection |
| w/b/e | Extend by word |
| 0/$/^ | Extend to line start/end |
| G | Extend to end of file |
| d/x | Delete selection |
| y | Yank selection |
| c | Change selection |
| > | Indent selection |
| < | Unindent selection |
| V | Switch to Visual Line |
| Esc | Exit visual mode |

### Visual Line Mode (V)
| Key | Action |
|-----|--------|
| j/k | Extend selection by line |
| G | Extend to end of file |
| d/x | Delete lines |
| y | Yank lines |
| c | Change lines |
| > | Indent lines |
| < | Unindent lines |
| v | Switch to Visual (char) |
| Esc | Exit visual mode |

### Repeat Count
Prefix commands with a number to repeat:
- `5j` - Move down 5 lines
- `3dd` - Delete 3 lines
- `10G` - Go to line 10
- `2>>` - Indent 2 times

## Supported Languages

C, C++, Java, JavaScript, TypeScript, Python, Go, Rust, HTML, CSS, JSON, XML, YAML, SQL, PHP, Ruby, Shell, Batch, PowerShell, Markdown

## Building

Requires MinGW-w64 with GCC:

```bash
make        # Build
make clean  # Clean build files
make run    # Build and run
```

## License

MIT License
