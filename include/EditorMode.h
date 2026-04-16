//
// Created by jan on 4/4/26.
//

#ifndef MODALEDITOR_EDITORMODE_H
#define MODALEDITOR_EDITORMODE_H
#include <optional>
#include <string_view>

enum class ModeType {
    NORMAL,
    INSERT,
    COMMAND
};

class IModeContext {
public:
    virtual ~IModeContext() = default;
    virtual void appendDebugMessage(std::string_view message) = 0;
};

class IInsertModeContext : public IModeContext{
public:
    ~IInsertModeContext() override = default;
};

class ICommandModeContext : public IModeContext {
public:
    ~ICommandModeContext() override = default;
    virtual void requestQuit() = 0;
};

class INormalModeContext : public IModeContext {
public:
    ~INormalModeContext() override = default;
    virtual void moveCursor(int key) = 0;
    virtual void movePage(int key) = 0;
    virtual void moveCursorLineStart() = 0;
    virtual void moveCursorLineEnd() = 0;
    virtual void moveCursorRightOne() = 0;
};


class EditorMode {
public:
    virtual ~EditorMode() = default;
    virtual std::string_view getName() const = 0;
};

class InsertMode : public EditorMode {
public:
    std::optional<ModeType> handle_input(IInsertModeContext& context, int key);

    std::string_view getName() const override {
        return "Insert";
    };
};

class CommandMode : public EditorMode {
public:
    std::optional<ModeType> handle_input(ICommandModeContext& context, int key);

    std::string_view getName() const override {
        return "Command";
    };
};

class NormalMode : public EditorMode {
public:
    std::optional<ModeType> handle_input(INormalModeContext& context, int key);

    std::string_view getName() const override {
        return "Normal";
    };
};

#endif //MODALEDITOR_EDITORMODE_H