#ifndef STATS_ENGINE_HPP
#define STATS_ENGINE_HPP

#include <string>
#include <vector>
#include <sqlite3.h> // For the sqlite3 pointer

// A simple struct to hold a name and a count.
// Used for tags, projects, and daily counts.
struct StatItem {
    std::string name;
    int count;
};

// A comprehensive struct to hold all statistics for the application.
struct AppStats {
    int total_notes;
    std::vector<StatItem> top_tags;
    std::vector<StatItem> notes_per_project;
    std::vector<StatItem> notes_per_day;
};

namespace stats {
    // Gathers all statistics from the database.
    AppStats gather_stats(sqlite3* db);

    // Prints the gathered statistics to the console.
    void print_stats(const AppStats& stats);
}

#endif