//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <algorithm>
#include <iostream>
#include <ranges>

namespace {
    int comparePositions(const int2d& left, const int2d& right) {
        if (left.y != right.y) {
            return left.y < right.y ? -1 : 1;
        }
        if (left.x != right.x) {
            return left.x < right.x ? -1 : 1;
        }
        return 0;
    }

    int2d makeExclusiveEnd(const TextDocument& document, int2d position) {
        if (document.empty()) {
            return position;
        }

        const auto line_length = static_cast<int>(document.lineRawLength(position.y));
        if (position.x < line_length) {
            ++position.x;
            return position;
        }

        if (position.y + 1 < static_cast<int>(document.lineCount())) {
            ++position.y;
            position.x = 0;
        }

        return position;
    }
}

/** @brief Constructs an editor with no initial file path. */
Editor::Editor() : Editor("") {}

/**
 * @brief Constructs an editor and optionally loads a file.
 * @param filename Path to file to open, or empty for a new buffer.
 */
Editor::Editor(const string& filename)
: cursor_controller{document, screen_size},
view{document, screen_size, cursor_controller}
{
    const auto file_name = filename.empty() ? "new file" : filename;
    view.setFileName(file_name);
    view.setLineNumbersEnabled(true);

    if (!filename.empty()) {
        std::string load_error;
        if (!document.loadFromFile(filename, load_error)) {
            view.appendDebugMessage(std::format("Open failed ({}): {}", filename, load_error));
        }
    }

    terminal_manager::enableRawMode();

    if (terminal_manager::getWindowSize(this->screen_size) == -1) {
        terminal_manager::die("Unable to get window size");
    }

    screen_size.y -= 3; // Reserve 1 status line and 2 debug lines
}


/** @brief Runs the main render/input loop until exit is requested. */
void Editor::run() {
    // terminal_manager::clear_screen();
    while (!should_exit) {
        view.refreshScreen(current_mode);
        processKeypress();
    }
    terminal_manager::clear_screen();
}



/** @brief Requests termination of the main editor loop. */
void Editor::requestQuit() {
    should_exit = true;
}

/** @brief Reads one key event and dispatches it to the active mode handler. */
void Editor::processKeypress() {
    const auto key = terminal_manager::readKey();
    view.appendDebugMessage(std::format("Key pressed: {}", terminal_manager::keyToDebugString(key)));
    const auto next_mode = std::visit([&](auto& mode) -> std::optional<ModeType> {
        return mode.handle_input(mode_context, key);
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

/**
 * @brief Forwards mode debug/status messages to the view.
 * @param message Message to append.
 */
void Editor::ModeContextGate::appendDebugMessage(const std::string_view message) {
    editor.view.appendDebugMessage(message);
}

/**
 * @brief Inserts a character at the current cursor position.
 * @param c Character to insert.
 */
void Editor::ModeContextGate::insertCharacter(const char c) {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    if (editor.document.insertCharAt(line, col, c)) {
        editor.cursor_controller.setCursorPositionX(col + 1);
    }
}

/** @brief Inserts a newline and moves the cursor to the next line start. */
void Editor::ModeContextGate::insertNewLine() {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    if (editor.document.insertNewlineAt(line, col)) {
        editor.cursor_controller.applyMotion(editor_motion::Down{});
        editor.cursor_controller.setCursorPositionX(0);
    }
}

/** @brief Performs a backward delete at the current cursor position. */
void Editor::ModeContextGate::backspace() {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    const auto previous_line_length = line > 0 ? static_cast<int>(editor.document.lineRawLength(line - 1)) : 0;
    if (editor.document.eraseCharBefore(line, col)) {
        if (col > 0) {
            editor.cursor_controller.setCursorPositionX(col - 1);
        }
        else if (line > 0) {
            editor.cursor_controller.setCursorPosition({previous_line_length, line - 1});
        }
    }
}

/** @brief Moves the cursor one position to the right. */
void Editor::ModeContextGate::moveCursorRightOne() {
    editor.cursor_controller.moveCursorRightOne();
}

void Editor::ModeContextGate::applyOperator(const NormalModeOperator op, const editor_motion::Motion& motion,
                                            const int count, const bool linewise, const bool inclusive) {
    editor.applyNormalOperator(op, motion, count, linewise, inclusive);
}

/** @brief Requests editor shutdown through the owning Editor instance. */
void Editor::ModeContextGate::requestQuit() {
    editor.requestQuit();
}

void Editor::ModeContextGate::saveToFile() {
    if (editor.view.getFileName() == "new file") {
        terminal_manager::disableRawMode();
        std::cout << "\nSave as: " << std::flush;
        string filename;
        std::getline(std::cin, filename);
        terminal_manager::enableRawMode();

        if (filename.empty()) {
            editor.view.appendDebugMessage("File save cancelled: missing file name");
            return;
        }

        editor.view.setFileName(filename);
    }

    string debug_message;
    editor.document.saveToFile(editor.view.getFileName(), debug_message);
    if (debug_message.empty()) {
        debug_message = "File saved successfully";
    } else {
        debug_message = std::format("File save failed: {}", debug_message);
    }
    editor.view.appendDebugMessage(debug_message);
}

/**
 * @brief Applies a motion command through the cursor controller.
 * @param motion Motion to apply.
 */
void Editor::ModeContextGate::applyMotion(const editor_motion::Motion& motion) {
    editor.cursor_controller.applyMotion(motion);
}

void Editor::applyNormalOperator(const NormalModeOperator op, const editor_motion::Motion& motion, const int count,
                                 const bool linewise, const bool inclusive) {
    if (document.empty()) {
        yank_buffer.clear();
        return;
    }

    const auto start = cursor_controller.getCursorPosition();
    for (int i = 0; i < count; ++i) {
        cursor_controller.applyMotion(motion);
    }
    const auto target = cursor_controller.getCursorPosition();

    if (linewise) {
        const auto first_line = std::min(start.y, target.y);
        const auto last_line = std::max(start.y, target.y);

        yank_buffer = document.copyLineRange(first_line, last_line);
        if (op == NormalModeOperator::Delete) {
            document.eraseLineRange(first_line, last_line);
            cursor_controller.setCursorPosition({0, first_line});
        }
        else {
            cursor_controller.setCursorPosition(start);
        }
        return;
    }

    auto range_start = start;
    auto range_end = target;
    if (comparePositions(range_end, range_start) < 0) {
        std::swap(range_start, range_end);
    }
    if (inclusive) {
        range_end = makeExclusiveEnd(document, range_end);
    }

    yank_buffer = document.copyRange(range_start.y, range_start.x, range_end.y, range_end.x);
    if (op == NormalModeOperator::Delete) {
        document.eraseRange(range_start.y, range_start.x, range_end.y, range_end.x);
        cursor_controller.setCursorPosition(range_start);
    }
    else {
        cursor_controller.setCursorPosition(start);
    }
}
