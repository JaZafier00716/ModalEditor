//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_EDITORVIEW_H
#define MODALEDITOR_EDITORVIEW_H
#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include "CursorController.h"
#include "EditorMode.h"

using std::vector, std::string;

class EditorView {
public:
    EditorView(
        TextDocument& document,
        int2d& screen_size,
        CursorController& cursor_controller
    ) :
        document{document},
        screen_size{screen_size},
        cursor_controller{cursor_controller}
    {};

    void refreshScreen(const std::variant<NormalMode, InsertMode, CommandMode>& current_mode);
    void setFileName(std::string_view text);
    void appendDebugMessage(std::string_view message);
    void setLineNumbersEnabled(bool enabled);

private:
    void printRows(string &s);
    void scroll();
    [[nodiscard]] string getRelativeLineNumber(int y) const;


    TextDocument& document;
    int2d& screen_size;
    CursorController& cursor_controller;
    string file_name;
    string debug_info_message;
    bool line_numbers{true};
    int tab_spaces = 4;

    int render_pos{0}; // for tab rendering
    int2d offset{0,0}; // col and row offset
    int line_num_offset{0}; // offset for line numbers
    string output_buffer;
};

#endif //MODALEDITOR_EDITORVIEW_H
