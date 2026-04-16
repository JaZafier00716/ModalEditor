//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_CURSORCONTROLLER_H
#define MODALEDITOR_CURSORCONTROLLER_H

#include <string>
#include <vector>

#include "TerminalManagement.h"

using std::string, std::vector;

class CursorController {
public:
    CursorController(
        vector<string>& rows,
        int2d& screen_size
    ) :
    rows{rows},
    screen_size{screen_size} {};

    void moveCursor(int key);
    void movePage(int key);
    void setCursorPositionX(int x);
    void moveCursorLineStart();
    void moveCursorLineEnd();
    void moveCursorRightOne();

    [[nodiscard]] int2d getCursorPosition() const { return cursor_pos; }
    [[nodiscard]] int getDesiredCursorPos() const { return desired_cursor_pos; }

private:
    [[nodiscard]] int getRowLength(int y) const;
    void clampCursorPosition();

    vector<string>& rows;
    int2d cursor_pos{0,0};
    int desired_cursor_pos{0};
    int2d& screen_size;
};

#endif //MODALEDITOR_CURSORCONTROLLER_H
