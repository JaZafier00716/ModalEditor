//
// Created by jan on 4/4/26.
//

#include "../include/EditorMode.h"

#include <cctype>
#include <fstream>
#include <format>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace {
    struct MotionCommand {
        editor_motion::Motion motion{};
        bool linewise{false};
        bool inclusive{false};
        bool uses_count{true};
    };

    struct MotionBinding {
        editor_motion::Motion motion{};
        bool linewise{false};
        bool linewise_with_operator{false};
        bool inclusive{false};
        bool uses_count{true};
    };

    struct MotionBindings {
        std::unordered_map<std::string, MotionBinding> sequence_bindings;
        std::unordered_map<std::string, MotionBinding> special_bindings;
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

    MotionCommand resolveMotionBinding(const MotionBinding& binding, const bool operator_pending) {
        return MotionCommand{
            binding.motion,
            binding.linewise || (binding.linewise_with_operator && operator_pending),
            binding.inclusive,
            binding.uses_count
        };
    }

    std::string trim(std::string_view value) {
        constexpr std::string_view whitespace = " \t\r\n";
        const auto start = value.find_first_not_of(whitespace);
        if (start == std::string_view::npos) {
            return "";
        }

        const auto end = value.find_last_not_of(whitespace);
        return std::string{value.substr(start, end - start + 1)};
    }

    bool isSpecialKeyName(const std::string_view key) {
        return key == "ArrowUp"
            || key == "ArrowDown"
            || key == "ArrowLeft"
            || key == "ArrowRight"
            || key == "PageUp"
            || key == "PageDown"
            || key == "Home"
            || key == "End";
    }

    std::optional<MotionBinding> motionBindingFromName(const std::string_view name) {
        if (name == "left") return MotionBinding{editor_motion::Left{}, false, false, false, true};
        if (name == "right") return MotionBinding{editor_motion::Right{}, false, false, false, true};
        if (name == "up") return MotionBinding{editor_motion::Up{}, false, true, false, true};
        if (name == "down") return MotionBinding{editor_motion::Down{}, false, true, false, true};
        if (name == "page_up") return MotionBinding{editor_motion::PageUp{}, false, true, false, true};
        if (name == "page_down") return MotionBinding{editor_motion::PageDown{}, false, true, false, true};
        if (name == "line_start") return MotionBinding{editor_motion::LineStart{}, false, false, false, false};
        if (name == "line_end") return MotionBinding{editor_motion::LineEnd{}, false, false, false, false};
        if (name == "file_start") return MotionBinding{editor_motion::FileStart{}, true, false, false, false};
        if (name == "file_end") return MotionBinding{editor_motion::FileEnd{}, true, false, false, false};
        if (name == "word_forward") return MotionBinding{editor_motion::WordForward{.big = false}, false, false, false, true};
        if (name == "word_forward_big") return MotionBinding{editor_motion::WordForward{.big = true}, false, false, false, true};
        if (name == "word_backward") return MotionBinding{editor_motion::WordBackward{.big = false}, false, false, false, true};
        if (name == "word_backward_big") return MotionBinding{editor_motion::WordBackward{.big = true}, false, false, false, true};
        if (name == "word_end") return MotionBinding{editor_motion::WordEnd{.big = false}, false, false, true, true};
        if (name == "word_end_big") return MotionBinding{editor_motion::WordEnd{.big = true}, false, false, true, true};
        if (name == "screen_top") return MotionBinding{editor_motion::ScreenTop{}, false, false, false, false};
        if (name == "screen_middle") return MotionBinding{editor_motion::ScreenMiddle{}, false, false, false, false};
        if (name == "screen_bottom") return MotionBinding{editor_motion::ScreenBottom{}, false, false, false, false};
        if (name == "paragraph_forward") return MotionBinding{editor_motion::ParagraphForward{}, true, false, false, true};
        if (name == "paragraph_backward") return MotionBinding{editor_motion::ParagraphBackward{}, true, false, false, true};
        if (name == "matching_bracket") return MotionBinding{editor_motion::MatchingBracket{}, false, false, true, false};
        if (name == "matching_brace") return MotionBinding{editor_motion::MatchingBrace{}, false, false, true, false};
        return std::nullopt;
    }

    MotionBindings buildDefaultMotionBindings() {
        MotionBindings bindings;
        const auto add_binding = [&](const std::string_view trigger, const std::string_view motion_name) {
            const auto binding = motionBindingFromName(motion_name);
            if (!binding.has_value()) {
                return;
            }

            if (isSpecialKeyName(trigger)) {
                bindings.special_bindings[std::string{trigger}] = *binding;
            }
            else {
                bindings.sequence_bindings[std::string{trigger}] = *binding;
            }
        };

        add_binding("h", "left");
        add_binding("j", "down");
        add_binding("k", "up");
        add_binding("l", "right");
        add_binding("0", "line_start");
        add_binding("$", "line_end");
        add_binding("gg", "file_start");
        add_binding("G", "file_end");
        add_binding("w", "word_forward");
        add_binding("W", "word_forward_big");
        add_binding("b", "word_backward");
        add_binding("B", "word_backward_big");
        add_binding("e", "word_end");
        add_binding("E", "word_end_big");
        add_binding("H", "screen_top");
        add_binding("M", "screen_middle");
        add_binding("L", "screen_bottom");
        add_binding("{", "paragraph_backward");
        add_binding("}", "paragraph_forward");
        add_binding("%", "matching_bracket");
        add_binding("&", "matching_brace");
        add_binding("ArrowUp", "up");
        add_binding("ArrowDown", "down");
        add_binding("ArrowLeft", "left");
        add_binding("ArrowRight", "right");
        add_binding("PageUp", "page_up");
        add_binding("PageDown", "page_down");
        add_binding("Home", "line_start");
        add_binding("End", "line_end");

        return bindings;
    }

    MotionBindings active_motion_bindings = buildDefaultMotionBindings();

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

    std::optional<std::string_view> specialKeyName(const editor_input::Key& key) {
        return std::visit([](const auto& value) -> std::optional<std::string_view> {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, editor_input::ArrowUp>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "ArrowUp";
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowDown>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "ArrowDown";
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowLeft>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "ArrowLeft";
            }
            else if constexpr (std::is_same_v<T, editor_input::ArrowRight>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "ArrowRight";
            }
            else if constexpr (std::is_same_v<T, editor_input::PageUp>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "PageUp";
            }
            else if constexpr (std::is_same_v<T, editor_input::PageDown>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "PageDown";
            }
            else if constexpr (std::is_same_v<T, editor_input::Home>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "Home";
            }
            else if constexpr (std::is_same_v<T, editor_input::End>) {
                if (!hasNoModifiers(value)) {
                    return std::nullopt;
                }
                return "End";
            }
            else {
                return std::nullopt;
            }
        }, key);
    }

    /**
     * @brief Maps non-character keys to motion commands.
     * @param key Parsed key event.
     * @param operator_pending True when resolving an operator target.
     * @return Motion command, or std::nullopt if the key is not a motion.
     */
    std::optional<MotionCommand> motionFromSpecialKey(const editor_input::Key& key, const bool operator_pending) {
        const auto key_name = specialKeyName(key);
        if (!key_name.has_value()) {
            return std::nullopt;
        }

        const auto binding = active_motion_bindings.special_bindings.find(std::string{*key_name});
        if (binding == active_motion_bindings.special_bindings.end()) {
            return std::nullopt;
        }

        return resolveMotionBinding(binding->second, operator_pending);
    }

    /**
     * @brief Maps configured character sequences to motion commands.
     * @param sequence Character sequence typed so far.
     * @param operator_pending True when resolving an operator target.
     * @return Motion command, or std::nullopt if the key is not a motion.
     */
    std::optional<MotionCommand> motionFromSequence(const std::string_view sequence, const bool operator_pending) {
        const auto binding = active_motion_bindings.sequence_bindings.find(std::string{sequence});
        if (binding == active_motion_bindings.sequence_bindings.end()) {
            return std::nullopt;
        }

        return resolveMotionBinding(binding->second, operator_pending);
    }

    bool hasSequencePrefix(const std::string_view sequence) {
        for (const auto& [configured_sequence, binding] : active_motion_bindings.sequence_bindings) {
            (void)binding;
            if (configured_sequence.size() > sequence.size() && configured_sequence.starts_with(sequence)) {
                return true;
            }
        }

        return false;
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

bool loadNormalModeConfig(const std::string_view path, std::string& error_message) {
    std::ifstream input{std::string{path}};
    if (!input.is_open()) {
        active_motion_bindings = buildDefaultMotionBindings();
        error_message = std::format("could not open motion config '{}'; using built-in defaults", path);
        return false;
    }

    MotionBindings loaded_bindings;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const auto trimmed_line = trim(line);
        if (trimmed_line.empty() || trimmed_line.starts_with('#')) {
            continue;
        }

        const auto separator = trimmed_line.find('=');
        if (separator == std::string::npos) {
            active_motion_bindings = buildDefaultMotionBindings();
            error_message = std::format("invalid motion config at {}:{}; expected <trigger> = <motion>", path, line_number);
            return false;
        }

        const auto trigger = trim(std::string_view{trimmed_line}.substr(0, separator));
        const auto motion_name = trim(std::string_view{trimmed_line}.substr(separator + 1));
        if (trigger.empty() || motion_name.empty()) {
            active_motion_bindings = buildDefaultMotionBindings();
            error_message = std::format("invalid motion config at {}:{}; trigger and motion must be non-empty", path, line_number);
            return false;
        }

        const auto binding = motionBindingFromName(motion_name);
        if (!binding.has_value()) {
            active_motion_bindings = buildDefaultMotionBindings();
            error_message = std::format("unknown motion '{}' at {}:{}", motion_name, path, line_number);
            return false;
        }

        if (isSpecialKeyName(trigger)) {
            loaded_bindings.special_bindings[trigger] = *binding;
            continue;
        }

        if (trigger.find_first_of(" \t") != std::string::npos) {
            active_motion_bindings = buildDefaultMotionBindings();
            error_message = std::format("invalid trigger '{}' at {}:{}; character sequences cannot contain spaces",
                                        trigger, path, line_number);
            return false;
        }

        loaded_bindings.sequence_bindings[trigger] = *binding;
    }

    active_motion_bindings = std::move(loaded_bindings);
    error_message.clear();
    return true;
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
        pending_sequence.clear();
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
        if (command_count != 0 || motion_count != 0 || !pending_sequence.empty() || pending_operator.has_value()) {
            context.appendDebugMessage("Cancelled pending command");
            resetPendingState();
        }
        return std::nullopt;
    }

    if (!pending_sequence.empty() && !std::holds_alternative<editor_input::Character>(key)) {
        pending_sequence.clear();
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

    if (!pending_sequence.empty()) {
        pending_sequence.push_back(*c);
        if (const auto motion = motionFromSequence(pending_sequence, pending_operator.has_value()); motion.has_value()) {
            if (pending_operator.has_value()) {
                finishOperator(*motion);
            }
            else {
                finishMotion(*motion);
            }
            return std::nullopt;
        }

        pending_sequence.clear();
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
            pending_sequence.clear();
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

    switch (*c) {
        case 'i': return handleInsertCommand(EnterInsert{});
        case 'I': return handleInsertCommand(EnterInsertAtLineStart{});
        case 'A': return handleInsertCommand(EnterInsertAtLineEnd{});
        case 'a': return handleInsertCommand(EnterInsertAfterCursor{});
        case ':': return handleInsertCommand(EnterCommandMode{});
        default:
            break;
    }

    const std::string single_key{*c};
    if (const auto motion = motionFromSequence(single_key, pending_operator.has_value()); motion.has_value()) {
        if (pending_operator.has_value()) {
            finishOperator(*motion);
        }
        else {
            finishMotion(*motion);
        }
        return std::nullopt;
    }

    if (hasSequencePrefix(single_key)) {
        pending_sequence = single_key;
        return std::nullopt;
    }

    context.appendDebugMessage("Unknown key");
    resetPendingState();
    return std::nullopt;
}
