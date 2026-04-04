//
// Created by jan on 3/19/26.
//

#ifndef MODALEDITOR_EDITOR_H
#define MODALEDITOR_EDITOR_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "TerminalManagement.h"
#include "EditorMode.h"

using std::string, std::vector;


class Editor {
public:
    Editor();

    explicit Editor(const string& filename) : Editor() {
        this->message = filename.empty() ? "new file" : filename;
    }
    void run();
    void processKeypress();
    void refreshScreen() const;

    // Input methods
    void moveCursor(int key);
    void movePage(int key);

    void setCursorPositionX(const int x) {
        if (cursor_pos.y < rows.size()) {
            cursor_pos.x = std::min(x, static_cast<int>(rows[cursor_pos.y].size()));
        } else {
             cursor_pos.x = x;
        }
    }
    void setCursorPositionY(const int y) {
        cursor_pos.y = y;
    }
    int2d getCursorPosition() const {
        return cursor_pos;
    }

    void moveCursorLineEnd() {
        if (cursor_pos.y < rows.size()) {
            cursor_pos.x = rows[cursor_pos.y].size();
        }
    }
private:
    vector<string> rows;
    int2d screen_size{0,0};
    int2d cursor_pos{0,0};
    int desired_cursor_pos{0};
    int render_pos{0}; // for tab rendering
    int2d offset{0,0}; // col and row offset
    string message;


    // Editor Modes
    std::unique_ptr<EditorMode> current_mode = std::make_unique<NormalMode>();


    // Output methods
    void printRows(std::string &s);
    void scroll();
    void printMessage(std::string_view message, ...);

    void moveCursorLeft();

    int get_row_length(const int y) const {
        if (y < 0 || y >= static_cast<int>(rows.size())) {
            return 0;
        }
        return static_cast<int>(rows[y].size());
    }

    void clampCursorPosition() {
        if (cursor_pos.y < 0) {
            cursor_pos.y = 0;
        }
        const auto max_y = rows.empty() ? 0 : static_cast<int>(rows.size()-1);
        if (cursor_pos.y > max_y) {
            cursor_pos.y = max_y;
        }

        const auto row_length = get_row_length(cursor_pos.y);
        if (cursor_pos.x < 0) {
            cursor_pos.x = 0;
        }
        if (cursor_pos.x > row_length) {
            cursor_pos.x = row_length;
        }
    }

    // Config
    int tab_spaces = 4;
    bool line_numbers = true;
};

#endif //MODALEDITOR_EDITOR_H