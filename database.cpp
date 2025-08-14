#include "database.hpp"
#include <iostream>
#include <vector>
#include <sstream>

namespace db {

// --- CORE HELPER AND INIT FUNCTIONS ---
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
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }
    execute_sql(db, "PRAGMA foreign_keys = ON;");
    const std::vector<std::string> create_tables_sql = {
        "CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY AUTOINCREMENT, text TEXT NOT NULL, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, type TEXT NOT NULL);",
        "CREATE TABLE IF NOT EXISTS tags (id INTEGER PRIMARY KEY AUTOINCREMENT, note_id INTEGER NOT NULL, tag_name TEXT NOT NULL, FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE);",
        "CREATE TABLE IF NOT EXISTS metadata (note_id INTEGER PRIMARY KEY, current_directory TEXT, last_edited_file TEXT, git_branch TEXT, recent_commit_hash TEXT, FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE);"
    };
    for(const auto& sql : create_tables_sql) {
        if (!execute_sql(db, sql)) { sqlite3_close(db); return nullptr; }
    }
    return db;
}


// --- NOTE ADDING FUNCTIONS ---
bool add_general_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'general');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_finalize(stmt);
    long long note_id = sqlite3_last_insert_rowid(db);
    std::string tag_sql = "INSERT INTO tags (note_id, tag_name) VALUES (?, ?);";
    for (const auto& tag : tags) {
        if (sqlite3_prepare_v2(db, tag_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { execute_sql(db, "ROLLBACK;"); return false; }
        std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
        sqlite3_bind_int64(stmt, 1, note_id);
        sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false; }
        sqlite3_finalize(stmt);
    }
    return execute_sql(db, "COMMIT;");
}

bool add_prog_note(sqlite3* db, const std::string& text, const std::vector<std::string>& tags, const ProgMetadata& metadata) {
    if (!execute_sql(db, "BEGIN TRANSACTION;")) return false;
    sqlite3_stmt* stmt;
    std::string note_sql = "INSERT INTO notes (text, type) VALUES (?, 'programming');";
    if (sqlite3_prepare_v2(db, note_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_finalize(stmt);
    long long note_id = sqlite3_last_insert_rowid(db);
    std::string tag_sql = "INSERT INTO tags (note_id, tag_name) VALUES (?, ?);";
    for (const auto& tag : tags) {
        if (sqlite3_prepare_v2(db, tag_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { execute_sql(db, "ROLLBACK;"); return false; }
        std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
        sqlite3_bind_int64(stmt, 1, note_id);
        sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false; }
        sqlite3_finalize(stmt);
    }
    std::string meta_sql = "INSERT INTO metadata (note_id, current_directory, last_edited_file, git_branch, recent_commit_hash) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, meta_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_bind_int64(stmt, 1, note_id);
    sqlite3_bind_text(stmt, 2, metadata.current_directory.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, metadata.last_edited_file.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, metadata.git_branch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, metadata.git_commit_hash.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); execute_sql(db, "ROLLBACK;"); return false; }
    sqlite3_finalize(stmt);
    return execute_sql(db, "COMMIT;");
}

// --- NOTE RETRIEVAL FUNCTIONS ---
const std::string BASE_SELECT_QUERY = "SELECT n.id, n.text, n.timestamp, n.type, m.current_directory, m.last_edited_file, m.git_branch, GROUP_CONCAT(t.tag_name, ' ') FROM notes n LEFT JOIN metadata m ON n.id = m.note_id LEFT JOIN tags t ON n.id = t.note_id ";
FullNote process_row(sqlite3_stmt* stmt) { /* Implementation from Step 5 */
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
    const char* tags_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    if (tags_str) {
        std::stringstream ss(tags_str);
        std::string tag;
        while (ss >> tag) { note.tags.push_back(tag); }
    }
    return note;
}
std::vector<FullNote> list_recent_notes(sqlite3* db, int limit) { /* Implementation from Step 5 */
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY + "GROUP BY n.id ORDER BY n.timestamp DESC LIMIT ?;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { return notes; }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) { notes.push_back(process_row(stmt)); }
    sqlite3_finalize(stmt);
    return notes;
}
std::vector<FullNote> list_notes_by_tag(sqlite3* db, const std::string& tag) { /* Implementation from Step 5 */
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY + "WHERE n.id IN (SELECT note_id FROM tags WHERE tag_name = ?) GROUP BY n.id ORDER BY n.timestamp DESC;";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { return notes; }
    std::string clean_tag = (tag[0] == '#') ? tag.substr(1) : tag;
    sqlite3_bind_text(stmt, 1, clean_tag.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) { notes.push_back(process_row(stmt)); }
    sqlite3_finalize(stmt);
    return notes;
}
std::vector<FullNote> search_notes(sqlite3* db, const std::string& query, const std::vector<std::string>& tags) { /* Implementation from Step 5 */
    std::vector<FullNote> notes;
    sqlite3_stmt* stmt;
    std::string sql = BASE_SELECT_QUERY + "WHERE n.text LIKE ? GROUP BY n.id ORDER BY n.timestamp DESC;";
    if (!tags.empty()) { sql = BASE_SELECT_QUERY + "WHERE n.text LIKE ? AND n.id IN (SELECT note_id FROM tags WHERE tag_name = ?) GROUP BY n.id ORDER BY n.timestamp DESC;"; }
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) { return notes; }
    std::string search_term = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, search_term.c_str(), -1, SQLITE_STATIC);
    if (!tags.empty()) {
        std::string clean_tag = (tags[0][0] == '#') ? tags[0].substr(1) : tags[0];
        sqlite3_bind_text(stmt, 2, clean_tag.c_str(), -1, SQLITE_STATIC);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) { notes.push_back(process_row(stmt)); }
    sqlite3_finalize(stmt);
    return notes;
}

// ===== FUNCTIONS FOR STATISTICS =====

int get_total_notes_count(sqlite3* db) {
    sqlite3_stmt* stmt;
    int count = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM notes;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return count;
}

std::vector<std::pair<std::string, int>> get_tag_counts(sqlite3* db) {
    std::vector<std::pair<std::string, int>> results;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT tag_name, COUNT(*) as count FROM tags GROUP BY tag_name ORDER BY count DESC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int count = sqlite3_column_int(stmt, 1);
            results.push_back({name, count});
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<std::string, int>> get_project_counts(sqlite3* db) {
    std::vector<std::pair<std::string, int>> results;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT current_directory, COUNT(*) as count FROM metadata GROUP BY current_directory ORDER BY count DESC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int count = sqlite3_column_int(stmt, 1);
            results.push_back({name, count});
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<std::string, int>> get_daily_counts(sqlite3* db) {
    std::vector<std::pair<std::string, int>> results;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT STRFTIME('%Y-%m-%d', timestamp) as day, COUNT(*) as count FROM notes GROUP BY day ORDER BY day DESC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int count = sqlite3_column_int(stmt, 1);
            results.push_back({name, count});
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

}