//
// Created by jan on 3/19/26.
//
#include "../include/Editor.h"
#include <iostream>
#include <ranges>

Editor::Editor() : Editor("") {}

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


void Editor::run() {
    // terminal_manager::clear_screen();
    while (!should_exit) {
        view.refreshScreen(current_mode);
        processKeypress();
    }
    terminal_manager::clear_screen();
}



void Editor::requestQuit() {
    should_exit = true;
}

void Editor::processKeypress() {
    const auto c = terminal_manager::readKey();
    view.appendDebugMessage(std::format("Key pressed: {}", c));
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
    editor.view.appendDebugMessage(message);
}

void Editor::ModeContextGate::insertCharacter(const char c) {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    if (editor.document.insertCharAt(line, col, c)) {
        editor.cursor_controller.setCursorPositionX(col + 1);
    }
}

void Editor::ModeContextGate::insertNewLine() {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    if (editor.document.insertNewlineAt(line, col)) {
        editor.cursor_controller.moveCursor('j');
        editor.cursor_controller.setCursorPositionX(0);
    }
}

void Editor::ModeContextGate::backspace() {
    const auto [col, line] = editor.cursor_controller.getCursorPosition();
    if (editor.document.eraseCharBefore(line, col)) {
        editor.cursor_controller.setCursorPositionX(line - 1);
    }
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

