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
        moveWordForward();
    }
    else if constexpr (std::is_same_v<T, editor_motion::WordBackward>) {
        moveWordBackward();
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

void CursorController::moveWordForward() {
    const auto row_count = static_cast<int>(document.lineCount());
    if (row_count <= 0) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};

    auto advance = [&]() {
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
    };
    const auto ch = charAt(document, pos);
    if (ch.has_value() && isWordChar(*ch)) {
        // If we're on a word, skip to the end of it first
        while (true) {
            const auto cur = charAt(document, pos);
            if (!cur.has_value() || !isWordChar(*cur)) {
                break;
            }
            if (!advance()) {
                break;
            }
        }
    }

    while (true) {
        const auto cur = charAt(document, pos);
        if (!cur.has_value() || !isWordChar(*cur)) {
            break;
        }
        if (!advance()) {
            break;
        }
    }

    cursor_pos.y = pos.y;
    cursor_pos.x = pos.x;
    desired_cursor_pos = cursor_pos.x;
}

void CursorController::moveWordBackward() {
    const auto row_count = static_cast<int>(document.lineCount());

    if (row_count <= 0) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};
    auto retreat = [&]() {
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
    };

    if (!retreat()) {
        return;
    }

    while (true) {
        const auto cur = charAt(document, pos);
        if (!cur.has_value() || !isWordChar(*cur)) {
            break;
        }
        if (!retreat()) {
            break;
        }
    }

    while (true) {
        const auto cur = charAt(document, pos);
        if (!cur.has_value() || isWordChar(*cur)) {
            break;
        }
        const TextPos previous = pos;
        if (!retreat()) {
            break;
        }

        const auto before = charAt(document, pos);
        if (!before.has_value() || !isWordChar(*before)) {
            pos = previous;
            break;
        }
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
    const auto row_count = static_cast<int>(document.lineCount());
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

    if (direction == 0) {
        return;
    }

    TextPos pos{cursor_pos.y, cursor_pos.x};
    int depth = 1;
    auto step = [&]() {
        if (direction > 0) {
            const auto line_len = static_cast<int>(document.lineRawLength(pos.y));
            if (pos.x + 1 < line_len) {
                ++pos.x;
                return true;
            }
            if (pos.y + 1 < row_count) {
                ++pos.y;
                pos.x = 0;
                return true;
            }
            return false;
        }
        if (pos.x > 0) {
            --pos.x;
            return true;
        }
        if (pos.y > 0) {
            --pos.y;
            pos.x = static_cast<int>(document.lineRawLength(pos.y));
            if (pos.x > 0) {
                --pos.x;
            }
            return true;
        }
        return false;
    };

    while (step()) {
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
            cursor_pos.y = pos.y;
            cursor_pos.x = pos.x;
            desired_cursor_pos = cursor_pos.x;
            break;
        }
    }
}

void CursorController::moveToMatchingBrace() {
    const auto row_count = static_cast<int>(document.lineCount());
    const auto at_cursor = charAt(document, TextPos{cursor_pos.y, cursor_pos.x});
    if (!at_cursor.has_value()) {
        return;
    }

    char open = '\0', close = '\0';
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

    TextPos pos{cursor_pos.y, cursor_pos.x};
    int depth = 1;
    auto step = [&]() {
        if (direction > 0) {
            const auto line_len = static_cast<int>(document.lineRawLength(pos.y));
            if (pos.x + 1 < line_len) {
                ++pos.x;
                return true;
            }
            if (pos.y + 1 < row_count) {
                ++pos.y;
                pos.x = 0;
                return true;
            }
            return false;
        }
        if (pos.x > 0) {
            --pos.x;
            return true;
        }
        if (pos.y > 0) {
            --pos.y;
            pos.x = static_cast<int>(document.lineRawLength(pos.y));
            if (pos.x > 0) {
                --pos.x;
            }
            return true;
        }
        return false;
    };

    while (step()) {
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
            cursor_pos.y = pos.y;
            cursor_pos.x = pos.x;
            desired_cursor_pos = cursor_pos.x;
            break;
        }
    }
}
