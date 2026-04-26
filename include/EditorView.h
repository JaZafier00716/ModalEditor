//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_EDITORVIEW_H
#define MODALEDITOR_EDITORVIEW_H
#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include "CursorController.h"
#include "EditorMode.h"

using std::vector, std::string;

/**
 * @brief Renders document content and status information to the terminal.
 */
class EditorView {
public:
    /**
     * @brief Creates a renderer bound to editor state.
     * @param document Text document to draw.
     * @param screen_size Current terminal dimensions.
     * @param cursor_controller Cursor state provider.
     */
    EditorView(
        TextDocument& document,
        int2d& screen_size,
        CursorController& cursor_controller
    ) :
        document{document},
        screen_size{screen_size},
        cursor_controller{cursor_controller}
    {};

    /** @brief Redraws the full editor frame and places the cursor. */
    void refreshScreen(const std::variant<NormalMode, InsertMode, CommandMode>& current_mode);
    /** @brief Sets the file name shown in the status bar. */
    void setFileName(std::string_view text);
    /** @brief Appends a message for display in the debug/status area. */
    void appendDebugMessage(std::string_view message);
    /** @brief Enables or disables rendering of line numbers. */
    void setLineNumbersEnabled(bool enabled);

    [[nodiscard]] string getFileName() const { return file_name; }

private:
    /** @brief Renders visible text rows and filler rows into the output buffer. */
    void printRows(string &s);
    /** @brief Updates viewport offsets so the cursor remains visible. */
    void scroll();
    /** @brief Returns a formatted relative line number for the given screen row. */
    [[nodiscard]] string getRelativeLineNumber(int y) const;


    TextDocument& document;
    int2d& screen_size;
    CursorController& cursor_controller;
    string file_name;
    string debug_info_message;
    bool line_numbers{true};
    int tab_spaces = 4;

    int render_pos{0}; // for tab rendering
    int2d offset{0,0}; // col and row offset
    int line_num_offset{0}; // offset for line numbers
    string output_buffer;
};

#endif //MODALEDITOR_EDITORVIEW_H
