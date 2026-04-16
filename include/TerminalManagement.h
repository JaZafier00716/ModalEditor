//
// Created by jan on 4/4/26.
//

#ifndef MODALEDITOR_TERMINALMANAGEMENT_H
#define MODALEDITOR_TERMINALMANAGEMENT_H
#include <string>
#include <termios.h>
#include <sys/ioctl.h>

struct int2d {
    int x{0};
    int y{0};
};


enum class EditorKey
{
    BACKSPACE_ALT = 8,
    ESCAPE_KEY = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 300, // Some number higher than max ascii
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
};

namespace terminal_control_sequences {
    // colors
    constexpr std::string_view reset_color = "\033[0m";
    constexpr std::string_view dimmed_color = "\033[1;35m";
    constexpr std::string_view invert_colors = "\x1b[7m";
    constexpr std::string_view reset_invert_colors = "\x1b[m";

    //control sequences
    constexpr std::string_view clear_screen = "\x1B[2J";
    constexpr std::string_view cursor_start = "\x1B[H";
    constexpr std::string_view hide_cursor = "\x1B[?25l";
    constexpr std::string_view show_cursor = "\x1B[?25h";
    constexpr std::string_view clear_to_eol = "\x1B[2K";
    constexpr std::string_view new_line = "\r\n";
}

namespace terminal_manager {
    void die(std::string_view message);
    void disableRawMode();
    void enableRawMode();
    int getWindowSize(int2d& size);
    int readKey();

    void clear_screen();
};


#endif //MODALEDITOR_TERMINALMANAGEMENT_H