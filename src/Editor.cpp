//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>


int getWindowSize(int2d& size) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return -1;
    }
    size.x = ws.ws_col;
    size.y = ws.ws_row;
    return 0;
};


Editor::Editor() {
    terminal_manager::enableRawMode();

    if (terminal_manager::getWindowSize(this->screen_size) == -1) {
        terminal_manager::die("Unable to get window size");
    }

    screen_size.y -= 2; // Reserve 2 lines for status bar and command line
}


void Editor::run() {
    printMessage(this->message.c_str());
    while (true) {
        refreshScreen();
        processKeypress();
    }
}

void Editor::printRows(std::string& s) {}

void Editor::scroll() {}
void Editor::printMessage(std::string_view message, ...) {}


void Editor::refreshScreen() const {
    terminal_manager::clear_screen();

    for (size_t i = 0; i < this->screen_size.y; ++i) {
        const string formatted_line_numbers = std::format("{:>6}  ", i);
        std::cout << terminal_control_sequences::dimmed_color << (line_numbers ? formatted_line_numbers : "") << "~" <<
            terminal_control_sequences::reset_color << "\r\n";
    }

    // Move the cursor back to the top
    std::cout << "\x1b[H";

    //Display the output
    std::cout.flush();
}

void Editor::moveCursor(int key) {
    switch (key) {
        case static_cast<int>(EditorKey::ARROW_UP):
        case 'k':
            // Move UP
            if (cursor_pos.y > 0) {
                cursor_pos.y--;
                cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
            }
            break;
        case static_cast<int>(EditorKey::ARROW_DOWN):
        case 'j':
            // Move DOWN
            if (!rows.empty() && cursor_pos.y < static_cast<int>(rows.size()) - 1) {
                cursor_pos.y++;
                cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
            }
            break;
        case static_cast<int>(EditorKey::ARROW_RIGHT):
        case 'l':
            // Move RIGHT
            if (cursor_pos.x < get_row_length(cursor_pos.y)) {
                cursor_pos.x++;
            } else if (cursor_pos.y < static_cast<int>(rows.size()) - 1) {
                cursor_pos.y++;
                cursor_pos.x = 0; // Move to start of next line
            }
            desired_cursor_pos = cursor_pos.x; // Update desired position for vertical movement
            break;
        case static_cast<int>(EditorKey::ARROW_LEFT):
        case 'h':
            // Move LEFT
            if (cursor_pos.x > 0) {
                cursor_pos.x--;
            } else if (cursor_pos.y > 0) {
                cursor_pos.y--;
                cursor_pos.x = get_row_length(cursor_pos.y); // Move to end of prev line
            }
            desired_cursor_pos = cursor_pos.x; // Update desired position for vertical movement
            break;
    }
    clampCursorPosition();
}

void Editor::movePage(int key) {}

void Editor::processKeypress() {
    char c = terminal_manager::readKey();

    auto next_mode = current_mode->handle_input(*this, c);
    if (next_mode) {
        current_mode = std::move(next_mode);
    }
}
