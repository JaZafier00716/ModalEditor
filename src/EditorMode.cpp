//
// Created by jan on 4/4/26.
//

#include "../include/EditorMode.h"

#include <array>

namespace {
    struct MoveByMotion { editor_motion::Motion motion{}; };

    struct EnterInsert {};
    struct EnterInsertAtLineStart{};
    struct EnterInsertAtLineEnd{};
    struct EnterInsertAfterCursor{};

    struct EnterCommandMode{};

    using NormalCommand = std::variant<
        std::monostate,
        MoveByMotion,
        EnterInsert,
        EnterInsertAtLineStart,
        EnterInsertAtLineEnd,
        EnterInsertAfterCursor,
        EnterCommandMode
    >;

    struct NormalKeyBinding {
        std::string_view key_id;
        NormalCommand command;
    };

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
     * @brief Builds a normalized key identifier used by normal-mode bindings.
     * @param key Parsed key event.
     * @return Binding id string (e.g. "char:h") or std::nullopt if unsupported.
     */
    std::optional<std::string> normalModeKeyId(const editor_input::Key& key) {
        return std::visit([](const auto& value) -> std::optional<std::string> {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, editor_input::Character>) {
                if (hasNoModifiers(value)) {
                    return std::string{"char:"} + value.value;
                }
                return std::nullopt;
            } else if constexpr (hasStaticNameV<T>) {
                if (hasNoModifiers(value)) {
                    return std::string{T::name};
                }
                return std::nullopt;
            }
            return std::nullopt;
        }, key);
    }

    /**
     * @brief Returns the default movement and mode-switch key map.
     * @return Static table of key bindings.
     */
    const auto& defaultNormalModeKeyBindings() {
        static const std::array<NormalKeyBinding, 24> bindings{{
            {"ArrowUp", MoveByMotion{editor_motion::Up{}}},
            {"ArrowDown", MoveByMotion{editor_motion::Down{}}},
            {"ArrowRight", MoveByMotion{editor_motion::Right{}}},
            {"ArrowLeft", MoveByMotion{editor_motion::Left{}}},
            {"PageUp", MoveByMotion{editor_motion::PageUp{}}},
            {"PageDown", MoveByMotion{editor_motion::PageDown{}}},
            {"Home", MoveByMotion{editor_motion::LineStart{}}},
            {"End", MoveByMotion{editor_motion::LineEnd{}}},
            {"char:k", MoveByMotion{editor_motion::Up{}}},
            {"char:j", MoveByMotion{editor_motion::Down{}}},
            {"char:l", MoveByMotion{editor_motion::Right{}}},
            {"char:h", MoveByMotion{editor_motion::Left{}}},
            {"char:g", MoveByMotion{editor_motion::FileStart{}}},
            {"char:G", MoveByMotion{editor_motion::FileEnd{}}},
            {"char:w", MoveByMotion{editor_motion::WordForward{}}},
            {"char:b", MoveByMotion{editor_motion::WordBackward{}}},
            {"char:H", MoveByMotion{editor_motion::ScreenTop{}}},
            {"char:M", MoveByMotion{editor_motion::ScreenMiddle{}}},
            {"char:L", MoveByMotion{editor_motion::ScreenBottom{}}},
            {"char:{", MoveByMotion{editor_motion::ParagraphBackward{}}},
            {"char:}", MoveByMotion{editor_motion::ParagraphForward{}}},
            {"char:%", MoveByMotion{editor_motion::MatchingBracket{}}},
            {"char:&", MoveByMotion{editor_motion::MatchingBrace{}}},
            {"char:i", EnterInsert{}},
        }};
        return bindings;
    }

    /**
     * @brief Returns the default action-only key map for normal mode.
     * @return Static table of key bindings.
     */
    const auto& defaultNormalModeActionBindings() {
        static const std::array<NormalKeyBinding, 13> bindings{{
            {"char:I", EnterInsertAtLineStart{}},
            {"char:A", EnterInsertAtLineEnd{}},
            {"char:a", EnterInsertAfterCursor{}},
            {"char::", EnterCommandMode{}},
        }};
        return bindings;
    }

    /**
     * @brief Resolves a key event into a normal-mode command variant.
     * @param k Parsed key event.
     * @return Command to execute, or std::monostate if unknown.
     */
    NormalCommand resolveNormalModeCommand(const editor_input::Key& k) {
        const auto key_id = normalModeKeyId(k);

        if (!key_id.has_value()) {
            return std::monostate{};
        }

        for (const auto& [key, command] : defaultNormalModeKeyBindings()) {
            if (key == key_id.value()) {
                return command;
            }
        }

        for (const auto& [key, command] : defaultNormalModeActionBindings()) {
            if (key == key_id.value()) {
                return command;
            }
        }

        return std::monostate{};
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
    const auto command = resolveNormalModeCommand(key);

    if (const auto* move = std::get_if<MoveByMotion>(&command)) {
        context.appendDebugMessage("Applying motion");
        context.applyMotion(move->motion);
        return std::nullopt;
    }

    if (std::holds_alternative<EnterInsert>(command)) {
        context.appendDebugMessage("Switching to insert mode");
        return ModeType::INSERT;
    }

    if (std::holds_alternative<EnterInsertAtLineStart>(command)) {
        context.appendDebugMessage("Switching to insert mode at line start");
        context.applyMotion(editor_motion::LineStart{});
        return ModeType::INSERT;
    }

    if (std::holds_alternative<EnterInsertAtLineEnd>(command)) {
        context.appendDebugMessage("Switching to insert mode at line end");
        context.applyMotion(editor_motion::LineEnd{});
        return ModeType::INSERT;
    }

    if (std::holds_alternative<EnterInsertAfterCursor>(command)) {
        context.appendDebugMessage("Switching to insert mode after cursor");
        context.moveCursorRightOne();
        return ModeType::INSERT;
    }

    if (std::holds_alternative<EnterCommandMode>(command)) {
        context.appendDebugMessage("Switching to command mode");
        return ModeType::COMMAND;
    }

    context.appendDebugMessage("Unknown key");
    return std::nullopt;
}
