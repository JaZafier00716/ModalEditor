//
// Created by jan on 4/4/26.
//

#include "../include/EditorMode.h"

#include <cctype>
#include <type_traits>

namespace {
    struct MotionCommand {
        editor_motion::Motion motion{};
        bool linewise{false};
        bool inclusive{false};
        bool uses_count{true};
    };

    struct EnterInsert {};
    struct EnterInsertAtLineStart{};
    struct EnterInsertAtLineEnd{};
    struct EnterInsertAfterCursor{};

    struct EnterCommandMode{};

    template<typename T>
    /**
     * @brief Checks whether no modifiers are active on a key event.
     * @param key Key event payload.
     * @return True when shift, ctrl, and alt are all false.
     */
    bool hasNoModifiers(const T& key) {
        return !key.modifiers.shift && !key.modifiers.ctrl && !key.modifiers.alt;
    }

    template<typename T, typename = void>
    struct hasStaticName : std::false_type {};

    template<typename T>
    struct hasStaticName<T, std::void_t<decltype(T::name)>> : std::true_type {};

    template<typename T>
    constexpr bool hasStaticNameV = hasStaticName<T>::value;

    /**
     * @brief Extracts a plain character key for normal-mode command parsing.
     * @param key Parsed key event.
     * @return Printable character when no modifiers are active.
     */
    std::optional<char> normalModeCharacter(const editor_input::Key& key) {
        if (const auto* c = std::get_if<editor_input::Character>(&key); c != nullptr && hasNoModifiers(*c)) {
            return c->value;
        }
        return std::nullopt;
    }

    /**
     * @brief Maps non-character keys to motion commands.
     * @param key Parsed key event.
     * @param operator_pending True when resolving an operator target.
     * @return Motion command, or std::nullopt if the key is not a motion.
     */
    std::optional<MotionCommand> motionFromSpecialKey(const editor_input::Key& key, const bool operator_pending) {
        return std::visit([operator_pending](const auto& value) -> std::optional<MotionCommand> {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, editor_input::ArrowUp>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::Up{}, operator_pending, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowDown>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::Down{}, operator_pending, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowLeft>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::Left{}, false, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowRight>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::Right{}, false, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::PageUp>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::PageUp{}, operator_pending, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::PageDown>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::PageDown{}, operator_pending, false, true};
            }
            else if constexpr (std::is_same_v<T, editor_input::Home>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::LineStart{}, false, false, false};
            }
            else if constexpr (std::is_same_v<T, editor_input::End>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return MotionCommand{editor_motion::LineEnd{}, false, false, false};
            }
            else if constexpr (hasStaticNameV<T>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return std::nullopt;
            }
            else {
                return std::nullopt;
            }
        }, key);
    }

    /**
     * @brief Maps normal-mode characters to motion commands.
     * @param c Character key.
     * @param operator_pending True when resolving an operator target.
     * @return Motion command, or std::nullopt if the key is not a motion.
     */
    std::optional<MotionCommand> motionFromCharacter(const char c, const bool operator_pending) {
        switch (c) {
            case 'h': return MotionCommand{editor_motion::Left{}, false, false, true};
            case 'j': return MotionCommand{editor_motion::Down{}, operator_pending, false, true};
            case 'k': return MotionCommand{editor_motion::Up{}, operator_pending, false, true};
            case 'l': return MotionCommand{editor_motion::Right{}, false, false, true};
            case '0': return MotionCommand{editor_motion::LineStart{}, false, false, false};
            case '$': return MotionCommand{editor_motion::LineEnd{}, false, false, false};
            case 'G': return MotionCommand{editor_motion::FileEnd{}, true, false, false};
            case 'w': return MotionCommand{editor_motion::WordForward{.big = false}, false, false, true};
            case 'W': return MotionCommand{editor_motion::WordForward{.big = true}, false, false, true};
            case 'b': return MotionCommand{editor_motion::WordBackward{.big = false}, false, false, true};
            case 'B': return MotionCommand{editor_motion::WordBackward{.big = true}, false, false, true};
            case 'e': return MotionCommand{editor_motion::WordEnd{.big = false}, false, true, true};
            case 'E': return MotionCommand{editor_motion::WordEnd{.big = true}, false, true, true};
            case 'H': return MotionCommand{editor_motion::ScreenTop{}, false, false, false};
            case 'M': return MotionCommand{editor_motion::ScreenMiddle{}, false, false, false};
            case 'L': return MotionCommand{editor_motion::ScreenBottom{}, false, false, false};
            case '{': return MotionCommand{editor_motion::ParagraphBackward{}, true, false, true};
            case '}': return MotionCommand{editor_motion::ParagraphForward{}, true, false, true};
            case '%': return MotionCommand{editor_motion::MatchingBracket{}, false, true, false};
            case '&': return MotionCommand{editor_motion::MatchingBrace{}, false, true, false};
            default: return std::nullopt;
        }
    }

    /**
     * @brief Returns the effective count value for a numeric prefix.
     * @param count Accumulated count digits.
     * @return Prefix count, defaulting to 1.
     */
    int resolvedCount(const int count) {
        return count == 0 ? 1 : count;
    }

    /**
     * @brief Appends a digit to an in-progress count.
     * @param count Existing count value.
     * @param digit Newly typed digit.
     * @return Updated count.
     */
    int appendDigit(const int count, const char digit) {
        return count * 10 + (digit - '0');
    }
}

