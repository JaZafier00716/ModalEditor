//
// Created by jan on 4/16/26.
//

#include "../include/TextDocument.h"

#include <algorithm>
#include <fstream>

/** @brief Returns the number of lines stored in the document. */
size_t TextDocument::lineCount() const {
    return lines.size();
}

/** @brief Returns whether the document currently has no lines. */
bool TextDocument::empty() const {
    return lines.empty();
}

/**
 * @brief Returns raw length for a line index.
 * @param index Line index.
 * @return Raw character length, or 0 for invalid index.
 */
size_t TextDocument::lineRawLength(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }
    return lines[static_cast<size_t>(index)].raw.size();
}

/**
 * @brief Returns rendered length for a line index.
 * @param index Line index.
 * @return Rendered character length, or 0 for invalid index.
 */
size_t TextDocument::lineRenderedLength(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }
    return lines[static_cast<size_t>(index)].rendered.size();
}

/**
 * @brief Returns raw text for a line index.
 * @param index Line index.
 * @return Raw line text copy, or empty string for invalid index.
 */
std::string TextDocument::lineRawText(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return "";
    }
    return lines[static_cast<size_t>(index)].raw;
}

/**
 * @brief Returns rendered text view for a line index.
 * @param index Line index.
 * @return Rendered line view, or empty view for invalid index.
 */
std::string_view TextDocument::lineRenderedTextView(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return "";
    }
    return lines[static_cast<size_t>(index)].rendered;
}

/**
 * @brief Converts a raw column into rendered-space column.
 * @param index Line index.
 * @param raw_column Raw character column.
 * @return Rendered column value with tab expansion applied.
 */
int TextDocument::renderColumnFromRaw(const int index, const int raw_column) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }

    const auto& raw_line = lines[static_cast<size_t>(index)].raw;
    const auto clamped_raw_col = std::clamp(raw_column, 0, static_cast<int>(raw_line.size()));
    int render_col = 0;

    for (int i = 0; i < clamped_raw_col; ++i) {
        render_col += raw_line[i] == '\t' ? tab_spaces : 1;
    }

    return render_col;
}

/**
 * @brief Inserts a character at a specific line and column.
 * @param line Target line.
 * @param column Target raw column.
 * @param c Character to insert.
 * @return True on success.
 */
bool TextDocument::insertCharAt(const int line, const int column, const char c) {
    if (line < 0) {
        return false;
    }

    if (lines.empty()) {
        lines.emplace_back();
    }

    if (static_cast<size_t>(line) >= lines.size()) {
        return false;
    }

    auto& [raw, rendered] = lines[static_cast<size_t>(line)];
    const auto insert_col = std::clamp(column, 0, static_cast<int>(raw.size()));
    raw.insert(static_cast<size_t>(insert_col), 1, c);
    updateRow(raw, rendered);

    return true;
}

/**
 * @brief Inserts a newline by splitting a line at a column.
 * @param line Target line.
 * @param column Split column.
 * @return True on success.
 */
bool TextDocument::insertNewlineAt(const int line, const int column) {
    if (line < 0) {
        return false;
    }

    if (lines.empty()) {
        lines.emplace_back();
        lines.emplace_back();
        return true;
    }

    if (static_cast<size_t>(line) >= lines.size()) {
        return false;
    }

    auto& [current_raw, current_rendered] = lines[static_cast<size_t>(line)];
    const auto split_col = std::clamp(column, 0, static_cast<int>(current_raw.size()));

    DocumentLine next;
    next.raw = current_raw.substr(static_cast<size_t>(split_col));
    current_raw.erase(static_cast<size_t>(split_col));

    updateRow(current_raw, current_rendered);
    updateRow(next.raw, next.rendered);

    lines.insert(lines.begin() + line + 1, std::move(next));

    return true;
}

/**
 * @brief Erases the character immediately before a cursor location.
 * @param line Target line.
 * @param column Cursor raw column.
 * @return True on success.
 */
bool TextDocument::eraseCharBefore(const int line, const int column) {
    if (line < 0 || static_cast<size_t>(line) >= lines.size()) {
        return false;
    }

    if (column <= 0) {
        if (line == 0) {
            return false;
        }

        auto& [previous_raw, previous_rendered] = lines[static_cast<size_t>(line - 1)];
        previous_raw += lines[static_cast<size_t>(line)].raw;
        updateRow(previous_raw, previous_rendered);
        lines.erase(lines.begin() + line);
        return true;
    }

    auto& [raw, rendered] = lines[static_cast<size_t>(line)];
    if (raw.empty()) {
        return false;
    }

    const auto erase_col = std::clamp(column - 1, 0, static_cast<int>(raw.size()) - 1);
    raw.erase(static_cast<size_t>(erase_col), 1);
    updateRow(raw, rendered);

    return true;
}

