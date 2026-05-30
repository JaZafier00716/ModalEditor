//
// Created by jan on 4/16/26.
//
#include <algorithm>
#include <cctype>
#include <optional>
#include <tuple>
#include <type_traits>
#include "../include/CursorController.h"

namespace {
    struct TextPos {
        int y{0};
        int x{0};
    };

    enum class WordStyle {
        Small,
        Big
    };

    enum class WordClass {
        Whitespace,
        Keyword,
        Punctuation
    };

    bool isWordChar(const char ch) {
        const auto uch = static_cast<unsigned char>(ch);
        return std::isalnum(uch) != 0 || ch == '_';
    }

    bool isLineBlank(const std::string_view line) {
        return std::ranges::all_of(line, [](const char ch) {
            return std::isspace(static_cast<unsigned char>(ch)) != 0;
        });
    }

    std::optional<char> charAt(const TextDocument& document, const TextPos pos) {
        const auto line = document.lineRawText(pos.y);
        if (pos.x < 0 || pos.x >= static_cast<int>(line.size())) {
            return std::nullopt;
        }
        return line[static_cast<size_t>(pos.x)];
    }

    bool advancePosition(const TextDocument& document, TextPos& pos) {
        const auto row_count = static_cast<int>(document.lineCount());
        if (row_count <= 0 || pos.y < 0 || pos.y >= row_count) {
            return false;
        }

        const auto line_length = static_cast<int>(document.lineRawLength(pos.y));
        if (pos.x < line_length) {
            ++pos.x;
            return true;
        }

        if (pos.y < row_count - 1) {
            ++pos.y;
            pos.x = 0;
            return true;
        }

        return false;
    }

    bool retreatPosition(const TextDocument& document, TextPos& pos) {
        if (pos.x > 0) {
            --pos.x;
            return true;
        }

        if (pos.y > 0) {
            --pos.y;
            pos.x = static_cast<int>(document.lineRawLength(pos.y));
            return true;
        }

        return false;
    }

    std::optional<WordClass> wordClassAt(const TextDocument& document, const TextPos pos, const WordStyle style) {
        if (const auto ch = charAt(document, pos); ch.has_value()) {
            if (std::isspace(static_cast<unsigned char>(*ch)) != 0) {
                return WordClass::Whitespace;
            }

            if (style == WordStyle::Big) {
                return WordClass::Keyword;
            }

            return isWordChar(*ch) ? WordClass::Keyword : WordClass::Punctuation;
        }

        const auto row_count = static_cast<int>(document.lineCount());
        if (row_count > 0 && pos.y >= 0 && pos.y < row_count - 1 &&
            pos.x == static_cast<int>(document.lineRawLength(pos.y))) {
            return WordClass::Whitespace;
        }

        return std::nullopt;
    }

    bool findMatchingDelimiter(const TextDocument& document, const TextPos start, const char open, const char close,
                               const int direction, TextPos& matched) {
        TextPos pos = start;
        int depth = 1;

        while (direction > 0 ? advancePosition(document, pos) : retreatPosition(document, pos)) {
            const auto ch = charAt(document, pos);
            if (!ch.has_value()) {
                continue;
            }

            if (*ch == open) {
                depth += direction > 0 ? 1 : -1;
            }
            else if (*ch == close) {
                depth += direction > 0 ? -1 : 1;
            }

            if (depth == 0) {
                matched = pos;
                return true;
            }
        }

        return false;
    }
}

