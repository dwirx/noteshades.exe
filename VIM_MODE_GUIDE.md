# XNote Vim Mode Guide

XNote includes a comprehensive Vim emulation mode for users who prefer Vim-style editing.

## Enabling Vim Mode

- **Menu**: View → Vim Mode
- **Command**: `:set vim` (when in command mode)

## Modes

| Mode | Description | Enter | Exit |
|------|-------------|-------|------|
| **NORMAL** | Navigation and commands | `Esc` | - |
| **INSERT** | Text input | `i`, `a`, `o`, etc. | `Esc` |
| **VISUAL** | Character selection | `v` | `Esc` |
| **V-LINE** | Line selection | `V` | `Esc` |
| **COMMAND** | Ex commands | `:` | `Enter` or `Esc` |
| **SEARCH** | Search mode | `/` or `?` | `Enter` or `Esc` |

## Navigation Commands

### Basic Movement
| Key | Action |
|-----|--------|
| `h` | Move left |
| `j` | Move down |
| `k` | Move up |
| `l` | Move right |

### Word Movement
| Key | Action |
|-----|--------|
| `w` | Next word start |
| `W` | Next WORD start (whitespace-delimited) |
| `b` | Previous word start |
| `B` | Previous WORD start |
| `e` | End of word |
| `E` | End of WORD |
| `ge` | End of previous word |
| `gE` | End of previous WORD |

### Line Movement
| Key | Action |
|-----|--------|
| `0` | Start of line |
| `^` | First non-blank character |
| `$` | End of line |
| `+` | First non-blank of next line |
| `-` | First non-blank of previous line |

### Screen Movement
| Key | Action |
|-----|--------|
| `H` | Top of screen |
| `M` | Middle of screen |
| `L` | Bottom of screen |
| `gg` | First line |
| `G` | Last line |
| `{count}G` | Go to line {count} |

### Scrolling
| Key | Action |
|-----|--------|
| `Ctrl+D` | Half page down |
| `Ctrl+U` | Half page up |
| `Ctrl+F` | Page down |
| `Ctrl+B` | Page up |
| `zz` | Center current line |
| `zt` | Scroll current line to top |
| `zb` | Scroll current line to bottom |

### Find on Line
| Key | Action |
|-----|--------|
| `f{char}` | Find character forward |
| `F{char}` | Find character backward |
| `t{char}` | Till character forward |
| `T{char}` | Till character backward |

### Other Movement
| Key | Action |
|-----|--------|
| `%` | Matching bracket |
| `{` | Previous paragraph |
| `}` | Next paragraph |

## Editing Commands

### Insert Mode Entry
| Key | Action |
|-----|--------|
| `i` | Insert before cursor |
| `I` | Insert at line start |
| `a` | Append after cursor |
| `A` | Append at line end |
| `o` | Open line below |
| `O` | Open line above |

### Delete
| Key | Action |
|-----|--------|
| `x` | Delete character |
| `X` | Delete character before |
| `dd` | Delete line |
| `D` | Delete to end of line |
| `dw` | Delete word |
| `d$` | Delete to end of line |
| `d0` | Delete to start of line |

### Change
| Key | Action |
|-----|--------|
| `cc` | Change line |
| `C` | Change to end of line |
| `cw` | Change word |
| `s` | Substitute character |
| `S` | Substitute line |

### Yank (Copy) and Paste
| Key | Action |
|-----|--------|
| `yy` | Yank line |
| `Y` | Yank line |
| `yw` | Yank word |
| `p` | Paste after cursor |
| `P` | Paste before cursor |

### Other Edit Commands
| Key | Action |
|-----|--------|
| `r{char}` | Replace character |
| `~` | Toggle case |
| `J` | Join lines |
| `u` | Undo |
| `Ctrl+R` | Redo |
| `>>` | Indent line |
| `<<` | Unindent line |

## Text Objects

Text objects work with operators (`d`, `c`, `y`):

| Text Object | Description |
|-------------|-------------|
| `iw` | Inner word |
| `aw` | A word (including whitespace) |
| `i"` | Inside double quotes |
| `a"` | Around double quotes |
| `i'` | Inside single quotes |
| `a'` | Around single quotes |
| `i(` or `ib` | Inside parentheses |
| `a(` or `ab` | Around parentheses |
| `i{` or `iB` | Inside braces |
| `a{` or `aB` | Around braces |
| `i[` | Inside brackets |
| `a[` | Around brackets |
| `i<` | Inside angle brackets |
| `a<` | Around angle brackets |

### Examples
- `diw` - Delete inner word
- `ci"` - Change inside quotes
- `ya(` - Yank around parentheses

## Visual Mode

| Key | Action |
|-----|--------|
| `v` | Enter visual mode |
| `V` | Enter visual line mode |
| `d` or `x` | Delete selection |
| `y` | Yank selection |
| `c` | Change selection |
| `>` | Indent selection |
| `<` | Unindent selection |

## Search

| Key | Action |
|-----|--------|
| `/pattern` | Search forward |
| `?pattern` | Search backward |
| `n` | Next match |
| `N` | Previous match |
| `*` | Search word under cursor forward |
| `#` | Search word under cursor backward |

## Ex Commands

| Command | Action |
|---------|--------|
| `:w` | Save file |
| `:q` | Close tab |
| `:q!` | Force close without saving |
| `:wq` or `:x` | Save and close |
| `:e` | Open file |
| `:tabnew` | New tab |
| `:tabn` | Next tab |
| `:tabp` | Previous tab |
| `:bn` | Next buffer |
| `:bp` | Previous buffer |
| `:bd` | Close buffer |
| `:{number}` | Go to line |
| `:set nu` | Show line numbers |
| `:set nonu` | Hide line numbers |
| `:set rnu` | Relative line numbers |
| `:set nornu` | Absolute line numbers |
| `:set wrap` | Enable word wrap |
| `:set nowrap` | Disable word wrap |
| `:noh` | Clear search highlight |
| `:help` | Show help |

## Tab Navigation

| Key | Action |
|-----|--------|
| `gt` | Next tab |
| `gT` | Previous tab |
| `{count}gt` | Go to tab {count} |
| `g0` | First tab |
| `g$` | Last tab |

## Tips

1. **Repeat Count**: Most commands accept a count prefix (e.g., `5j` moves down 5 lines)
2. **Dot Command**: `.` repeats the last change (not yet implemented)
3. **Registers**: Deleted/yanked text is stored for paste operations
4. **Line Mode Paste**: When you `dd` or `yy`, paste with `p` creates a new line

## Differences from Vim

- No multiple registers (only default register)
- No macros
- No marks (planned)
- No split windows (use tabs instead)
- Limited ex commands
