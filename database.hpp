// database.hpp
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector> 
#include <sqlite3.h>

namespace db {
    sqlite3* init_database(const std::string& db_path);
    bool execute_sql(sqlite3* db, const std::string& sql);

    // Adds a new note and its tags to the database. Returns true on success.
    bool add_note(sqlite3* db, const std::string& text, const std::string& type, const std::vector<std::string>& tags);
}

#endif 