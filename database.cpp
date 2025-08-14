// database.cpp
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

// Function to add a note
bool add_note(sqlite3* db, const std::string& text, const std::string& type, const std::vector<std::string>& tags) {
    // Begin a transaction to ensure all or nothing is written
    if (!execute_sql(db, "BEGIN TRANSACTION;")) {
        return false;
    }

    // 1. Insert the main note into the 'notes' table
    std::string insert_note_sql = "INSERT INTO notes (text, type) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, insert_note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare note insertion statement: " << sqlite3_errmsg(db) << std::endl;
        execute_sql(db, "ROLLBACK;"); // Rollback on failure
        return false;
    }

    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to execute note insertion: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        execute_sql(db, "ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);

    // 2. Get the ID of the note we just created
    long long note_id = sqlite3_last_insert_rowid(db);

    // 3. Insert all the tags into the 'tags' table
    if (!tags.empty()) {
        std::string insert_tag_sql = "INSERT INTO tags (note_id, tag_name) VALUES (?, ?);";
        
        for (const auto& tag : tags) {
            if (sqlite3_prepare_v2(db, insert_tag_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                std::cerr << "Failed to prepare tag insertion statement: " << sqlite3_errmsg(db) << std::endl;
                execute_sql(db, "ROLLBACK;");
                return false;
            }
            
            // Bind the note_id and the tag name (remove the '#' before inserting)
            std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
            sqlite3_bind_int64(stmt, 1, note_id);
            sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to execute tag insertion: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(stmt);
                execute_sql(db, "ROLLBACK;");
                return false;
            }
            sqlite3_finalize(stmt);
        }
    }

    return execute_sql(db, "COMMIT;");
}


} // namespace db