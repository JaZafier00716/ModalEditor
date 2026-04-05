//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <ranges>

Editor::Editor() : Editor("new file") {}

Editor::Editor(const string& filename){
    this->message = filename.empty() ? "new file" : filename;

    terminal_manager::enableRawMode();

    if (terminal_manager::getWindowSize(this->screen_size) == -1) {
        terminal_manager::die("Unable to get window size");
    }

    screen_size.y -= 1; // Reserve 2 lines for status bar and command line
}


void Editor::run() {
    printMessage(this->message);
    terminal_manager::clear_screen();
    while (true) {
        refreshScreen();
        processKeypress();
    }
}

void Editor::printRows(std::string& s) {
    size_t i = 0;

    const auto visible_rows = rows | std::views::drop(offset.y) | std::views::take(screen_size.y);

    for (auto& row : visible_rows) {
        if (line_numbers) {

            const auto line_num = getRelativeLineNumber(i+offset.y);
            line_num_offset = line_num.length();
            s += terminal_control_sequences::dimmed_color;
            s += line_num;
            s += terminal_control_sequences::reset_color;
        }

        const auto available_width = screen_size.x - line_num_offset;


        if (offset.x < row.size()) {
            std::string_view row_view = row;
            // Cut string before cursor
            row_view.remove_prefix(offset.x);
            // Cut string after visible area
            if (row_view.size() > static_cast<size_t>(available_width)) {
                row_view.remove_suffix(row_view.size() - available_width);
            }

            s += row_view;
        }

        s += terminal_control_sequences::clear_to_eol;
        s += terminal_control_sequences::new_line;
        i++;
    }
    for (; i < screen_size.y; ++i) {
        if (line_numbers) {
            const auto line_num = getRelativeLineNumber(i+offset.y);
            s += terminal_control_sequences::dimmed_color;
            s += line_num;
            s += terminal_control_sequences::reset_color;
        }
        s += terminal_control_sequences::dimmed_color;
        s += "~\r\n";
        s += terminal_control_sequences::reset_color;
    }
}

void Editor::scroll() {
    render_pos = cursor_pos.x;

    // Horizontal scrolling
    if (cursor_pos.x < offset.x) {
        // Scrolling left
        // Updating horizontal offset to current cursor position
        offset.x = cursor_pos.x;
    }
    if (cursor_pos.x >= offset.x + screen_size.x) {
        // Cursor OOB -> Scrolling right
        offset.x = cursor_pos.x - screen_size.x + 5; // +5 for some padding
    }

    // Vertical scrolling
    if (cursor_pos.y < offset.y) {
        // Scrolling up
        // Updating vertical offset to current cursor position
        offset.y = cursor_pos.y;
    }
    if (cursor_pos.y >= offset.y + screen_size.y) {
        // Scrolling down
        offset.y = cursor_pos.y - screen_size.y + 5;
    }
}
void Editor::printMessage(std::string_view message, ...) {

}


void Editor::refreshScreen() {
    terminal_manager::clear_screen();
    scroll();
    output_buffer.clear();

    output_buffer += terminal_control_sequences::hide_cursor;
    output_buffer += terminal_control_sequences::cursor_start;

    printRows(output_buffer);

    // Bottom status bar
    output_buffer += terminal_control_sequences::invert_colors;
    std::format_to(std::back_inserter(output_buffer), "{:>8} Mode | {:<10} | Line {}", current_mode->getName(), message, cursor_pos.y + 1);
    output_buffer += terminal_control_sequences::reset_invert_colors;
    output_buffer += terminal_control_sequences::new_line;
    std::format_to(std::back_inserter(output_buffer), "{}", debug_info_message);


    const int2d visual_cursor = {
        .x = cursor_pos.x - offset.x + 1 + line_num_offset,
        .y = cursor_pos.y - offset.y + 1
    };
    // Position cursor
    std::format_to(std::back_inserter(output_buffer), "\x1b[{};{}H", visual_cursor.y, visual_cursor.x);
    output_buffer += terminal_control_sequences::show_cursor;

    std::cout << output_buffer << std::flush;
    debug_info_message.clear();
}

void Editor::moveCursor(const int key) {
    switch (key) {
        case static_cast<int>(EditorKey::ARROW_UP):
        case 'k':
            // Move UP
            appendDebugMessage("Moving up");
            if (cursor_pos.y > 0) {
                cursor_pos.y--;
                cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
            }
            break;
        case static_cast<int>(EditorKey::ARROW_DOWN):
        case 'j':
            // Move DOWN
            appendDebugMessage("Moving down");
            if (!rows.empty() && cursor_pos.y < static_cast<int>(rows.size()) - 1) {
                cursor_pos.y++;
                cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
            }
            break;
        case static_cast<int>(EditorKey::ARROW_RIGHT):
        case 'l':
            // Move RIGHT
            appendDebugMessage("Moving right");
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
            appendDebugMessage("Moving left");
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

void Editor::movePage(const int key) {
    if (key == static_cast<int>(EditorKey::PAGE_UP)) {
        cursor_pos.y = std::max(cursor_pos.y - screen_size.y, 0);
    } else if (key == static_cast<int>(EditorKey::PAGE_DOWN)) {
        cursor_pos.y = std::min(cursor_pos.y + screen_size.y, static_cast<int>(rows.size()) - 1);
    }
    clampCursorPosition();
}

void Editor::processKeypress() {
    const auto c = terminal_manager::readKey();
    appendDebugMessage(std::format("Key pressed: {}", c));
    auto next_mode = current_mode->handle_input(*this, c);
    if (next_mode) {
        current_mode = std::move(next_mode);
    }
}