void CursorController::operator()(const auto& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, std::monostate>) {
        // No motion - do nothing
    }
    else if constexpr (std::is_same_v<T, editor_motion::Up>) {
        // Move UP
        moveUp();
    }
    else if constexpr (std::is_same_v<T, editor_motion::Down>) {
        // Move DOWN
        moveDown();
    }
    else if constexpr (std::is_same_v<T, editor_motion::Right>) {
        // Move RIGHT
        moveRight();
    }
    else if constexpr (std::is_same_v<T, editor_motion::Left>) {
        // Move LEFT
        moveLeft();
    }
    else if constexpr (std::is_same_v<T, editor_motion::PageUp>) {
        // Move PAGE UP
        movePageUp();
    }
    else if constexpr (std::is_same_v<T, editor_motion::PageDown>) {
        // Move PAGE DOWN
        movePageDown();
    }
    else if constexpr (std::is_same_v<T, editor_motion::LineStart>) {
        moveCursorLineStart();
    }
    else if constexpr (std::is_same_v<T, editor_motion::LineEnd>) {
        moveCursorLineEnd();
    }
    else if constexpr (std::is_same_v<T, editor_motion::FileStart>) {
        moveFileStart();
    }
    else if constexpr (std::is_same_v<T, editor_motion::FileEnd>) {
        moveFileEnd();
    }
    else if constexpr (std::is_same_v<T, editor_motion::WordForward>) {
        moveWordForward(value.big);
    }
    else if constexpr (std::is_same_v<T, editor_motion::WordBackward>) {
        moveWordBackward(value.big);
    }
    else if constexpr (std::is_same_v<T, editor_motion::WordEnd>) {
        moveWordEnd(value.big);
    }
    else if constexpr (std::is_same_v<T, editor_motion::ScreenTop>) {
        moveScreenTop();
    }
    else if constexpr (std::is_same_v<T, editor_motion::ScreenMiddle>) {
        moveScreenMiddle();
    }
    else if constexpr (std::is_same_v<T, editor_motion::ScreenBottom>) {
        moveScreenBottom();
    }
    else if constexpr (std::is_same_v<T, editor_motion::ParagraphForward>) {
        moveParagraphForward();
    }
    else if constexpr (std::is_same_v<T, editor_motion::ParagraphBackward>) {
        moveParagraphBackward();
    }
    else if constexpr (std::is_same_v<T, editor_motion::MatchingBracket>) {
        moveToMatchingBracket();
    }
    else if constexpr (std::is_same_v<T, editor_motion::MatchingBrace>) {
        moveToMatchingBrace();
    }
    else {
        static_assert(unsupported_type<T>, "Missing branch for type");
    }
}


/**
 * @brief Applies a motion variant and updates cursor state accordingly.
 * @param motion Motion command to execute.
 */
void CursorController::applyMotion(const editor_motion::Motion& motion) {
    std::visit(*this, motion);

    clampCursorPosition();
}

/** @brief Moves the cursor to column zero on the current row. */
void CursorController::moveCursorLineStart() {
    setCursorPositionX(0);
}

/**
 * @brief Sets the cursor column while clamping to the current row length.
 * @param x Requested raw column.
 */
void CursorController::setCursorPositionX(const int x) {
    if (cursor_pos.y >= 0 && cursor_pos.y < static_cast<int>(document.lineCount())) {
        cursor_pos.x = std::min(x, getRowLength(cursor_pos.y));
    }
    else {
        cursor_pos.x = x;
    }
}

void CursorController::setCursorPosition(const int2d& position) {
    cursor_pos = position;
    desired_cursor_pos = cursor_pos.x;
    clampCursorPosition();
}

/** @brief Moves the cursor to the end of the current row. */
void CursorController::moveCursorLineEnd() {
    if (cursor_pos.y >= 0 && cursor_pos.y < static_cast<int>(document.lineCount())) {
        cursor_pos.x = getRowLength(cursor_pos.y);
    }
}

/** @brief Advances the cursor by one raw column. */
void CursorController::moveCursorRightOne() {
    setCursorPositionX(cursor_pos.x + 1);
}

/**
 * @brief Returns the raw row length for the provided line index.
 * @param y Row index.
 * @return Raw character count for that row.
 */
int CursorController::getRowLength(const int y) const {
    return static_cast<int>(document.lineRawLength(y));
}

/** @brief Clamps cursor coordinates to valid document bounds. */
void CursorController::clampCursorPosition() {
    const auto row_count = static_cast<int>(document.lineCount());
    const auto max_y = row_count == 0 ? 0 : row_count - 1;
    cursor_pos.y = std::clamp(cursor_pos.y, 0, max_y);

    const auto row_length = getRowLength(cursor_pos.y);
    cursor_pos.x = std::clamp(cursor_pos.x, 0, row_length);
}

void CursorController::moveUp() {
    if (cursor_pos.y > 0) {
        cursor_pos.y--;
        cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
    }
}

void CursorController::moveDown() {
    const auto row_count = static_cast<int>(document.lineCount());

    if (row_count > 0 && cursor_pos.y < row_count - 1) {
        cursor_pos.y++;
        cursor_pos.x = desired_cursor_pos; // Try to maintain horizontal position - otherwise clamp will fix :)
    }
}

