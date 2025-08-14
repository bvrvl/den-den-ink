#include "note_formatter.hpp"
#include <iostream>
#include <iomanip>

namespace formatter {

void print_notes(const std::vector<FullNote>& notes) {
    if (notes.empty()) {
        std::cout << "No notes found. 🐌" << std::endl;
        return;
    }

    std::cout << "--- 🐌 Den Den Ink Found " << notes.size() << " Note(s) ---" << std::endl;

    for (const auto& note : notes) {
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "ID:        " << note.id << std::endl;
        std::cout << "Type:      " << note.type << std::endl;
        std::cout << "Created:   " << note.timestamp << std::endl;
        
        if (!note.tags.empty()) {
            std::cout << "Tags:      ";
            for (const auto& tag : note.tags) {
                std::cout << "#" << tag << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "\n> " << note.text << std::endl;

        if (note.type == "programming") {
            std::cout << "\n  [Code Meta]" << std::endl;
            std::cout << "  Directory: " << note.metadata.current_directory << std::endl;
            if (note.metadata.git_branch != "N/A") {
                std::cout << "  Git Branch: " << note.metadata.git_branch << std::endl;
            }
        }
        std::cout << "----------------------------------------" << std::endl;
    }
}

}