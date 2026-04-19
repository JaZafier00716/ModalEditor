//
// Created by jan on 4/16/26.
//

#ifndef MODALEDITOR_TEXTDOCUMENT_H
#define MODALEDITOR_TEXTDOCUMENT_H
#include <vector>
#include <string>
#include <string_view>

class TextDocument {
public:

    [[nodiscard]] size_t lineCount() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t lineRawLength(int index) const;
    [[nodiscard]] size_t lineRenderedLength(int index) const;
    [[nodiscard]] std::string lineRawText(int index) const;
    [[nodiscard]] std::string_view lineRenderedTextView(int index) const;
    [[nodiscard]] int renderColumnFromRaw(int index, int raw_column) const;

    bool insertCharAt(int line, int column, char c);
    bool insertNewlineAt(int line, int column);
    bool eraseCharBefore(int line, int column);
    // TODO: eraseCharAfter

    bool loadFromFile(const std::string& filename, std::string& error_message);
    // TODO: saveToFile

private:
    void updateRow(const std::string& raw_row, std::string& rendered_row) const;

    struct DocumentLine {
        std::string raw;
        std::string rendered;
    };

    std::vector<DocumentLine> lines; // TODO: Gap buffer
    int tab_spaces{4};
};

#endif //MODALEDITOR_TEXTDOCUMENT_H
