//
// Created by jan on 4/4/26.
//

#include "../include/TerminalManagement.h"
#include <termios.h>
#include <sys/ioctl.h>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <format>
#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unistd.h>
#include <vector>

namespace terminal_manager {
    termios original_values;

    namespace {
        /**
         * @brief Attempts to read one byte from stdin.
         * @param output Destination character.
         * @return True when a byte was read.
         */
        bool readByte(char& output) {
            const auto bytes_read = read(STDIN_FILENO, &output, 1);
            if (bytes_read == 1) {
                return true;
            }

            if (bytes_read == -1 && errno != EAGAIN) {
                die("read");
            }

            return false;
        }

        /**
         * @brief Maps VT modifier code to KeyModifiers.
         * @param code Numeric modifier code from escape sequences.
         * @return Parsed modifier flags.
         */
        editor_input::KeyModifiers modifiersFromCode(const int code) {
            using editor_input::KeyModifiers;
            switch (code) {
                case 2: return KeyModifiers{.shift = true};
                case 3: return KeyModifiers{.alt = true};
                case 4: return KeyModifiers{.shift = true, .alt = true};
                case 5: return KeyModifiers{.ctrl = true};
                case 6: return KeyModifiers{.shift = true, .ctrl = true};
                case 7: return KeyModifiers{.alt = true, .ctrl = true};
                case 8: return KeyModifiers{.shift = true, .alt = true, .ctrl = true};
                default: return KeyModifiers{};
            }
        }

        /**
         * @brief Parses a non-negative integer token.
         * @param token Token to parse.
         * @return Parsed integer value or std::nullopt on failure.
         */
        std::optional<int> parsePositiveInt(const std::string_view token) {
            if (token.empty()) {
                return std::nullopt;
            }

            int value = 0;
            const auto* begin = token.data();
            const auto* end = token.data() + token.size();
            const auto [ptr, error] = std::from_chars(begin, end, value);
            if (error != std::errc() || ptr != end || value < 0) {
                return std::nullopt;
            }

            return value;
        }

        /**
         * @brief Parses semicolon-separated CSI parameters.
         * @param params Raw parameter string.
         * @return Parsed parameter list.
         */
        std::vector<int> parseCsiParams(const std::string_view params) {
            std::vector<int> parsed;
            std::size_t cursor = 0;

            while (cursor < params.length()) {
                const auto next = params.find(';', cursor);
                const auto end = (next == std::string_view::npos) ? params.length() : next;
                const auto token = params.substr(cursor, end - cursor);
                const auto value = parsePositiveInt(token);
                if (value.has_value()) {
                    parsed.emplace_back(*value);
                }
                else {
                    parsed.emplace_back(0); // Default value for empty or invalid tokens
                }

                if (next == std::string_view::npos) {
                    break;
                }
                cursor = next + 1;
            }

            return parsed;
        }

        /**
         * @brief Converts VT function-key code to function key number.
         * @param key_code VT key code.
         * @return Function key number or 0 when unknown.
         */
        int functionKeyFromVtCode(const int key_code) {
            switch (key_code) {
                case 11 ... 15:
                    return key_code - 10;
                case 17 ... 21:
                    return key_code - 11;
                case 23 ... 26:
                    return key_code - 12;
                case 28 ... 29:
                    return key_code - 13;
                case 31 ... 34:
                    return key_code - 14;
                default: return 0;
            }
        }

        /**
         * @brief Converts a single character into a parsed key event.
         * @param c Input character byte.
         * @param modifiers Modifier state to attach.
         * @return Parsed key variant.
         */
        editor_input::Key asCharacterKey(const char c, editor_input::KeyModifiers modifiers = {}) {
            const auto raw = static_cast<unsigned char>(c);

            if (c == 127 || c == 8) {
                return editor_input::Backspace{modifiers};
            }

            if (c == '\r' || c == '\n') {
                return editor_input::Enter{modifiers};
            }

            if (c == '\t') {
                return editor_input::Tab{modifiers};
            }

            if (raw >= 1 && raw < 26) {
                modifiers.ctrl = true;
                return editor_input::Character{static_cast<char>('a' + raw - 1), modifiers};
            }

            if (raw == 0) {
                return editor_input::Character{' ', modifiers};
            }

            return editor_input::Character{c, modifiers};
        }
    }

