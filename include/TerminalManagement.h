//
// Created by jan on 4/4/26.
//

#ifndef MODALEDITOR_TERMINALMANAGEMENT_H
#define MODALEDITOR_TERMINALMANAGEMENT_H
#include <string>
#include <string_view>
#include <variant>

/** @brief Simple integer 2D coordinate or size pair. */
struct int2d {
    int x{0};
    int y{0};
};

/** @brief Terminal key event model used by the editor input layer. */
namespace editor_input {
    /** @brief Modifier key state associated with an input event. */
    struct KeyModifiers {
        bool shift{false};
        bool alt{false};
        bool ctrl{false};
    };

    /** @brief Printable character input event. */
    struct Character {
        char value{};
        KeyModifiers modifiers{};
    };
    /** @brief Escape key input event. */
    struct Escape {
        static constexpr std::string_view name = "Escape";
        KeyModifiers modifiers{};
    };
    /** @brief Enter/Return key input event. */
    struct Enter {
        static constexpr std::string_view name = "Enter";
        KeyModifiers modifiers{};
    };
    /** @brief Tab key input event. */
    struct Tab {
        static constexpr std::string_view name = "Tab";
        KeyModifiers modifiers{};
    };
    /** @brief Backspace key input event. */
    struct Backspace {
        static constexpr std::string_view name = "Backspace";
        KeyModifiers modifiers{};
    };
    /** @brief Left arrow key input event. */
    struct ArrowLeft {
        static constexpr std::string_view name = "ArrowLeft";
        KeyModifiers modifiers{};
    };
    /** @brief Right arrow key input event. */
    struct ArrowRight {
        static constexpr std::string_view name = "ArrowRight";
        KeyModifiers modifiers{};
    };
    /** @brief Up arrow key input event. */
    struct ArrowUp {
        static constexpr std::string_view name = "ArrowUp";
        KeyModifiers modifiers{};
    };
    /** @brief Down arrow key input event. */
    struct ArrowDown {
        static constexpr std::string_view name = "ArrowDown";
        KeyModifiers modifiers{};
    };
    /** @brief Delete key input event. */
    struct DeleteKey {
        static constexpr std::string_view name = "Delete";
        KeyModifiers modifiers{};
    };
    /** @brief PageUp key input event. */
    struct PageUp {
        static constexpr std::string_view name = "PageUp";
        KeyModifiers modifiers{};
    };
    /** @brief PageDown key input event. */
    struct PageDown {
        static constexpr std::string_view name = "PageDown";
        KeyModifiers modifiers{};
    };
    /** @brief Home key input event. */
    struct Home {
        static constexpr std::string_view name = "Home";
        KeyModifiers modifiers{};
    };
    /** @brief End key input event. */
    struct End {
        static constexpr std::string_view name = "End";
        KeyModifiers modifiers{};
    };
    /** @brief Function key input event (F1..Fn). */
    struct FunctionKey {
        int number{0};
        KeyModifiers modifiers{};
    };


    // Keeps unparsed escape payload for inspection of unknown keys.
    struct RawEscapeSequence { std::string bytes{}; };

    /** @brief Variant containing every key event supported by the parser. */
    using Key = std::variant<
        std::monostate,
        Character,
        Escape,
        Enter,
        Tab,
        Backspace,
        ArrowLeft,
        ArrowRight,
        ArrowUp,
        ArrowDown,
        DeleteKey,
        PageUp,
        PageDown,
        Home,
        End,
        FunctionKey,
        RawEscapeSequence
    >;
}

/** @brief Cursor and viewport motions used by normal mode commands. */
namespace editor_motion {
    struct Left {};
    struct Right {};
    struct Up {};
    struct Down {};

    struct PageUp{};
    struct PageDown{};

    struct LineStart{};
    struct LineEnd{};

    struct FileStart{};
    struct FileEnd{};

    struct WordForward{};
    struct WordBackward{};

    struct ScreenTop{};
    struct ScreenMiddle{};
    struct ScreenBottom{};

    struct ParagraphForward{};
    struct ParagraphBackward{};

    struct MatchingBracket{};
    struct MatchingBrace{};


    struct JumpToLine {
        int line_number;
    };

    /** @brief Variant of all supported cursor motions. */
    using Motion = std::variant<
        std::monostate,
        Left, Right, Up, Down,
        PageUp, PageDown,
        LineStart, LineEnd,
        FileStart, FileEnd,
        WordForward, WordBackward,
        ScreenTop, ScreenMiddle, ScreenBottom,
        ParagraphForward, ParagraphBackward,
        MatchingBracket, MatchingBrace,
        JumpToLine
    >;
}

/** @brief Legacy key codes used by older input paths. */
enum class EditorKey
{
    BACKSPACE_ALT = 8,
    ESCAPE_KEY = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 300, // Some number higher than max ascii
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
};

/** @brief Terminal escape sequences used for rendering and cursor control. */
namespace terminal_control_sequences {
    // colors
    constexpr std::string_view reset_color = "\033[0m";
    constexpr std::string_view dimmed_color = "\033[1;35m";
    constexpr std::string_view invert_colors = "\x1b[7m";
    constexpr std::string_view reset_invert_colors = "\x1b[m";

    //control sequences
    constexpr std::string_view clear_screen = "\x1B[2J";
    constexpr std::string_view cursor_start = "\x1B[H";
    constexpr std::string_view hide_cursor = "\x1B[?25l";
    constexpr std::string_view show_cursor = "\x1B[?25h";
    constexpr std::string_view clear_to_eol = "\x1B[K";
    constexpr std::string_view new_line = "\r\n";
}

/** @brief Terminal management and input parsing utilities. */
namespace terminal_manager {
    /** @brief Prints an error and exits after restoring terminal state. */
    void die(std::string_view message);
    /** @brief Restores canonical terminal mode. */
    void disableRawMode();
    /** @brief Enables raw terminal mode for interactive key-by-key input. */
    void enableRawMode();
    /** @brief Queries terminal size in characters. */
    int getWindowSize(int2d& size);
    /** @brief Reads and parses a key event from stdin. */
    editor_input::Key readKey();
    /** @brief Converts a key event into a human-readable debug string. */
    std::string keyToDebugString(const editor_input::Key& key);

    /** @brief Clears the terminal and moves the cursor to the top-left corner. */
    void clear_screen();
};


#endif //MODALEDITOR_TERMINALMANAGEMENT_H