#include "database.hpp"
#include <iostream>
#include <vector>
#include <sstream>

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
        "id INTEGER PRIMARY KEY AUTOINCREMENT, text TEXT NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, type TEXT NOT NULL);",
        "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, note_id INTEGER NOT NULL, "
        "tag_name TEXT NOT NULL, FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE);",
        "CREATE TABLE IF NOT EXISTS metadata ("
        "note_id INTEGER PRIMARY KEY, current_directory TEXT, last_edited_file TEXT, "
        "git_branch TEXT, recent_commit_hash TEXT, "
        "FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE);"
    };
    execute_sql(db, "PRAGMA foreign_keys = ON;");
    for(const auto& sql : create_tables_sql) {
        if (!execute_sql(db, sql)) {
            sqlite3_close(db);
            return nullptr;
        }
    }
    return db;
}

bool add_general_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'general');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_finalize(stmt);
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
            sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false;
        }
        sqlite3_finalize(stmt);
    }
    return execute_sql(db, "COMMIT;");
}

bool add_prog_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags, const ProgMetadata& metadata) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'programming');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_finalize(stmt);
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
            sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false;
        }
        sqlite3_finalize(stmt);
    }
    std::string meta_sql = "INSERT INTO metadata (note_id, current_directory, last_edited_file, git_branch, recent_commit_hash) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, meta_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_bind_int64(stmt, 1, note_id);
    sqlite3_bind_text(stmt, 2, metadata.current_directory.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, metadata.last_edited_file.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, metadata.git_branch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, metadata.git_commit_hash.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false;
    }
    sqlite3_finalize(stmt);
    return execute_sql(db, "COMMIT;");
}


// Base query that joins all tables. Using GROUP_CONCAT to collect all tags for a note.
const std::string BASE_SELECT_QUERY = 
    "SELECT n.id, n.text, n.timestamp, n.type, "
    "m.current_directory, m.last_edited_file, m.git_branch, "
    "GROUP_CONCAT(t.tag_name, ' ') " // Note the space as a separator
    "FROM notes n "
    "LEFT JOIN metadata m ON n.id = m.note_id "
    "LEFT JOIN tags t ON n.id = t.note_id ";

// Helper function to turn a single database row into our FullNote struct.
FullNote process_row(sqlite3_stmt* stmt) {
    FullNote note;
    note.id = sqlite3_column_int64(stmt, 0);
    note.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    note.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    note.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    const char* dir = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    note.metadata.current_directory = dir ? dir : "";
    const char* file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    note.metadata.last_edited_file = file ? file : "";
    const char* branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    note.metadata.git_branch = branch ? branch : "";
    
    // Tags are returned as a single space-separated string. We split them.
    const char* tags_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    if (tags_str) {
        std::stringstream ss(tags_str);
        std::string tag;
        while (ss >> tag) {
            note.tags.push_back(tag);
        }
    }
    return note;
}

std::vector<FullNote> list_recent_notes(sqlite3* db, int limit) {
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY + "GROUP BY n.id ORDER BY n.timestamp DESC LIMIT ?;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Error: " << sqlite3_errmsg(db) << std::endl;
        return notes;
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        notes.push_back(process_row(stmt));
    }
    sqlite3_finalize(stmt);
    return notes;
}

std::vector<FullNote> list_notes_by_tag(sqlite3* db, const std::string& tag) {
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY + 
        "WHERE n.id IN (SELECT note_id FROM tags WHERE tag_name = ?) "
        "GROUP BY n.id ORDER BY n.timestamp DESC;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Error: " << sqlite3_errmsg(db) << std::endl;
        return notes;
    }
    std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
    sqlite3_bind_text(stmt, 1, clean_tag.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        notes.push_back(process_row(stmt));
    }
    sqlite3_finalize(stmt);
    return notes;
}

std::vector<FullNote> search_notes(sqlite3* db, const std::string& query, const std::vector<std::string>& tags) {
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY;
    std::string where_clause = "WHERE n.text LIKE ? ";
    int param_index = 1;

    // A more robust search that can handle multiple tags (though we only implement one for now)
    for (const auto& tag : tags) {
        where_clause += "AND n.id IN (SELECT note_id FROM tags WHERE tag_name = ?) ";
    }
    
    sql += where_clause + "GROUP BY n.id ORDER BY n.timestamp DESC;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Error: " << sqlite3_errmsg(db) << std::endl;
        return notes;
    }

    std::string search_term = "%" + query + "%";
    sqlite3_bind_text(stmt, param_index++, search_term.c_str(), -1, SQLITE_STATIC);

    for (const auto& tag : tags) {
        std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
        sqlite3_bind_text(stmt, param_index++, clean_tag.c_str(), -1, SQLITE_STATIC);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        notes.push_back(process_row(stmt));
    }
    sqlite3_finalize(stmt);
    return notes;
}

}