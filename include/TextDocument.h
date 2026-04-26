//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_TEXTDOCUMENT_H
#define MODALEDITOR_TEXTDOCUMENT_H
#include <vector>
#include <string>
#include <string_view>

/**
 * @brief Stores and edits the file text in raw and rendered forms.
 */
class TextDocument {
public:

    /** @brief Returns the number of lines currently stored. */
    [[nodiscard]] size_t lineCount() const;
    /** @brief Returns true when the document has no lines. */
    [[nodiscard]] bool empty() const;
    /** @brief Returns the raw character length of a line. */
    [[nodiscard]] size_t lineRawLength(int index) const;
    /** @brief Returns the rendered length of a line (tabs expanded). */
    [[nodiscard]] size_t lineRenderedLength(int index) const;
    /** @brief Returns a copy of the raw line text. */
    [[nodiscard]] std::string lineRawText(int index) const;
    /** @brief Returns a view of the rendered line text. */
    [[nodiscard]] std::string_view lineRenderedTextView(int index) const;
    /** @brief Converts a raw column to its rendered column for a line. */
    [[nodiscard]] int renderColumnFromRaw(int index, int raw_column) const;

    /** @brief Inserts a character at a given line and column. */
    bool insertCharAt(int line, int column, char c);
    /** @brief Splits a line by inserting a newline at a given column. */
    bool insertNewlineAt(int line, int column);
    /** @brief Removes the character before the provided cursor position. */
    bool eraseCharBefore(int line, int column);
    // TODO: eraseCharAfter

    /** @brief Loads document contents from disk. */
    bool loadFromFile(const std::string& filename, std::string& error_message);
    bool saveToFile(const std::string& filename, std::string& error_message) const;

private:
    /** @brief Rebuilds the rendered line from its raw text representation. */
    void updateRow(const std::string& raw_row, std::string& rendered_row) const;

    /** @brief One document line in both storage and display forms. */
    struct DocumentLine {
        std::string raw;
        std::string rendered;
    };

    std::vector<DocumentLine> lines; // TODO: Gap buffer
    int tab_spaces{4};
};

#endif //MODALEDITOR_TEXTDOCUMENT_H
