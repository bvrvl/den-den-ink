#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <sqlite3.h>
#include "metadata_collector.hpp" 

namespace db {
    sqlite3* init_database(const std::string& db_path);

    // General note adding function (now just calls the more specific one)
    bool add_general_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags);

    // Programming note adding function
    bool add_prog_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags, const ProgMetadata& metadata);
}

#endif // DATABASE_HPP