/**
 * @brief Handles one key event while in insert mode.
 * @param context Insert-mode callback interface.
 * @param key Parsed key event.
 * @return Next mode to switch to, or std::nullopt to remain in insert mode.
 */
std::optional<ModeType> InsertMode::handle_input(IInsertModeContext& context, const editor_input::Key& key) {
    if (std::holds_alternative<editor_input::Escape>(key)) {
        context.appendDebugMessage("ESC: Switching to normal mode");
        return ModeType::NORMAL;
    }

    if (std::holds_alternative<editor_input::Enter>(key)) {
        context.insertNewLine();
        return std::nullopt;
    }

    if (std::holds_alternative<editor_input::Tab>(key)) {
        context.insertCharacter('\t');
        return std::nullopt;
    }

    if (std::holds_alternative<editor_input::Backspace>(key)) {
        context.backspace();
        return std::nullopt;
    }

    if (const auto* c = std::get_if<editor_input::Character>(&key)) {
        if (c->value == '\r' || c->value == '\n') {
            context.insertNewLine();
            return std::nullopt;
        }

        if (static_cast<unsigned char>(c->value) >= 32 && static_cast<unsigned char>(c->value) <= 126) {
            context.insertCharacter(c->value);
            return std::nullopt;
        }
    }

    return std::nullopt;
}

/**
 * @brief Handles one key event while in command mode.
 * @param context Command-mode callback interface.
 * @param key Parsed key event.
 * @return Next mode to switch to, or std::nullopt to remain in command mode.
 */
std::optional<ModeType> CommandMode::handle_input(ICommandModeContext& context, const editor_input::Key& key) {
    if (std::holds_alternative<editor_input::Escape>(key)) {
        context.appendDebugMessage("ESC: Switching to normal mode");
        return ModeType::NORMAL;
    }
    const auto* c = std::get_if<editor_input::Character>(&key);
    if (c != nullptr && c->value == 'q') {
        // Clear the screen
        context.appendDebugMessage("q: Requesting editor shutdown");
        context.requestQuit();
    }
    if (c != nullptr && c->value == 'w') {
        // Save the file
        context.appendDebugMessage("w: Requesting file save");
        context.saveToFile();
    }
    return std::nullopt;
}

/**
 * @brief Handles one key event while in normal mode.
 * @param context Normal-mode callback interface.
 * @param key Parsed key event.
 * @return Next mode to switch to, or std::nullopt to remain in normal mode.
 */
