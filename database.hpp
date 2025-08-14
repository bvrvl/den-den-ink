#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <sqlite3.h>
#include "metadata_collector.hpp"

// This struct represents a complete note retrieved from the database,
// including its main data, tags, and any associated programming metadata.
struct FullNote {
    long long id;
    std::string text;
    std::string type;
    std::string timestamp;
    std::vector<std::string> tags;
    ProgMetadata metadata; // Will be empty for general notes
};

namespace db {
    // Initialization
    sqlite3* init_database(const std::string& db_path);

    // Functions to add notes
    bool add_general_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags);
    bool add_prog_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags, const ProgMetadata& metadata);

    // Functions to retrieve notes
    std::vector<FullNote> list_recent_notes(sqlite3* db, int limit);
    std::vector<FullNote> list_notes_by_tag(sqlite3* db, const std::string& tag);
    std::vector<FullNote> search_notes(sqlite3* db, const std::string& query, const std::vector<std::string>& tags);
}

#endif