void CursorController::moveLeft() {
    if (cursor_pos.x > 0) {
        cursor_pos.x--;
    }
    else if (cursor_pos.y > 0) {
        cursor_pos.y--;
        cursor_pos.x = getRowLength(cursor_pos.y); // Move to end of prev line
    }
    desired_cursor_pos = cursor_pos.x; // Update desired position for vertical movement
}

void CursorController::moveRight() {
    const auto row_count = static_cast<int>(document.lineCount());

    if (cursor_pos.x < getRowLength(cursor_pos.y)) {
        cursor_pos.x++;
    }
    else if (cursor_pos.y < row_count - 1) {
        cursor_pos.y++;
        cursor_pos.x = 0; // Move to start of next line
    }
    desired_cursor_pos = cursor_pos.x; // Update desired position for vertical movement
}

void CursorController::movePageUp() {
    cursor_pos.y = std::max(cursor_pos.y - screen_size.y, 0);
}

void CursorController::movePageDown() {
    const auto row_count = static_cast<int>(document.lineCount());

    const auto max_y = row_count == 0 ? 0 : row_count - 1;
    cursor_pos.y = std::min(cursor_pos.y + screen_size.y, max_y);
}

void CursorController::moveFileStart() {
    cursor_pos.y = 0;
    cursor_pos.x = 0;
    desired_cursor_pos = 0;
}

void CursorController::moveFileEnd() {
    const auto row_count = static_cast<int>(document.lineCount());

    if (row_count > 0) {
        cursor_pos.y = row_count - 1;
        cursor_pos.x = getRowLength(cursor_pos.y);
        desired_cursor_pos = cursor_pos.x;
    }
}

void CursorController::moveWordForward(const bool big_word) {
    if (document.empty()) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};
    const auto style = big_word ? WordStyle::Big : WordStyle::Small;
    auto current_class = wordClassAt(document, pos, style);

    if (!current_class.has_value()) {
        return;
    }

    if (*current_class == WordClass::Whitespace) {
        while (current_class.has_value() && *current_class == WordClass::Whitespace) {
            if (!advancePosition(document, pos)) {
                break;
            }
            current_class = wordClassAt(document, pos, style);
        }
    }
    else {
        const auto token_class = *current_class;
        while (current_class.has_value() && *current_class == token_class) {
            if (!advancePosition(document, pos)) {
                break;
            }
            current_class = wordClassAt(document, pos, style);
        }

        while (current_class.has_value() && *current_class == WordClass::Whitespace) {
            if (!advancePosition(document, pos)) {
                break;
            }
            current_class = wordClassAt(document, pos, style);
        }
    }

    cursor_pos.y = pos.y;
    cursor_pos.x = pos.x;
    desired_cursor_pos = cursor_pos.x;
}

void CursorController::moveWordBackward(const bool big_word) {
    if (document.empty()) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};
    const auto style = big_word ? WordStyle::Big : WordStyle::Small;

    if (!retreatPosition(document, pos)) {
        return;
    }

    auto current_class = wordClassAt(document, pos, style);
    while (current_class.has_value() && *current_class == WordClass::Whitespace) {
        if (!retreatPosition(document, pos)) {
            break;
        }
        current_class = wordClassAt(document, pos, style);
    }

    if (!current_class.has_value() || *current_class == WordClass::Whitespace) {
        cursor_pos.y = pos.y;
        cursor_pos.x = pos.x;
        desired_cursor_pos = cursor_pos.x;
        return;
    }

    const auto token_class = *current_class;
    while (true) {
        TextPos previous = pos;
        if (!retreatPosition(document, previous)) {
            break;
        }

        const auto previous_class = wordClassAt(document, previous, style);
        if (!previous_class.has_value() || *previous_class != token_class) {
            break;
        }

        pos = previous;
    }

    cursor_pos.y = pos.y;
    cursor_pos.x = pos.x;
    desired_cursor_pos = cursor_pos.x;
}