    /**
     * @brief Parses a CSI escape sequence into a key event.
     * @param sequence Escape payload beginning with '['.
     * @return Parsed key variant.
     */
    editor_input::Key parseCsiSequence(const std::string& sequence) {
        if (sequence.size() < 2 || sequence.front() != '[') {
            return editor_input::RawEscapeSequence{std::string(sequence)};
        }

        const char final = sequence.back();
        const std::string_view params_view{sequence.data() + 1, sequence.size() - 2};
        const auto params = parseCsiParams(params_view);

        const int modifier_code = params.size() >= 2 ? params[1] : 1;
        const auto modifiers = modifiersFromCode(modifier_code);

        switch (final) {
            case 'A': return editor_input::ArrowUp{modifiers};
            case 'B': return editor_input::ArrowDown{modifiers};
            case 'C': return editor_input::ArrowRight{modifiers};
            case 'D': return editor_input::ArrowLeft{modifiers};
            case 'F': return editor_input::End{modifiers};
            case 'H': return editor_input::Home{modifiers};
            case 'P': return editor_input::FunctionKey{1, modifiers};
            case 'Q': return editor_input::FunctionKey{2, modifiers};
            case 'R': return editor_input::FunctionKey{3, modifiers};
            case 'S': return editor_input::FunctionKey{4, modifiers};
            case 'Z': return editor_input::Tab{editor_input::KeyModifiers{.shift = true}};
            case '~': {
                const int key_code = params.empty() ? 0 : params.front();
                switch (key_code) {
                    case 1:
                    case 7:
                        return editor_input::Home{modifiers};
                    case 3:
                        return editor_input::DeleteKey{modifiers};
                    case 4:
                        return editor_input::End{modifiers};
                    case 5:
                        return editor_input::PageUp{modifiers};
                    case 6:
                        return editor_input::PageDown{modifiers};
                    case 8:
                        return editor_input::End{modifiers};
                    default:
                        break;
                }

                const int fn = functionKeyFromVtCode(key_code);
                if (fn > 0) {
                    return editor_input::FunctionKey{fn, modifiers};
                }
                break;
            }
            default:
                break;
        }

        return editor_input::RawEscapeSequence{sequence};
    }

    /**
     * @brief Parses an SS3 escape sequence into a key event.
     * @param sequence Escape payload beginning with 'O'.
     * @return Parsed key variant.
     */
    editor_input::Key parseSS3Sequence(const std::string& sequence) {
        if (sequence.size() < 2 || sequence.front() != 'O') {
            return editor_input::RawEscapeSequence{std::string(sequence)};
        }

        const char final = sequence.back();
        const std::string_view params_view{sequence.data() + 1, sequence.size() - 2};
        const auto params = parseCsiParams(params_view);
        const int modifier_code = params.size() >= 2 ? params[1] : 1;
        const auto modifiers = modifiersFromCode(modifier_code);

        switch (final) {
            case 'P': return editor_input::FunctionKey{1, modifiers};
            case 'Q': return editor_input::FunctionKey{2, modifiers};
            case 'R': return editor_input::FunctionKey{3, modifiers};
            case 'S': return editor_input::FunctionKey{4, modifiers};
            case 'F': return editor_input::End{modifiers};
            case 'H': return editor_input::Home{modifiers};
            default: break;
        }

        return editor_input::RawEscapeSequence{sequence};
    }

    /**
     * @brief Builds a human-readable modifier prefix string.
     * @param modifiers Modifier flags.
     * @return Prefix such as "Ctrl+Alt+".
     */
    std::string modifierPrefix(const editor_input::KeyModifiers& modifiers) {
        std::string prefix;
        if (modifiers.ctrl) prefix += "Ctrl+";
        if (modifiers.alt) prefix += "Alt+";
        if (modifiers.shift) prefix += "Shift+";
        return prefix;
    }

