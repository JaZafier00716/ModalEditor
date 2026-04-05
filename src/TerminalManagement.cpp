//
// Created by jan on 4/4/26.
//

#include "../include/TerminalManagement.h"

#include "../include/Editor.h"

namespace terminal_manager {
    termios original_values;

    void clear_screen() {
        std::cout << terminal_control_sequences::clear_screen; // Clears the screen
        std::cout << terminal_control_sequences::cursor_start; // Moves the cursor to the top-left corner
    }

    void die(const std::string_view message) {
        clear_screen();
        perror(message.data());
        std::exit(1);
    }

    void disableRawMode() {
        // Set current terminal attributes
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_values) == -1) {
            die("tcsetattr");
        }
    }

    void enableRawMode() {
        // Source: https://www.man7.org/linux/man-pages/man3/termios.3.html

        // Get current terminal attributes
        if (tcgetattr(STDIN_FILENO, &original_values) == -1) {
            die("tcgetattr");
        }

        // Ensure disableRawMode is called upon exit
        atexit(disableRawMode);

        struct termios raw_values = original_values;

        // Input flags:
        // BRKINT: disables break signal
        // ICRNL: disables CR (carriage) to NL (new line) translation
        // ISTRIP: disables stripping of the 8th bit
        // IXON: disables software flow control (Ctrl-S and Ctrl-Q)
        raw_values.c_iflag &= ~(BRKINT | ICRNL | ISTRIP | IXON);

        // Output flags:
        // OPOST: disables post-processing
        raw_values.c_oflag &= ~(OPOST);

        // Local flags:
        // ECHO: disable echo
        // ICANON: canonical mode
        // IEXTEN: Special keys
        // ISIG: Signals
        raw_values.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

        // Control flags:
        // set 8bit characters
        raw_values.c_cflag |= (CS8);

        // Control characters: set read timeout
        raw_values.c_cc[VMIN] = 0; // Return immediately
        raw_values.c_cc[VTIME] = 1; // 100ms timeout for input

        // Apply modified attributes
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_values) == -1) {
            die("tcsetattr");
        }
    }

    int getWindowSize(int2d& size) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
            return -1;
        }
        size.x = ws.ws_col;
        size.y = ws.ws_row;
        return 0;
    }

    int readKey() {
        int bytes_read;
        char c;

        while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
            if (bytes_read == -1 && errno != EAGAIN) {
                die("read");
            }
        }

        if (c == '\x1b') {
            // Escape sequence - reading special characters
            char seq[3];

            // Read the next two bytes of the escape sequence
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return static_cast<int>(EditorKey::ESCAPE_KEY);
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return static_cast<int>(EditorKey::ESCAPE_KEY);

            // Source:
            // https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    // vt sequences:
                    /*
                        <esc>[1~    - Home        <esc>[16~   -             <esc>[31~   - F17
                        <esc>[2~    - Insert      <esc>[17~   - F6          <esc>[32~   - F18
                        <esc>[3~    - Delete      <esc>[18~   - F7          <esc>[33~   - F19
                        <esc>[4~    - End         <esc>[19~   - F8          <esc>[34~   - F20
                        <esc>[5~    - PgUp        <esc>[20~   - F9          <esc>[35~   -
                        <esc>[6~    - PgDn        <esc>[21~   - F10
                        <esc>[7~    - Home        <esc>[22~   -
                        <esc>[8~    - End         <esc>[23~   - F11
                        <esc>[9~    -             <esc>[24~   - F12
                        <esc>[10~   - F0          <esc>[25~   - F13
                        <esc>[11~   - F1          <esc>[26~   - F14
                        <esc>[12~   - F2          <esc>[27~   -
                        <esc>[13~   - F3          <esc>[28~   - F15
                        <esc>[14~   - F4          <esc>[29~   - F16
                        <esc>[15~   - F5          <esc>[30~   -
                    */

                    // Extended escape sequence => read one more byte
                    if (read(STDIN_FILENO, &seq[2], 1) != 1) return static_cast<int>(EditorKey::ESCAPE_KEY);

                    if (seq[2] == '~') {
                        // Mapping to special keys
                        switch (seq[1]) {
                            case '1': return static_cast<int>(EditorKey::HOME_KEY);     // Home key
                            // case '2': // insert key - not implemented
                            case '3': return static_cast<int>(EditorKey::DEL_KEY);      // Delete key
                            case '4': return static_cast<int>(EditorKey::END_KEY);      // End key
                            case '5': return static_cast<int>(EditorKey::PAGE_UP);      // Page Up key
                            case '6': return static_cast<int>(EditorKey::PAGE_DOWN);    // Page Down key
                            case '7': return static_cast<int>(EditorKey::HOME_KEY);     // Home key (alternative)
                            case '8': return static_cast<int>(EditorKey::END_KEY);      // End key (alternative)
                        }
                    }
                }
                else {
                    // xterm sequences:
                    /*
                        <esc>[A     - Up          <esc>[K     -             <esc>[U     -
                        <esc>[B     - Down        <esc>[L     -             <esc>[V     -
                        <esc>[C     - Right       <esc>[M     -             <esc>[W     -
                        <esc>[D     - Left        <esc>[N     -             <esc>[X     -
                        <esc>[E     -             <esc>[O     -             <esc>[Y     -
                        <esc>[F     - End         <esc>OP     - F1          <esc>[Z     -
                        <esc>[G     - Keypad 5    <esc>OQ     - F2
                        <esc>[H     - Home        <esc>OR     - F3
                        <esc>[I     -             <esc>OS     - F4
                        <esc>[J     -             <esc>[T     -
                    */
                    switch (seq[1]) {
                        case 'A': return static_cast<int>(EditorKey::ARROW_UP);     // Up arrow
                        case 'B': return static_cast<int>(EditorKey::ARROW_DOWN);   // Down arrow
                        case 'C': return static_cast<int>(EditorKey::ARROW_RIGHT);  // Right arrow
                        case 'D': return static_cast<int>(EditorKey::ARROW_LEFT);   // Left arrow
                        case 'F': return static_cast<int>(EditorKey::END_KEY);      // End key (alternative)
                        case 'H': return static_cast<int>(EditorKey::HOME_KEY);
                    }
                }
            }
            else if (seq[0] == 'O') {
                // Home and End key alternatives
                switch (seq[1]) {
                    case 'F': return static_cast<int>(EditorKey::END_KEY);      // End key (alternative
                    case 'H': return static_cast<int>(EditorKey::HOME_KEY);     // Home key (alternative)
                }
            }

            // Unrecognized escape sequence
            return '\x1b';
        }
            
        // Regular character
        return c;
    }
}
