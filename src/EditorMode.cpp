//
// Created by jan on 4/4/26.
//

#include "../include/EditorMode.h"
#include "../include/Editor.h"

std::unique_ptr<EditorMode> InsertMode::handle_input(Editor& editor, const int key) {
    if (key == static_cast<int>(EditorKey::ESCAPE_KEY)) {
        editor.appendDebugMessage("ESC: Switching to normal mode");
        return std::make_unique<NormalMode>();
    }
    return nullptr;
}

std::unique_ptr<EditorMode> CommandMode::handle_input(Editor& editor, const int key) {
    if (key == static_cast<int>(EditorKey::ESCAPE_KEY)) {
        editor.appendDebugMessage("ESC: Switching to normal mode");
        return std::make_unique<NormalMode>();
    }
    if (key == 'q') {
        // Clear the screen
        terminal_manager::clear_screen();
        std::exit(0);
    }
    return nullptr;
}

std::unique_ptr<EditorMode> NormalMode::handle_input(Editor& editor, const int key) {
    switch (key) {
        case static_cast<int>(EditorKey::ARROW_UP):
        case static_cast<int>(EditorKey::ARROW_DOWN):
        case static_cast<int>(EditorKey::ARROW_RIGHT):
        case static_cast<int>(EditorKey::ARROW_LEFT):
        case 'j': // move down
        case 'k': // move up
        case 'h': // move left
        case 'l': // move right
            editor.appendDebugMessage("Moving cursor");
            editor.moveCursor(key);
            break;

        case static_cast<int>(EditorKey::PAGE_UP):
        case static_cast<int>(EditorKey::PAGE_DOWN):
            // Move editor by full screen
            editor.appendDebugMessage("Moving page");
            editor.movePage(key);
            break;

        // Move the cursor to the start of the line
        case static_cast<int>(EditorKey::HOME_KEY):
            editor.appendDebugMessage("Moving cursor start");
            editor.setCursorPositionX(0);
            break;
        // Move the cursor to the end of the line
        case static_cast<int>(EditorKey::END_KEY):
            editor.appendDebugMessage("Moving cursor end");
            editor.moveCursorLineEnd();
            break;

        // Mode switching
        case 'i':
            editor.appendDebugMessage("i: Switching to insert mode");
            return std::make_unique<InsertMode>();
        case 'I':
            editor.appendDebugMessage("I: Switching to insert mode at line start");
            editor.setCursorPositionX(0);
            return std::make_unique<InsertMode>();
        case 'A':
            editor.appendDebugMessage("A: Switching to insert mode at line end");
            editor.moveCursorLineEnd();
            return std::make_unique<InsertMode>();
        case 'a':
            editor.appendDebugMessage("a: Switching to insert mode at next char");
            editor.setCursorPositionX(editor.getCursorPosition().x + 1);
            return std::make_unique<InsertMode>();
        case ':':
            editor.appendDebugMessage("c: Switching to command mode");
            return std::make_unique<CommandMode>();
        default:
             editor.appendDebugMessage("Unknown key");
    }
    return nullptr;
}
