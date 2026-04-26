//
// Created by jan on 4/16/26.
//
#include "../include/EditorView.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>
#include "../include/TerminalManagement.h"

/**
 * @brief Renders the full frame and positions the terminal cursor.
 * @param current_mode Active editor mode used for status display.
 */
void EditorView::refreshScreen(const std::variant<NormalMode, InsertMode, CommandMode>& current_mode) {
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
    std::format_to(std::back_inserter(output_buffer), "{:>8} Mode | {:<10} | Line {}", mode_name, file_name,
                   row_pos + 1);
    output_buffer += terminal_control_sequences::reset_invert_colors;
    output_buffer += terminal_control_sequences::new_line;
    std::format_to(std::back_inserter(output_buffer), "{}", debug_info_message);

    const auto render_col_pos = document.renderColumnFromRaw(row_pos, col_pos);
    const int2d visual_cursor = {
            .x = render_col_pos - offset.x + 1 + line_num_offset,
            .y = row_pos - offset.y + 1
        };
    // Position cursor
    std::format_to(std::back_inserter(output_buffer), "\x1b[{};{}H", visual_cursor.y, visual_cursor.x);
    output_buffer += terminal_control_sequences::show_cursor;

    std::cout << output_buffer << std::flush;
    debug_info_message.clear();
}

/**
 * @brief Sets the status-bar file name.
 * @param text File name to display.
 */
void EditorView::setFileName(const std::string_view text) {
    file_name = text;
}

/**
 * @brief Appends a debug message to the status area buffer.
 * @param message Message to append.
 */
void EditorView::appendDebugMessage(const std::string_view message) {
    debug_info_message += message;
    debug_info_message += " | ";
}

/**
 * @brief Enables or disables rendering of line numbers.
 * @param enabled True to render line numbers.
 */
void EditorView::setLineNumbersEnabled(const bool enabled) {
    line_numbers = enabled;
}

/**
 * @brief Renders all visible rows into the provided string buffer.
 * @param s Output buffer to append rendered rows to.
 */
void EditorView::printRows(string& s) {
    const auto total_rows = document.lineCount();

    line_num_offset = line_numbers ? static_cast<int>(std::to_string(total_rows).length()) + 2 : 0;

    const auto available_width = std::max(0, screen_size.x - line_num_offset);

    for (int i = 0; i < screen_size.y; ++i) {
        const auto file_y = i + offset.y;

        if (file_y < static_cast<int>(total_rows)) {
            // Draw file content
            if (line_numbers) {
                s += terminal_control_sequences::dimmed_color;
                s += getRelativeLineNumber(file_y);
                s += terminal_control_sequences::reset_color;
            }

            const auto row = document.lineRenderedTextView(file_y);

            if (available_width > 0 && offset.x < static_cast<int>(row.size())) {
                std::string_view row_view = row;
                // Cut string before cursor
                row_view.remove_prefix(offset.x);
                // Cut string after visible area
                if (row_view.size() > static_cast<size_t>(available_width)) {
                    row_view.remove_suffix(row_view.size() - available_width);
                }
                s += row_view;
            }
        }
        else {
            // Draw empty line -- Tilde (~)
            if (line_numbers) {
                // Draw spaces instead of line numbers
                s += std::string(line_num_offset, ' ');
            }
            s += terminal_control_sequences::dimmed_color;
            s += "~";
            s += terminal_control_sequences::reset_color;
        }

        s += terminal_control_sequences::clear_to_eol;
        s += terminal_control_sequences::new_line;
    }
}

/** @brief Adjusts viewport offsets to keep the cursor in view. */
void EditorView::scroll() {
    auto [col_pos, row_pos] = cursor_controller.getCursorPosition();
    render_pos = document.renderColumnFromRaw(row_pos, col_pos);
    const auto total_rows = document.lineCount();
    line_num_offset = line_numbers ? static_cast<int>(std::to_string(total_rows).length()) + 2 : 0;
    const auto available_width = std::max(0, screen_size.x - line_num_offset);

    // Horizontal scrolling
    if (render_pos < offset.x) {
        // Scrolling left
        // Updating horizontal offset to current cursor position
        offset.x = render_pos;
    }
    if (available_width > 0 && render_pos >= offset.x + available_width) {
        // Cursor OOB -> Scrolling right
        offset.x = render_pos - available_width + 5; // +5 for some padding
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

/**
 * @brief Formats either absolute (current line) or relative line number text.
 * @param y Document row index.
 * @return Formatted line number field, or empty string when disabled.
 */
string EditorView::getRelativeLineNumber(const int y) const {
    if (!line_numbers) {
        return {};
    }

    const auto cursor_pos_y = cursor_controller.getCursorPosition().y;

    int line_number = (y == cursor_pos_y) ? (y + 1) : std::abs(y - cursor_pos_y);

    return std::format("{:>{}}  ", line_number, line_num_offset - 2); // -2 beacause of the padding added
}
