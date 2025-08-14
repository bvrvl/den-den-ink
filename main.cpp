#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <sqlite3.h>
#include "database.hpp"

void show_usage() {
    std::cout << "Welcome to Den Den Ink! 🐌" << std::endl;
    std::cout << "A simple CLI note-taking tool." << std::endl;
    std::cout << "\nUsage:" << std::endl;
    std::cout << "  ink \"note text\" [#tags...]" << std::endl;
    std::cout << "  ink p \"programming note\" [#tags...]" << std::endl;
    std::cout << "  ink search \"query\" [#tags...]" << std::endl;
    std::cout << "  ink list [#tag]" << std::endl;
    std::cout << "  ink stats" << std::endl;
}

void parse_note_input(const std::vector<std::string>& args, int start_index, std::string& text, std::vector<std::string>& tags) {
    std::stringstream text_builder;
    
    for (size_t i = start_index; i < args.size(); ++i) {
        if (!args[i].empty() && args[i][0] == '#') {
            tags.push_back(args[i]);
        } else {
            text_builder << args[i] << " ";
        }
    }

    std::string full_text = text_builder.str();

    // Trim trailing space
    if (!full_text.empty()) {
        full_text.pop_back();
    }
    
    // Handle quoted text by removing the quotes if they exist
    if (full_text.size() > 1 && full_text.front() == '"' && full_text.back() == '"') {
        text = full_text.substr(1, full_text.length() - 2);
    } else {
        text = full_text;
    }

    // Second pass: find tags that might have been inside the quotes
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        if (!word.empty() && word[0] == '#') {
            // Avoid adding duplicate tags
            if (std::find(tags.begin(), tags.end(), word) == tags.end()) {
                tags.push_back(word);
            }
        }
    }
}


int main(int argc, char* argv[]) {
    // --- Database Initialization ---
    const char* home_dir = getenv("HOME");
    if (home_dir == nullptr) {
        std::cerr << "Error: Could not find HOME environment variable." << std::endl;
        return 1;
    }
    std::string db_path = std::string(home_dir) + "/.den_den_ink.db";
    sqlite3* db_connection = db::init_database(db_path);

    if (!db_connection) {
        return 1;
    }

    std::vector<std::string> args(argv, argv + argc);

    if (args.size() < 2) {
        show_usage();
        sqlite3_close(db_connection);
        return 1;
    }

    // --- Command Dispatching ---
    std::string command = args[1];

    // Check for specific commands first
    if (command == "p" || command == "search" || command == "list" || command == "stats") {
        // We will implement these later
        std::cout << "Command '" << command << "' is not yet implemented." << std::endl;
    } else {
        // DEFAULT ACTION: If it's not a known command, treat it as a general note.
        std::string note_text;
        std::vector<std::string> tags;
        
        // The note starts from the first argument (index 1)
        parse_note_input(args, 1, note_text, tags);

        if (note_text.empty()) {
            std::cerr << "Error: Note text cannot be empty." << std::endl;
            show_usage();
        } else {
            if (db::add_note(db_connection, note_text, "general", tags)) {
                std::cout << "\n🐌 Ink captured!" << std::endl;
            } else {
                std::cerr << "Error: Failed to save your note." << std::endl;
            }
        }
    }

    // --- Cleanup ---
    sqlite3_close(db_connection);
    return 0;
}