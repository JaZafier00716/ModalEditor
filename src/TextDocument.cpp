//
// Created by jan on 4/16/26.
//

#include "../include/TextDocument.h"

#include <algorithm>
#include <fstream>

size_t TextDocument::lineCount() const {
    return lines.size();
}

bool TextDocument::empty() const {
    return lines.empty();
}

size_t TextDocument::lineRawLength(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }
    return lines[static_cast<size_t>(index)].raw.size();
}

size_t TextDocument::lineRenderedLength(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }
    return lines[static_cast<size_t>(index)].rendered.size();
}

std::string TextDocument::lineRawText(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return "";
    }
    return lines[static_cast<size_t>(index)].raw;
}

std::string_view TextDocument::lineRenderedTextView(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return "";
    }
    return lines[static_cast<size_t>(index)].rendered;
}

int TextDocument::renderColumnFromRaw(const int index, const int raw_column) const {
    if (index < 0 || static_cast<size_t>(index) >= lines.size()) {
        return 0;
    }

    const auto& raw_line = lines[static_cast<size_t>(index)].raw;
    const auto clamped_raw_col = std::clamp(raw_column, 0, static_cast<int>(raw_line.size()));
    int render_col = 0;

    for (size_t i = 0; i < clamped_raw_col; ++i) {
        render_col += raw_line[i] == '\t' ? tab_spaces : 1;
    }

    return render_col;
}

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

bool TextDocument::eraseCharBefore(const int line, const int column) {
    if (line < 0 || static_cast<size_t>(line) >= lines.size() || column <= 0) {
        return false;
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