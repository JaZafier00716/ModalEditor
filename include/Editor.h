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
#include "CursorController.h"

using std::string, std::vector;


class Editor {
public:
    Editor();

    explicit Editor(const string& filename);
    void run();
private:
    class ModeContextGate final : public INormalModeContext, public IInsertModeContext, public ICommandModeContext {
    public:
        explicit ModeContextGate(Editor& editor) : editor{editor} {}

        void appendDebugMessage(std::string_view message) override;

        void moveCursor(int key) override;
        void movePage(int key) override;
        void moveCursorLineStart() override;
        void moveCursorLineEnd() override;
        void moveCursorRightOne() override;

        void requestQuit() override;

    private:
        Editor& editor;
    };


    void processKeypress();
    void refreshScreen();

    void appendDebugMessage(std::string_view message);
    void requestQuit();

    // Output methods
    void printRows(std::string &s);
    void scroll();
    void printMessage(std::string_view message, ...);

    [[nodiscard]] int2d getCursorPosition() const;
    std::string getRelativeLineNumber(const int y) const;

    CursorController cursor_controller;
    vector<string> rows;

    int2d screen_size{0,0};
    int render_pos{0}; // for tab rendering
    int2d offset{0,0}; // col and row offset
    int line_num_offset{0}; // offset for line numbers
    string file_name;
    string output_buffer;
    string debug_info_message;

    std::variant<NormalMode, InsertMode, CommandMode> current_mode{NormalMode{}};
    ModeContextGate mode_context{*this};
    bool should_exit{false};

    // Config
    int tab_spaces = 4;
    bool line_numbers = true;
};

#endif //MODALEDITOR_EDITOR_H