//
// Created by jan on 4/16/26.
//
#include <algorithm>

#include "../include/CursorController.h"

void CursorController::moveCursor(const int key) {
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
            if (cursor_pos.x < getRowLength(cursor_pos.y)) {
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
                cursor_pos.x = getRowLength(cursor_pos.y); // Move to end of prev line
            }
            desired_cursor_pos = cursor_pos.x; // Update desired position for vertical movement
            break;
        default: break;
    }
    clampCursorPosition();
}

void CursorController::movePage(const int key) {
    if (key == static_cast<int>(EditorKey::PAGE_UP)) {
        cursor_pos.y = std::max(cursor_pos.y - screen_size.y, 0);
    } else if (key == static_cast<int>(EditorKey::PAGE_DOWN)) {
        cursor_pos.y = std::min(cursor_pos.y + screen_size.y, static_cast<int>(rows.size()) - 1);
    }
    clampCursorPosition();
}

void CursorController::moveCursorLineStart() {
    setCursorPositionX(0);
}

void CursorController::setCursorPositionX(const int x) {
    if (cursor_pos.y < rows.size()) {
        cursor_pos.x = std::min(x, static_cast<int>(rows[cursor_pos.y].size()));
    } else {
        cursor_pos.x = x;
    }
}

void CursorController::moveCursorLineEnd() {
    if (cursor_pos.y < rows.size()) {
        cursor_pos.x = rows[cursor_pos.y].size();
    }
}

void CursorController::moveCursorRightOne() {
    setCursorPositionX(cursor_pos.x + 1);
}

int CursorController::getRowLength(const int y) const {
    if (y < 0 || y >= static_cast<int>(rows.size())) {
        return 0;
    }
    return static_cast<int>(rows[y].size());
}

void CursorController::clampCursorPosition() {
    const auto max_y = rows.empty() ? 0 : static_cast<int>(rows.size()-1);
    cursor_pos.y = std::clamp(cursor_pos.y, 0, max_y);

    const auto row_length = getRowLength(cursor_pos.y);
    cursor_pos.x = std::clamp(cursor_pos.x, 0, row_length);
}
