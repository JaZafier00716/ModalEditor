//
// Created by jan on 4/4/26.
//

#ifndef MODALEDITOR_EDITORMODE_H
#define MODALEDITOR_EDITORMODE_H
#include <optional>
#include <string_view>

#include "TerminalManagement.h"

/** @brief Supported editor modes. */
enum class ModeType {
    NORMAL,
    INSERT,
    COMMAND
};

/**
 * @brief Shared mode callback interface implemented by the editor host.
 */
class IModeContext {
public:
    /** @brief Virtual destructor for polymorphic mode contexts. */
    virtual ~IModeContext() = default;
    /** @brief Appends a message to the editor debug/status area. */
    virtual void appendDebugMessage(std::string_view message) = 0;
};

/** @brief Context operations available while in insert mode. */
class IInsertModeContext : public IModeContext{
public:
    /** @brief Virtual destructor for polymorphic insert mode contexts. */
    ~IInsertModeContext() override = default;
    /** @brief Inserts a character at the current cursor location. */
    virtual void insertCharacter(char c) = 0;
    /** @brief Inserts a newline at the current cursor location. */
    virtual void insertNewLine() = 0;
    /** @brief Deletes the character before the cursor. */
    virtual void backspace() = 0;
};

/** @brief Context operations available while in command mode. */
class ICommandModeContext : public IModeContext {
public:
    /** @brief Virtual destructor for polymorphic command mode contexts. */
    ~ICommandModeContext() override = default;
    /** @brief Requests graceful editor shutdown. */
    virtual void requestQuit() = 0;
};

/** @brief Context operations available while in normal mode. */
class INormalModeContext : public IModeContext {
public:
    /** @brief Virtual destructor for polymorphic normal mode contexts. */
    ~INormalModeContext() override = default;
    /** @brief Applies a parsed motion command to the cursor. */
    virtual void applyMotion(const editor_motion::Motion& motion) = 0;
    /** @brief Moves cursor right by one character. */
    virtual void moveCursorRightOne() = 0;
};


/** @brief Base type for all editor mode state handlers. */
class EditorMode {
public:
    /** @brief Virtual destructor for polymorphic mode handlers. */
    virtual ~EditorMode() = default;
    /** @brief Human-readable mode name used by the UI. */
    [[nodiscard]] virtual std::string_view getName() const = 0;
};

/** @brief Implements key handling while inserting text. */
class InsertMode : public EditorMode {
public:
    /**
     * @brief Handles a keypress in insert mode.
     * @return Next mode to switch to, or std::nullopt to stay in insert mode.
     */
    std::optional<ModeType> handle_input(IInsertModeContext& context, const editor_input::Key& key);

    /** @brief Returns the display name of insert mode. */
    [[nodiscard]] std::string_view getName() const override {
        return "Insert";
    };
};

/** @brief Implements command entry behavior (e.g. quit commands). */
class CommandMode : public EditorMode {
public:
    /**
     * @brief Handles a keypress in command mode.
     * @return Next mode to switch to, or std::nullopt to stay in command mode.
     */
    std::optional<ModeType> handle_input(ICommandModeContext& context, const editor_input::Key& key);

    /** @brief Returns the display name of command mode. */
    [[nodiscard]] std::string_view getName() const override {
        return "Command";
    };
};

/** @brief Implements navigation-centric normal mode key handling. */
class NormalMode : public EditorMode {
public:
    /**
     * @brief Handles a keypress in normal mode.
     * @return Next mode to switch to, or std::nullopt to stay in normal mode.
     */
    std::optional<ModeType> handle_input(INormalModeContext& context, const editor_input::Key& key);

    /** @brief Returns the display name of normal mode. */
    [[nodiscard]] std::string_view getName() const override {
        return "Normal";
    };
};

#endif //MODALEDITOR_EDITORMODE_H