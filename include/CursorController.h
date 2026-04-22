//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_CURSORCONTROLLER_H
#define MODALEDITOR_CURSORCONTROLLER_H

#include <string>
#include <vector>

#include "TerminalManagement.h"
#include "TextDocument.h"

using std::string, std::vector;

/**
 * @brief Handles logical cursor movement over a text document.
 */
class CursorController {
public:
    /**
     * @brief Creates a cursor controller bound to the editor state.
     * @param document Editable text document.
     * @param screen_size Current terminal dimensions.
     */
    CursorController(
        TextDocument& document,
        int2d& screen_size
    ) :
    document{document},
    screen_size{screen_size} {};

    /** @brief Applies a high-level motion command to the cursor. */
    void applyMotion(const editor_motion::Motion& motion);
    /** @brief Sets the desired horizontal cursor position in raw columns. */
    void setCursorPositionX(int x);
    /** @brief Moves the cursor to the beginning of the current line. */
    void moveCursorLineStart();
    /** @brief Moves the cursor to the end of the current line. */
    void moveCursorLineEnd();
    /** @brief Moves the cursor one character to the right. */
    void moveCursorRightOne();

    /** @brief Returns the current cursor position in document coordinates. */
    [[nodiscard]] int2d getCursorPosition() const { return cursor_pos; }
    /** @brief Returns the preferred horizontal cursor position when moving vertically. */
    [[nodiscard]] int getDesiredCursorPos() const { return desired_cursor_pos; }

private:
    /** @brief Returns the raw character length of the given row. */
    [[nodiscard]] int getRowLength(int y) const;
    /** @brief Clamps the cursor to valid document bounds. */
    void clampCursorPosition();

    // Motion implementations
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void movePageUp();
    void movePageDown();
    void moveFileStart();
    void moveFileEnd();
    void moveWordForward();
    void moveWordBackward();
    void moveScreenTop();
    void moveScreenMiddle();
    void moveScreenBottom();
    void moveParagraphForward();
    void moveParagraphBackward();
    void moveToMatchingBracket();
    void moveToMatchingBrace();

    TextDocument& document;
    int2d cursor_pos{0,0};
    int desired_cursor_pos{0};
    int2d& screen_size;
};

#endif //MODALEDITOR_CURSORCONTROLLER_H
