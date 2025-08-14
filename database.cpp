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
    }

    const std::vector<std::string> create_tables_sql = {
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "text TEXT NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "type TEXT NOT NULL"
        ");",
        "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "note_id INTEGER NOT NULL, "
        "tag_name TEXT NOT NULL, "
        "FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE"
        ");",
        "CREATE TABLE IF NOT EXISTS metadata ("
        "note_id INTEGER PRIMARY KEY, "
        "current_directory TEXT, "
        "last_edited_file TEXT, "
        "git_branch TEXT, "
        "recent_commit_hash TEXT, "
        "FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE"
        ");"
    };
    
    // Enable foreign key support
    execute_sql(db, "PRAGMA foreign_keys = ON;");

    for(const auto& sql : create_tables_sql) {
        if (!execute_sql(db, sql)) {
            sqlite3_close(db);
            return nullptr;
        }
    }
    
    return db;
}


// Function for general notes
bool add_general_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;

    // 1. Insert note
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'general');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Prep Error: " << sqlite3_errmsg(db) << std::endl;
        execute_sql(db, "ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQL Exec Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        execute_sql(db, "ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);

    // 2. Get note ID and insert tags
    long long note_id = sqlite3_last_insert_rowid(db);
    std::string tag_sql = "INSERT INTO tags (note_id, tag_name) VALUES (?, ?);";
    for (const auto& tag : tags) {
        if (sqlite3_prepare_v2(db, tag_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            execute_sql(db, "ROLLBACK;"); return false;
        }
        std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
        sqlite3_bind_int64(stmt, 1, note_id);
        sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execute_sql(db, "ROLLBACK;"); return false;
        }
        sqlite3_finalize(stmt);
    }

    return execute_sql(db, "COMMIT;");
}

// Function for programming notes
bool add_prog_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags, const ProgMetadata& metadata) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;

    // 1. Insert base note (similar to general note)
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'programming');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_finalize(stmt);

    long long note_id = sqlite3_last_insert_rowid(db);

    // 2. Insert tags (same as general note)
    std::string tag_sql = "INSERT INTO tags (note_id, tag_name) VALUES (?, ?);";
    for (const auto& tag : tags) {
        if (sqlite3_prepare_v2(db, tag_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            execute_sql(db, "ROLLBACK;"); return false;
        }
        std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
        sqlite3_bind_int64(stmt, 1, note_id);
        sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execute_sql(db, "ROLLBACK;"); return false;
        }
        sqlite3_finalize(stmt);
    }

    // 3. Insert metadata
    std::string meta_sql = "INSERT INTO metadata (note_id, current_directory, last_edited_file, git_branch, recent_commit_hash) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, meta_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Prep Error (Metadata): " << sqlite3_errmsg(db) << std::endl;
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_bind_int64(stmt, 1, note_id);
    sqlite3_bind_text(stmt, 2, metadata.current_directory.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, metadata.last_edited_file.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, metadata.git_branch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, metadata.git_commit_hash.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQL Exec Error (Metadata): " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_finalize(stmt);

    return execute_sql(db, "COMMIT;");
}

}