    template<typename T>
    constexpr bool hasNameField = requires {
        { T::name } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    constexpr bool hasModifiersField = requires(T t) {
        { t.modifiers } -> std::same_as<editor_input::KeyModifiers&>;
    };

    template<typename T>
    /**
     * @brief Formats named key variants with their modifier prefix.
     * @param key Parsed key payload.
     * @return Human-readable key representation.
     */
    std::string formatNamedKey(const T& key) {
        if constexpr (hasNameField<T> && hasModifiersField<T>) {
            return std::format("{}{}", modifierPrefix(key.modifiers), T::name);
        }

        return "Unknown";
    }

    /** @brief Clears terminal contents and moves cursor to home position. */
    void clear_screen() {
        std::cout << terminal_control_sequences::clear_screen; // Clears the screen
        std::cout << terminal_control_sequences::cursor_start; // Moves the cursor to the top-left corner
    }

    /**
     * @brief Prints an error and terminates the process.
     * @param message Error context for perror.
     */
    void die(const std::string_view message) {
        clear_screen();
        perror(message.data());
        std::exit(1);
    }

    /** @brief Restores saved terminal settings from startup. */
    void disableRawMode() {
        // Set current terminal attributes
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_values) == -1) {
            die("tcsetattr");
        }
    }

    /** @brief Enables raw mode for byte-wise keyboard input. */
    void enableRawMode() {
        // Source: https://www.man7.org/linux/man-pages/man3/termios.3.html

        // Get current terminal attributes
        if (tcgetattr(STDIN_FILENO, &original_values) == -1) {
            die("tcgetattr");
        }

        // Ensure disableRawMode is called upon exit
        atexit(disableRawMode);

        struct termios raw_values = original_values;

        // Input flags:
        // BRKINT: disables break signal
        // ICRNL: disables CR (carriage) to NL (new line) translation
        // ISTRIP: disables stripping of the 8th bit
        // IXON: disables software flow control (Ctrl-S and Ctrl-Q)
        raw_values.c_iflag &= ~(BRKINT | ICRNL | ISTRIP | IXON);

        // Output flags:
        // OPOST: disables post-processing
        raw_values.c_oflag &= ~(OPOST);

        // Local flags:
        // ECHO: disable echo
        // ICANON: canonical mode
        // IEXTEN: Special keys
        // ISIG: Signals
        raw_values.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

        // Control flags:
        // set 8bit characters
        raw_values.c_cflag |= (CS8);

        // Control characters: set read timeout
        raw_values.c_cc[VMIN] = 0; // Return immediately
        raw_values.c_cc[VTIME] = 1; // 100ms timeout for input

        // Apply modified attributes
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_values) == -1) {
            die("tcsetattr");
        }
    }

    /**
     * @brief Reads terminal dimensions into the provided output struct.
     * @param size Receives width and height in characters.
     * @return 0 on success, -1 on failure.
     */
    int getWindowSize(int2d& size) {
        struct winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
            return -1;
        }
        size.x = ws.ws_col;
        size.y = ws.ws_row;
        return 0;
    }

    /**
     * @brief Reads one key event from stdin and parses escape sequences.
     * @return Parsed key event.
     */
    editor_input::Key readKey() {
        long bytes_read;
        char c;

        while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
            if (bytes_read == -1 && errno != EAGAIN) {
                die("read");
            }
        }

        // Source:
        // https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
        if (c == '\x1b') {
            // Escape sequence - reading special characters
            char seq_start = 0;

            if (!readByte(seq_start)) {
                return editor_input::Escape{};
            }

            if (seq_start != '[' && seq_start != 'O') {
                editor_input::KeyModifiers alt_modifiers;
                alt_modifiers.alt = true;
                return asCharacterKey(seq_start, alt_modifiers);
            }

            std::string sequence;
            sequence.push_back(seq_start);

            for (int i = 0; i < 30; ++i) {
                char part = 0;
                if (!readByte(part)) {
                    return editor_input::RawEscapeSequence{sequence};
                }

                sequence.push_back(part);
                if (part >= '@' && part <= '~') {
                    break;
                }
            }

            if (seq_start == '[') {
                return parseCsiSequence(sequence);
            }
            return parseSS3Sequence(sequence);
        }

        // Regular character (including newline, ctrl chars, etc.)
        return asCharacterKey(c);
    }

    /**
     * @brief Converts a key event to a debug-friendly string.
     * @param key Parsed key event.
     * @return Human-readable key representation.
     */
    std::string keyToDebugString(const editor_input::Key& key) {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                return "None";
            }
            else if constexpr (std::is_same_v<T, editor_input::Character>) {
                const auto prefix = modifierPrefix(value.modifiers);
                const auto raw = static_cast<unsigned char>(value.value);
                switch (value.value) {
                    case '\n':
                        return std::format("{}Character(\\n)", prefix);
                    case '\r':
                        return std::format("{}Character(\\r)", prefix);
                    case '\t':
                        return std::format("{}Character(\\t)", prefix);
                    default:
                        break;
                }
                if (raw < 32 || raw > 127) {
                    return std::format("{}Character(ctrl:{})", prefix, static_cast<int>(raw));
                }
                return std::format("{}Character({})", prefix, value.value);
            } else if constexpr (std::is_same_v<T, editor_input::FunctionKey>) {
                return std::format("{}F{}", modifierPrefix(value.modifiers), value.number);
            } else if constexpr (hasNameField<T> && hasModifiersField<T>) {
                return formatNamedKey(value);
            } else if constexpr (std::is_same_v<T, editor_input::RawEscapeSequence>) {
                return std::format("RawEscapeSequence({})", value.bytes);
            }
            return "Unknown";
        }, key);
    }
}
