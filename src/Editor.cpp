//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <ranges>

Editor::Editor() : Editor("new file") {}

Editor::Editor(const string& filename) : cursor_controller(rows, screen_size) {
    this->file_name = filename.empty() ? "new file" : filename;

    terminal_manager::enableRawMode();

    if (terminal_manager::getWindowSize(this->screen_size) == -1) {
        terminal_manager::die("Unable to get window size");
    }

    screen_size.y -= 1; // Reserve 2 lines for status bar and command line
}


void Editor::run() {
    printMessage(this->file_name);
    terminal_manager::clear_screen();
    while (!should_exit) {
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
    auto [col_pos, row_pos] = cursor_controller.getCursorPosition();
    render_pos = col_pos;

    // Horizontal scrolling
    if (col_pos < offset.x) {
        // Scrolling left
        // Updating horizontal offset to current cursor position
        offset.x = col_pos;
    }
    if (col_pos >= offset.x + screen_size.x) {
        // Cursor OOB -> Scrolling right
        offset.x = col_pos - screen_size.x + 5; // +5 for some padding
    }

    // Vertical scrolling
    if (row_pos < offset.y) {
        // Scrolling up
        // Updating vertical offset to current cursor position
        offset.y = row_pos;
    }
    if (row_pos >= offset.y + screen_size.y) {
        // Scrolling down
        offset.y = row_pos - screen_size.y + 5;
    }
}


void Editor::printMessage(std::string_view message, ...) {

}

int2d Editor::getCursorPosition() const {
    return cursor_controller.getCursorPosition();
}

std::string Editor::getRelativeLineNumber(const int y) const {
    int line_number;
    const auto cursor_pos_y = getCursorPosition().y;

    if (y == cursor_pos_y) {
        line_number = cursor_pos_y + 1; // numbering starts at 1
    } else {
        line_number = abs(y - cursor_pos_y);
    }
    return std::format("{:>4}  ", line_number);
}


void Editor::refreshScreen() {
    terminal_manager::clear_screen();
    scroll();
    output_buffer.clear();

    output_buffer += terminal_control_sequences::hide_cursor;
    output_buffer += terminal_control_sequences::cursor_start;

    printRows(output_buffer);

    
    // Bottom status bar
    auto [col_pos, row_pos] = cursor_controller.getCursorPosition();
    const std::string_view mode_name = std::visit([](const auto& mode) {
        return mode.getName();
    }, current_mode);

    output_buffer += terminal_control_sequences::invert_colors;
    std::format_to(std::back_inserter(output_buffer), "{:>8} Mode | {:<10} | Line {}", mode_name, file_name, row_pos + 1);
    output_buffer += terminal_control_sequences::reset_invert_colors;
    output_buffer += terminal_control_sequences::new_line;
    std::format_to(std::back_inserter(output_buffer), "{}", debug_info_message);


    const int2d visual_cursor = {
        .x = col_pos - offset.x + 1 + line_num_offset,
        .y = row_pos - offset.y + 1
    };
    // Position cursor
    std::format_to(std::back_inserter(output_buffer), "\x1b[{};{}H", visual_cursor.y, visual_cursor.x);
    output_buffer += terminal_control_sequences::show_cursor;

    std::cout << output_buffer << std::flush;
    debug_info_message.clear();
}

void Editor::appendDebugMessage(const std::string_view message) {
    debug_info_message += message;
    debug_info_message += " | ";
}
void Editor::requestQuit() {
    should_exit = true;
}

void Editor::processKeypress() {
    const auto c = terminal_manager::readKey();
    appendDebugMessage(std::format("Key pressed: {}", c));
    const auto next_mode = std::visit([&](auto& mode) -> std::optional<ModeType> {
        return mode.handle_input(mode_context, c);
    }, current_mode);

    if (!next_mode.has_value()) {
        return;
    }

    switch (next_mode.value()) {
        case ModeType::NORMAL:
            current_mode = NormalMode{};
            break;
        case ModeType::INSERT:
            current_mode = InsertMode{};
            break;
        case ModeType::COMMAND:
            current_mode = CommandMode{};
            break;
    }
}


void Editor::ModeContextGate::appendDebugMessage(const std::string_view message) {
    editor.appendDebugMessage(message);
}

void Editor::ModeContextGate::moveCursor(const int key) {
    editor.cursor_controller.moveCursor(key);
}

void Editor::ModeContextGate::movePage(const int key) {
    editor.cursor_controller.movePage(key);
}

void Editor::ModeContextGate::moveCursorLineStart() {
    editor.cursor_controller.moveCursorLineStart();
}

void Editor::ModeContextGate::moveCursorLineEnd() {
    editor.cursor_controller.moveCursorLineEnd();
}

void Editor::ModeContextGate::moveCursorRightOne() {
    editor.cursor_controller.moveCursorRightOne();
}

void Editor::ModeContextGate::requestQuit() {
    editor.requestQuit();
}

