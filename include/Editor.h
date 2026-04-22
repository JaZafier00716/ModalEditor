//
// Created by jan on 3/19/26.
//

#ifndef MODALEDITOR_EDITOR_H
#define MODALEDITOR_EDITOR_H

#include <iostream>
#include <string>
#include <vector>

#include "TerminalManagement.h"
#include "EditorMode.h"
#include "CursorController.h"
#include "EditorView.h"
#include "TextDocument.h"

using std::string, std::vector;


/**
 * @brief Coordinates input handling, document state, and rendering.
 */
class Editor {
public:
    /** @brief Creates an editor with an empty document. */
    Editor();

    /** @brief Creates an editor and attempts to load the provided file. */
    explicit Editor(const string& filename);
    /** @brief Starts the interactive editor loop. */
    void run();
private:
    /**
     * @brief Adapts editor operations to the mode context interfaces.
     */
    class ModeContextGate final : public INormalModeContext, public IInsertModeContext, public ICommandModeContext {
    public:
        /** @brief Creates a mode context bridge bound to the owning editor. */
        explicit ModeContextGate(Editor& editor) : editor{editor} {}

        /** @brief Forwards status/debug output from mode handlers. */
        void appendDebugMessage(std::string_view message) override;

        // INSERT MODE
        /** @brief Inserts a character using editor editing operations. */
        void insertCharacter(char c) override;
        /** @brief Inserts a newline using editor editing operations. */
        void insertNewLine() override;
        /** @brief Deletes backward using editor editing operations. */
        void backspace() override;

        // NORMAL MODE
        /** @brief Applies a parsed movement command to the cursor controller. */
        void applyMotion(const editor_motion::Motion& motion) override;
        /** @brief Moves cursor right by one character. */
        void moveCursorRightOne() override;

        // COMMAND MODE
        /** @brief Requests editor shutdown. */
        void requestQuit() override;

    private:
        Editor& editor;
    };


    /** @brief Reads and dispatches one input event. */
    void processKeypress();
    /** @brief Marks the editor loop for exit. */
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