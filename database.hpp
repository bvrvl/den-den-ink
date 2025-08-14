#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <sqlite3.h>

namespace db {
    // Initializes the database. Creates the DB file and tables if they don't exist.
    // Returns a pointer to the database connection. The caller is responsible for closing it.
    sqlite3* init_database(const std::string& db_path);

    // A utility function to execute SQL statements.
    // It handles error checking and prints messages on failure.
    bool execute_sql(sqlite3* db, const std::string& sql);
}

#endif // DATABASE_HPP