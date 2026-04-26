//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <iostream>
#include <ranges>

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

    screen_size.y -= 2; // Reserve 2 lines for status bar and command line
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
    if (editor.document.eraseCharBefore(line, col)) {
        editor.cursor_controller.setCursorPositionX(line - 1);
    }
}

/** @brief Moves the cursor one position to the right. */
void Editor::ModeContextGate::moveCursorRightOne() {
    editor.cursor_controller.moveCursorRightOne();
}

/** @brief Requests editor shutdown through the owning Editor instance. */
void Editor::ModeContextGate::requestQuit() {
    editor.requestQuit();
}

void Editor::ModeContextGate::saveToFile() {
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