void CursorController::moveWordEnd(const bool big_word) {
    if (document.empty()) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};
    const auto style = big_word ? WordStyle::Big : WordStyle::Small;
    auto current_class = wordClassAt(document, pos, style);

    if (!current_class.has_value()) {
        return;
    }

    while (current_class.has_value() && *current_class == WordClass::Whitespace) {
        if (!advancePosition(document, pos)) {
            cursor_pos.y = pos.y;
            cursor_pos.x = pos.x;
            desired_cursor_pos = cursor_pos.x;
            return;
        }
        current_class = wordClassAt(document, pos, style);
    }

    if (!current_class.has_value()) {
        return;
    }

    const auto token_class = *current_class;
    while (true) {
        TextPos next = pos;
        if (!advancePosition(document, next)) {
            break;
        }

        const auto next_class = wordClassAt(document, next, style);
        if (!next_class.has_value() || *next_class != token_class) {
            break;
        }

        pos = next;
    }

    cursor_pos.y = pos.y;
    cursor_pos.x = pos.x;
    desired_cursor_pos = cursor_pos.x;
}

void CursorController::moveScreenTop() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count > 0) {
        cursor_pos.y = std::max(0, cursor_pos.y - (screen_size.y / 2));
        cursor_pos.x = desired_cursor_pos;
    }
}

void CursorController::moveScreenMiddle() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count > 0) {
        const auto max_y = row_count - 1;
        const auto target = std::clamp(cursor_pos.y, screen_size.y / 2, std::max(screen_size.y / 2, max_y));
        cursor_pos.y = target;
        cursor_pos.x = desired_cursor_pos;
    }
}

void CursorController::moveScreenBottom() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count > 0) {
        const auto max_y = row_count - 1;
        cursor_pos.y = std::min(max_y, cursor_pos.y + (screen_size.y / 2));
        cursor_pos.x = desired_cursor_pos;
    }
}

void CursorController::moveParagraphForward() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count <= 0) {
        return;
    }

    int y = std::min(cursor_pos.y + 1, row_count - 1);
    while (y < row_count && !isLineBlank(document.lineRawText(y))) {
        ++y;
    }
    while (y < row_count && isLineBlank(document.lineRawText(y))) {
        ++y;
    }
    cursor_pos.y = std::min(y, row_count - 1);
    cursor_pos.x = 0;
    desired_cursor_pos = 0;
}

void CursorController::moveParagraphBackward() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count <= 0) {
        return;
    }

    int y = std::max(cursor_pos.y - 1, 0);
    while (y > 0 && isLineBlank(document.lineRawText(y))) {
        --y;
    }
    while (y > 0 && !isLineBlank(document.lineRawText(y - 1))) {
        --y;
    }
    cursor_pos.y = y;
    cursor_pos.x = 0;
    desired_cursor_pos = 0;
}

void CursorController::moveToMatchingBracket() {
    const auto at_cursor = charAt(document, TextPos{cursor_pos.y, cursor_pos.x});
    if (!at_cursor.has_value()) {
        return;
    }

    char open = '\0', close = '\0';
    int direction = 0;

    if (*at_cursor == '(') {
        open = '(';
        close = ')';
        direction = +1;
    }
    else if (*at_cursor == ')') {
        open = '(';
        close = ')';
        direction = -1;
    }
    else if (*at_cursor == '[') {
        open = '[';
        close = ']';
        direction = +1;
    }
    else if (*at_cursor == ']') {
        open = '[';
        close = ']';
        direction = -1;
    }
    else if (*at_cursor == '{') {
        open = '{';
        close = '}';
        direction = +1;
    }
    else if (*at_cursor == '}') {
        open = '{';
        close = '}';
        direction = -1;
    }

    if (direction == 0) {
        return;
    }

    TextPos matched{};
    if (findMatchingDelimiter(document, TextPos{cursor_pos.y, cursor_pos.x}, open, close, direction, matched)) {
        cursor_pos.y = matched.y;
        cursor_pos.x = matched.x;
        desired_cursor_pos = cursor_pos.x;
    }
}

void CursorController::moveToMatchingBrace() {
    const auto at_cursor = charAt(document, TextPos{cursor_pos.y, cursor_pos.x});
    if (!at_cursor.has_value()) {
        return;
    }

    char open = '\0';
    char close = '\0';
    int direction = 0;

    if (*at_cursor == '{') {
        open = '{';
        close = '}';
        direction = +1;
    }
    else if (*at_cursor == '}') {
        open = '{';
        close = '}';
        direction = -1;
    }

    if (direction == 0) {
        return;
    }

    TextPos matched{};
    if (findMatchingDelimiter(document, TextPos{cursor_pos.y, cursor_pos.x}, open, close, direction, matched)) {
        cursor_pos.y = matched.y;
        cursor_pos.x = matched.x;
        desired_cursor_pos = cursor_pos.x;
    }
}
