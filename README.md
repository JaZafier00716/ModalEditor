# ModalEditor

A small Vim-inspired modal text editor for the terminal, written in C++23. It talks to the terminal directly through ANSI escape sequences and POSIX `termios` raw mode, with no curses or other UI library. Normal-mode motions are configurable through a plain-text file.

This is a personal learning project, not a Vim replacement: it covers a focused subset of modal editing.

## Features

- **Three modes:** normal, insert and command, shown by the editor
- **Normal-mode motions** with numeric counts: character, word (`w`/`W`, `b`/`B`, `e`/`E`), line start and end, file start and end, paragraph, screen top/middle/bottom, matching bracket/brace, page up/down
- **Operators:** `d` (delete) and `y` (yank) combined with a motion, plus `dd` / `yy` for whole lines
- **Insert commands:** `i`, `a`, `I`, `A`
- **Command mode** (`:`): `w` saves, `q` quits. When the buffer has no name, you are asked for a file name on save
- **Configurable bindings:** motions are bound to keys, including multi-key sequences such as `gg`, in `motions.conf`
- Arrow keys, Home/End and PageUp/PageDown as motions

## Tech stack

C++23 (`std::format`, `std::print`, `std::filesystem`), CMake, POSIX terminal APIs. The code is split into small classes: `Editor` (main loop and wiring), `EditorMode` (normal/insert/command key handling), `CursorController` (motions), `TextDocument` (buffer, file I/O), `EditorView` (rendering) and `TerminalManagement` (raw mode and key parsing).

## Getting started

### Prerequisites

- Linux or another POSIX system with an ANSI-capable terminal
- A C++23 compiler with `<print>` support (GCC 14+ or a recent Clang with a matching standard library)
- CMake 4.1 or newer (`CMakeLists.txt` requires it)

### Build

```bash
cmake -S . -B build
cmake --build build
```

If your default compiler is older, point CMake at a newer one, for example `-DCMAKE_CXX_COMPILER=g++-14`.

### Run

```bash
./build/ModalEditor [file] [--default]
```

- `file` is opened if given; otherwise you start with an empty buffer called `new file`
- `--default` ignores `motions.conf` and loads `motions.default.conf` instead

`dummy_text.txt` is a sample file for trying out the motions, for example `./build/ModalEditor dummy_text.txt --default`.

### Memory checks

```bash
cmake -S . -B build-asan -DENABLE_SANITIZER=ON     # AddressSanitizer + LeakSanitizer
cmake --build build-asan
cmake --build build --target memcheck              # Valgrind (target exists only if valgrind is installed)
```

`memcheck` starts the editor under Valgrind, so it is meant for an interactive terminal.

## Usage

| Mode | Enter with | Leave with | What it does |
|---|---|---|---|
| Normal | start-up, `Esc` | | Motions, counts, `d` / `y` operators |
| Insert | `i`, `a`, `I`, `A` | `Esc` | Type text; Enter, Tab and Backspace are supported |
| Command | `:` | `Esc` | `w` save, `q` quit |

## Configuration

Normal-mode bindings are read from `motions.conf` (or `motions.default.conf` with `--default`) in the source directory; the path is fixed at build time. Each line is `<trigger> = <motion>`; Lines starting with `#` are comments. If the file is missing or invalid, the editor prints a warning and falls back to built-in defaults.

```
h = left
gg = file_start
w = word_forward
ArrowUp = up
```

Available motions: `left`, `right`, `up`, `down`, `page_up`, `page_down`, `line_start`, `line_end`, `file_start`, `file_end`, `word_forward`, `word_forward_big`, `word_backward`, `word_backward_big`, `word_end`, `word_end_big`, `screen_top`, `screen_middle`, `screen_bottom`, `paragraph_forward`, `paragraph_backward`, `matching_bracket`, `matching_brace`. Special keys (`ArrowUp`, `PageDown`, `Home`, ...) can be bound too. Keys such as `i`, `a`, `I`, `A`, `:`, `d` and `y` stay command keys.

## Project structure

```
main.cpp                 entry point, argument and config handling
include/  src/           Editor, EditorMode, CursorController, TextDocument,
                         EditorView, TerminalManagement
motions.conf             your bindings
motions.default.conf     reference bindings
dummy_text.txt           sample text for trying motions
CMakeLists.txt           build, sanitizer option, memcheck target
```

## Limitations

There is no paste, undo or search yet. The yank buffer is filled by `y` and `d` but nothing reads it back. There are no automated tests and no license file in the repository.
