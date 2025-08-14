#include "database.hpp"
#include <iostream>
#include <vector>

namespace db {

bool execute_sql(sqlite3* db, const std::string& sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

sqlite3* init_database(const std::string& db_path) {
    sqlite3* db;
    int rc = sqlite3_open(db_path.c_str(), &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    } else {
        std::cout << "Opened database successfully." << std::endl;
    }

    // SQL to create tables
    const std::string notes_table_sql =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "text TEXT NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "type TEXT NOT NULL"
        ");";

    const std::string tags_table_sql =
        "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "note_id INTEGER NOT NULL, "
        "tag_name TEXT NOT NULL, "
        "FOREIGN KEY(note_id) REFERENCES notes(id)"
        ");";

    const std::string metadata_table_sql =
        "CREATE TABLE IF NOT EXISTS metadata ("
        "note_id INTEGER PRIMARY KEY, "
        "current_directory TEXT, "
        "last_edited_file TEXT, "
        "git_branch TEXT, "
        "recent_commit_hash TEXT, "
        "FOREIGN KEY(note_id) REFERENCES notes(id)"
        ");";

    // Execute creation statements
    if (!execute_sql(db, notes_table_sql)) return nullptr;
    if (!execute_sql(db, tags_table_sql)) return nullptr;
    if (!execute_sql(db, metadata_table_sql)) return nullptr;

    std::cout << "Tables are ready." << std::endl;
    return db;
}

} // namespace db