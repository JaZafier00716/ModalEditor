//
// Created by jan on 4/4/26.
//

#include "../include/EditorMode.h"
#include "../include/Editor.h"

std::optional<ModeType> InsertMode::handle_input(IInsertModeContext& context, const int key) {
    if (key == static_cast<int>(EditorKey::ESCAPE_KEY)) {
        context.appendDebugMessage("ESC: Switching to normal mode");
        return ModeType::NORMAL;
    }
    return std::nullopt;
}

std::optional<ModeType> CommandMode::handle_input(ICommandModeContext& context, const int key) {
    if (key == static_cast<int>(EditorKey::ESCAPE_KEY)) {
        context.appendDebugMessage("ESC: Switching to normal mode");
        return ModeType::NORMAL;
    }
    if (key == 'q') {
        // Clear the screen
        context.appendDebugMessage("q: Requesting editor shutdown");
        context.requestQuit();
    }
    return std::nullopt;
}

std::optional<ModeType> NormalMode::handle_input(INormalModeContext& context, const int key) {
    string direction;
    switch (key) {
        case static_cast<int>(EditorKey::ARROW_UP):
        case 'k': // move up
             context.appendDebugMessage("Moving cursor up");
             context.moveCursor(key);
             break;
        case static_cast<int>(EditorKey::ARROW_DOWN):
        case 'j': // move down
            context.appendDebugMessage("Moving cursor down");
            context.moveCursor(key);
            break;
        case static_cast<int>(EditorKey::ARROW_RIGHT):
        case 'l': // move right
            context.appendDebugMessage("Moving cursor right");
            context.moveCursor(key);
            break;
        case static_cast<int>(EditorKey::ARROW_LEFT):
        case 'h': // move left
            context.appendDebugMessage("Moving cursor left");
            context.moveCursor(key);
            break;
        case static_cast<int>(EditorKey::PAGE_UP):
            // Move editor by full screen up
            context.appendDebugMessage("Moving page up");
            context.movePage(key);
            break;
        case static_cast<int>(EditorKey::PAGE_DOWN):
            // Move editor by full screen down
            context.appendDebugMessage("Moving page down");
            context.movePage(key);
            break;

        // Move the cursor to the start of the line
        case static_cast<int>(EditorKey::HOME_KEY):
            context.appendDebugMessage("Moving cursor start");
            context.moveCursorLineStart();
            break;
        // Move the cursor to the end of the line
        case static_cast<int>(EditorKey::END_KEY):
            context.appendDebugMessage("Moving cursor end");
            context.moveCursorLineEnd();
            break;

        // Mode switching
        case 'i':
            context.appendDebugMessage("i: Switching to insert mode");
            return ModeType::INSERT;
        case 'I':
            context.appendDebugMessage("I: Switching to insert mode at line start");
            context.moveCursorLineStart();
            return ModeType::INSERT;
        case 'A':
            context.appendDebugMessage("A: Switching to insert mode at line end");
            context.moveCursorLineEnd();
            return ModeType::INSERT;
        case 'a':
            context.appendDebugMessage("a: Switching to insert mode at next char");
            context.moveCursorRightOne();
            return ModeType::INSERT;
        case ':':
            context.appendDebugMessage("c: Switching to command mode");
            return ModeType::COMMAND;
        default:
             context.appendDebugMessage("Unknown key");
    }
    return std::nullopt;
}
