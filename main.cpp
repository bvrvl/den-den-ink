#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <sqlite3.h>
#include "database.hpp"
#include "metadata_collector.hpp"
#include "note_formatter.hpp"
#include "stats_engine.hpp" 

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
    std::string combined_text;
    for (size_t i = start_index; i < args.size(); ++i) {
        if (!args[i].empty() && args[i][0] == '#') {
            if (std::find(tags.begin(), tags.end(), args[i]) == tags.end()) {
                 tags.push_back(args[i]);
            }
        } else {
            text_builder << args[i] << " ";
        }
    }
    combined_text = text_builder.str();
    if (!combined_text.empty()) { combined_text.pop_back(); }
    if (combined_text.length() >= 2 && combined_text.front() == '"' && combined_text.back() == '"') {
        text = combined_text.substr(1, combined_text.length() - 2);
    } else {
        text = combined_text;
    }
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        if (!word.empty() && word[0] == '#') {
            if (std::find(tags.begin(), tags.end(), word) == tags.end()) {
                tags.push_back(word);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    const char* home_dir = getenv("HOME");
    if (home_dir == nullptr) {
        std::cerr << "Error: Could not find HOME environment variable." << std::endl;
        return 1;
    }
    std::string db_path = std::string(home_dir) + "/.den_den_ink.db";
    sqlite3* db_connection = db::init_database(db_path);
    if (!db_connection) return 1;

    std::vector<std::string> args(argv, argv + argc);
    if (args.size() < 2) {
        show_usage();
        sqlite3_close(db_connection);
        return 0;
    }

    std::string command = args[1];

    if (command == "p") {
        if (args.size() < 3) {
            std::cerr << "Error: Programming note text cannot be empty." << std::endl;
            show_usage();
        } else {
            std::string note_text;
            std::vector<std::string> tags;
            parse_note_input(args, 2, note_text, tags);
            ProgMetadata metadata = metadata::collect_metadata();
            if (db::add_prog_note(db_connection, note_text, tags, metadata)) {
                std::cout << "\n🐌 Ink captured!" << std::endl;
            } else {
                std::cerr << "Error: Failed to save your programming note." << std::endl;
            }
        }
    } else if (command == "list") {
        if (args.size() == 2) {
            formatter::print_notes(db::list_recent_notes(db_connection, 10));
        } else if (args.size() == 3 && args[2][0] == '#') {
            formatter::print_notes(db::list_notes_by_tag(db_connection, args[2]));
        } else {
            show_usage();
        }
    } else if (command == "search") {
        if (args.size() < 3) {
            std::cerr << "Error: Search query cannot be empty." << std::endl;
            show_usage();
        } else {
            std::string query;
            std::vector<std::string> tags;
            parse_note_input(args, 2, query, tags);
            formatter::print_notes(db::search_notes(db_connection, query, tags));
        }
    } else if (command == "stats") {
        AppStats app_stats = stats::gather_stats(db_connection);
        stats::print_stats(app_stats);
    } else {
        // Default action: General note
        std::string note_text;
        std::vector<std::string> tags;
        parse_note_input(args, 1, note_text, tags);
        if (note_text.empty()) {
            std::cerr << "Error: Note text cannot be empty." << std::endl;
            show_usage();
        } else {
            if (db::add_general_note(db_connection, note_text, tags)) {
                std::cout << "\n🐌 Ink captured!" << std::endl;
            } else {
                std::cerr << "Error: Failed to save your note." << std::endl;
            }
        }
    }

    sqlite3_close(db_connection);
    return 0;
}