std::optional<ModeType> NormalMode::handle_input(INormalModeContext& context, const editor_input::Key& key) {
    auto resetPendingState = [this]() {
        command_count = 0;
        motion_count = 0;
        awaiting_g = false;
        pending_operator.reset();
    };

    auto finishMotion = [&](const MotionCommand& command) {
        const auto repeat = command.uses_count ? resolvedCount(command_count) : 1;
        context.appendDebugMessage("Applying motion");
        for (int i = 0; i < repeat; ++i) {
            context.applyMotion(command.motion);
        }
        resetPendingState();
    };

    auto finishOperator = [&](const MotionCommand& command) {
        const auto repeat = command.uses_count
            ? resolvedCount(command_count) * resolvedCount(motion_count)
            : 1;
        context.appendDebugMessage("Applying operator");
        context.applyOperator(*pending_operator, command.motion, repeat, command.linewise, command.inclusive);
        resetPendingState();
    };

    auto handleInsertCommand = [&](const auto& command) -> std::optional<ModeType> {
        if (pending_operator.has_value()) {
            context.appendDebugMessage("Unknown key");
            resetPendingState();
            return std::nullopt;
        }

        if constexpr (std::is_same_v<std::decay_t<decltype(command)>, EnterInsert>) {
            context.appendDebugMessage("Switching to insert mode");
            resetPendingState();
            return ModeType::INSERT;
        }
        else if constexpr (std::is_same_v<std::decay_t<decltype(command)>, EnterInsertAtLineStart>) {
            context.appendDebugMessage("Switching to insert mode at line start");
            context.applyMotion(editor_motion::LineStart{});
            resetPendingState();
            return ModeType::INSERT;
        }
        else if constexpr (std::is_same_v<std::decay_t<decltype(command)>, EnterInsertAtLineEnd>) {
            context.appendDebugMessage("Switching to insert mode at line end");
            context.applyMotion(editor_motion::LineEnd{});
            resetPendingState();
            return ModeType::INSERT;
        }
        else if constexpr (std::is_same_v<std::decay_t<decltype(command)>, EnterInsertAfterCursor>) {
            context.appendDebugMessage("Switching to insert mode after cursor");
            context.moveCursorRightOne();
            resetPendingState();
            return ModeType::INSERT;
        }
        else {
            context.appendDebugMessage("Switching to command mode");
            resetPendingState();
            return ModeType::COMMAND;
        }
    };

    if (std::holds_alternative<editor_input::Escape>(key)) {
        if (command_count != 0 || motion_count != 0 || awaiting_g || pending_operator.has_value()) {
            context.appendDebugMessage("Cancelled pending command");
            resetPendingState();
        }
        return std::nullopt;
    }

    if (awaiting_g) {
        if (const auto c = normalModeCharacter(key); c.has_value() && *c == 'g') {
            const MotionCommand command{editor_motion::FileStart{}, true, false, false};
            if (pending_operator.has_value()) {
                finishOperator(command);
            }
            else {
                finishMotion(command);
            }
            return std::nullopt;
        }

        awaiting_g = false;
    }

    if (const auto special_motion = motionFromSpecialKey(key, pending_operator.has_value()); special_motion.has_value()) {
        if (pending_operator.has_value()) {
            finishOperator(*special_motion);
        }
        else {
            finishMotion(*special_motion);
        }
        return std::nullopt;
    }

    const auto c = normalModeCharacter(key);
    if (!c.has_value()) {
        context.appendDebugMessage("Unknown key");
        resetPendingState();
        return std::nullopt;
    }

    if (*c >= '1' && *c <= '9') {
        if (pending_operator.has_value()) {
            motion_count = appendDigit(motion_count, *c);
        }
        else {
            command_count = appendDigit(command_count, *c);
        }
        return std::nullopt;
    }

    if (*c == '0') {
        if (pending_operator.has_value() && motion_count != 0) {
            motion_count = appendDigit(motion_count, *c);
            return std::nullopt;
        }

        if (!pending_operator.has_value() && command_count != 0) {
            command_count = appendDigit(command_count, *c);
            return std::nullopt;
        }
    }

    if (*c == 'd' || *c == 'y') {
        const auto op = *c == 'd' ? NormalModeOperator::Delete : NormalModeOperator::Yank;
        if (!pending_operator.has_value()) {
            pending_operator = op;
            motion_count = 0;
            awaiting_g = false;
            return std::nullopt;
        }

        if (*pending_operator == op) {
            const auto total_lines = resolvedCount(command_count) * resolvedCount(motion_count);
            context.appendDebugMessage("Applying operator");
            context.applyOperator(op, editor_motion::Down{}, total_lines - 1, true, false);
            resetPendingState();
            return std::nullopt;
        }

        context.appendDebugMessage("Unknown key");
        resetPendingState();
        return std::nullopt;
    }

    if (*c == 'g') {
        awaiting_g = true;
        return std::nullopt;
    }

    if (const auto motion = motionFromCharacter(*c, pending_operator.has_value()); motion.has_value()) {
        if (pending_operator.has_value()) {
            finishOperator(*motion);
        }
        else {
            finishMotion(*motion);
        }
        return std::nullopt;
    }

    switch (*c) {
        case 'i': return handleInsertCommand(EnterInsert{});
        case 'I': return handleInsertCommand(EnterInsertAtLineStart{});
        case 'A': return handleInsertCommand(EnterInsertAtLineEnd{});
        case 'a': return handleInsertCommand(EnterInsertAfterCursor{});
        case ':': return handleInsertCommand(EnterCommandMode{});
        default:
            context.appendDebugMessage("Unknown key");
            resetPendingState();
            return std::nullopt;
    }
}
