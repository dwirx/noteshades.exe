# XNote - Lightweight Text Editor for Windows

A fast, lightweight text editor built with Win32 API featuring syntax highlighting, multiple tabs, line numbers, and Vim-style navigation.

## Features

- Multiple tabs support
- Syntax highlighting for 20+ languages
- Line numbers
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
| Ctrl+W | Close tab |

### Edit Operations
| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo |
| Ctrl+X | Cut |
| Ctrl+C | Copy |
| Ctrl+V | Paste |
| Ctrl+A | Select all |

### Find & Replace
| Shortcut | Action |
|----------|--------|
| Ctrl+F | Find |
| Ctrl+H | Find and Replace |
| F3 | Find next |

### View Options
| Menu | Action |
|------|--------|
| View → Line Numbers | Toggle line numbers |
| View → Syntax Highlighting | Toggle syntax colors |
| View → Vim Mode | Toggle Vim navigation |
| Format → Word Wrap | Toggle word wrap |

## Vim Mode

Enable via **View → Vim Mode**. Status bar shows current mode (NORMAL/INSERT/VISUAL).

### Normal Mode - Navigation
| Key | Action |
|-----|--------|
| h / ← | Move left |
| j / ↓ | Move down |
| k / ↑ | Move up |
| l / → | Move right |
| w | Next word |
| b | Previous word |
| 0 | Line start |
| $ | Line end |
| ^ | First non-blank |
| gg | First line |
| G | Last line |
| {n}G | Go to line n |
| Page Up/Down | Scroll page |

### Normal Mode - Editing
| Key | Action |
|-----|--------|
| x | Delete character |
| X | Delete char before |
| dd | Delete line |
| dw | Delete word |
| D | Delete to end of line |
| yy | Yank (copy) line |
| Y | Yank line |
| p | Paste after |
| P | Paste before |
| u | Undo |
| cc | Change line |
| cw | Change word |
| C | Change to end of line |

### Mode Transitions
| Key | Action |
|-----|--------|
| i | Insert mode (at cursor) |
| a | Insert mode (after cursor) |
| I | Insert at line start |
| A | Insert at line end |
| o | New line below |
| O | New line above |
| v | Visual mode |
| Esc | Return to Normal mode |

### Visual Mode
| Key | Action |
|-----|--------|
| h/j/k/l | Extend selection |
| w/b | Extend by word |
| d/x | Delete selection |
| y | Yank selection |
| c | Change selection |
| Esc | Exit visual mode |

### Repeat Count
Prefix commands with a number to repeat:
- `5j` - Move down 5 lines
- `3dd` - Delete 3 lines
- `10G` - Go to line 10

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