std::string TextDocument::copyRange(const int start_line, const int start_column,
                                    const int end_line, const int end_column) const {
    if (start_line < 0 || end_line < start_line ||
        static_cast<size_t>(end_line) >= lines.size()) {
        return {};
    }

    const auto start_col = std::clamp(start_column, 0, static_cast<int>(lines[static_cast<size_t>(start_line)].raw.size()));
    const auto end_col = std::clamp(end_column, 0, static_cast<int>(lines[static_cast<size_t>(end_line)].raw.size()));

    if (start_line == end_line) {
        if (end_col <= start_col) {
            return {};
        }
        return lines[static_cast<size_t>(start_line)].raw.substr(static_cast<size_t>(start_col),
                                                                 static_cast<size_t>(end_col - start_col));
    }

    std::string copied = lines[static_cast<size_t>(start_line)].raw.substr(static_cast<size_t>(start_col));
    copied += '\n';

    for (int line = start_line + 1; line < end_line; ++line) {
        copied += lines[static_cast<size_t>(line)].raw;
        copied += '\n';
    }

    copied += lines[static_cast<size_t>(end_line)].raw.substr(0, static_cast<size_t>(end_col));
    return copied;
}

std::string TextDocument::copyLineRange(const int start_line, const int end_line) const {
    if (start_line < 0 || end_line < start_line ||
        static_cast<size_t>(end_line) >= lines.size()) {
        return {};
    }

    std::string copied;
    for (int line = start_line; line <= end_line; ++line) {
        copied += lines[static_cast<size_t>(line)].raw;
        copied += '\n';
    }
    return copied;
}

bool TextDocument::eraseRange(const int start_line, const int start_column,
                              const int end_line, const int end_column) {
    if (start_line < 0 || end_line < start_line ||
        static_cast<size_t>(end_line) >= lines.size()) {
        return false;
    }

    auto start_col = std::clamp(start_column, 0, static_cast<int>(lines[static_cast<size_t>(start_line)].raw.size()));
    auto finish_col = std::clamp(end_column, 0, static_cast<int>(lines[static_cast<size_t>(end_line)].raw.size()));

    if (start_line == end_line) {
        if (finish_col <= start_col) {
            return true;
        }

        auto& [raw, rendered] = lines[static_cast<size_t>(start_line)];
        raw.erase(static_cast<size_t>(start_col), static_cast<size_t>(finish_col - start_col));
        updateRow(raw, rendered);
        return true;
    }

    auto& [start_raw, start_rendered] = lines[static_cast<size_t>(start_line)];
    const auto suffix = lines[static_cast<size_t>(end_line)].raw.substr(static_cast<size_t>(finish_col));
    start_raw = start_raw.substr(0, static_cast<size_t>(start_col)) + suffix;
    updateRow(start_raw, start_rendered);

    lines.erase(lines.begin() + start_line + 1, lines.begin() + end_line + 1);
    return true;
}

bool TextDocument::eraseLineRange(const int start_line, const int end_line) {
    if (start_line < 0 || end_line < start_line ||
        static_cast<size_t>(end_line) >= lines.size()) {
        return false;
    }

    lines.erase(lines.begin() + start_line, lines.begin() + end_line + 1);
    return true;
}


/**
 * @brief Rebuilds rendered row text from raw row text.
 * @param raw_row Source raw row.
 * @param rendered_row Destination rendered row.
 */
void TextDocument::updateRow(const std::string& raw_row, std::string& rendered_row) const {
    rendered_row.clear();

    for (const auto& raw_char : raw_row) {
        if (raw_char == '\t') {
            rendered_row += std::string(tab_spaces, ' '); // Replace tab with spaces for rendering
        } else {
            rendered_row += raw_char;
        }
    }
}


/**
 * @brief Loads document contents from a text file.
 * @param filename File path to read.
 * @param error_message Output error message on failure.
 * @return True on success.
 */
bool TextDocument::loadFromFile(const std::string& filename, std::string& error_message) {
    std::ifstream ifs(filename.c_str(), std::ios::in);

    if (!ifs.is_open()) {
        error_message = "Failed to open file";
        return false;
    }

    std::vector<DocumentLine> loaded_lines;

    std::string line;
    while (std::getline(ifs, line)) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }

        DocumentLine loaded_line;
        loaded_line.raw = line;
        updateRow(loaded_line.raw, loaded_line.rendered);
        loaded_lines.emplace_back(loaded_line);
    }

    if (ifs.bad()) {
        error_message = "Failed while reading file";
        return false;
    }

    lines = std::move(loaded_lines);
    error_message.clear();
    ifs.close();

    return true;
}

bool TextDocument::saveToFile(const std::string& filename, std::string& error_message) const {
    const std::string temp_filename = filename+".tmp";
    std::ofstream ofs(temp_filename, std::ios::out | std::ios::binary);

    if (!ofs.is_open()) {
        error_message = "Failed to open file for writing";
        return false;
    }

    for (const auto& [raw, rendered] : lines) {
        ofs << raw << "\n";
    }
    ofs.close();
    if (ofs.fail()) {
        error_message = "Failed while writing to disk";
        return false;
    }

    if (std::rename(temp_filename.c_str(), filename.c_str()) != 0) {
        error_message = "Failed to rename temporary file";
        return false;
    }

    

    error_message.clear();
    ofs.close();
    return true;
}
