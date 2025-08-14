#include "stats_engine.hpp"
#include "database.hpp"
#include <iostream>
#include <iomanip> 
#include <algorithm>

namespace stats {

AppStats gather_stats(sqlite3* db) {
    AppStats app_stats;
    app_stats.total_notes = db::get_total_notes_count(db);
    
    // Convert std::pair to StatItem
    auto tag_pairs = db::get_tag_counts(db);
    for (const auto& p : tag_pairs) {
        app_stats.top_tags.push_back({p.first, p.second});
    }

    auto project_pairs = db::get_project_counts(db);
    for (const auto& p : project_pairs) {
        app_stats.notes_per_project.push_back({p.first, p.second});
    }
    
    auto daily_pairs = db::get_daily_counts(db);
    for (const auto& p : daily_pairs) {
        app_stats.notes_per_day.push_back({p.first, p.second});
    }

    return app_stats;
}

void print_stats(const AppStats& stats) {
    std::cout << "\n--- 📊 Den Den Ink Statistics ---" << std::endl;
    
    // 1. Total Notes
    std::cout << "\nTotal Notes: " << stats.total_notes << std::endl;

    // 2. Top Tags
    if (!stats.top_tags.empty()) {
        std::cout << "\n--- Top 5 Tags ---" << std::endl;
        size_t limit = std::min<size_t>(5, stats.top_tags.size());
        for (size_t i = 0; i < limit; ++i) {
            std::cout << " #" << std::left << std::setw(20) << stats.top_tags[i].name 
                      << " (" << stats.top_tags[i].count << " uses)" << std::endl;
        }
    }

    // 3. Notes Per Project
    if (!stats.notes_per_project.empty()) {
        std::cout << "\n--- Top 5 Projects ---" << std::endl;
        size_t limit = std::min<size_t>(5, stats.notes_per_project.size());
        for (size_t i = 0; i < limit; ++i) {
             std::cout << " " << std::left << std::setw(30) << stats.notes_per_project[i].name 
                       << " (" << stats.notes_per_project[i].count << " notes)" << std::endl;
        }
    }
    
    // 4. Notes Per Day
    if (!stats.notes_per_day.empty()) {
        std::cout << "\n--- Recent Activity (Last 7 Days) ---" << std::endl;
        size_t limit = std::min<size_t>(7, stats.notes_per_day.size());
        for (size_t i = 0; i < limit; ++i) {
             std::cout << " " << stats.notes_per_day[i].name << ": " 
                       << stats.notes_per_day[i].count << " notes" << std::endl;
        }
    }

    std::cout << "\n-----------------------------------\n";
}

}