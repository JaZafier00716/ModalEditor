//
// Created by jan on 4/4/26.
//

#ifndef MODALEDITOR_EDITORMODE_H
#define MODALEDITOR_EDITORMODE_H
#include <memory>
#include "TerminalManagement.h"
class Editor;

class EditorMode {
public:
    virtual ~EditorMode() = default;

    virtual std::unique_ptr<EditorMode> handle_input(Editor& editor, int key) = 0;
};

class InsertMode : public EditorMode {
    std::unique_ptr<EditorMode> handle_input(Editor& editor, const int key) override;
};

class CommandMode : public EditorMode {
    std::unique_ptr<EditorMode> handle_input(Editor& editor, const int key) override;
};

class NormalMode : public EditorMode {
    std::unique_ptr<EditorMode> handle_input(Editor& editor, const int key) override;
};

#endif //MODALEDITOR_EDITORMODE_H