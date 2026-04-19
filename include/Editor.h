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
#include "EditorView.h"
#include "TextDocument.h"

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

        // INSERT MODE
        void insertCharacter(char c) override;
        void insertNewLine() override;
        void backspace() override;

        // NORMAL MODE
        void moveCursor(int key) override;
        void movePage(int key) override;
        void moveCursorLineStart() override;
        void moveCursorLineEnd() override;
        void moveCursorRightOne() override;

        // COMMAND MODE
        void requestQuit() override;

    private:
        Editor& editor;
    };


    void processKeypress();
    void requestQuit();

    TextDocument document;
    int2d screen_size{0,0};
    CursorController cursor_controller;
    EditorView view;

    std::variant<NormalMode, InsertMode, CommandMode> current_mode{NormalMode{}};
    ModeContextGate mode_context{*this};
    bool should_exit{false};
};

#endif //MODALEDITOR_EDITOR_H