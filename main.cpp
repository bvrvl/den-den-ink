// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>     
#include <sqlite3.h>    
#include "database.hpp" 

void show_usage() {
    std::cout << "Welcome to Den Den Ink! 🐌" << std::endl;
    std::cout << "A simple CLI note-taking tool." << std::endl;
    std::cout << "\nUsage:" << std::endl;
    std::cout << "  ink \"note text\" [#tags...]" << std::endl;
    std::cout << "  ink p \"programming note\" [#tags...]" << std::endl;
    std::cout << "  ink search \"query\" [#tags...]" << std::endl;
    std::cout << "  ink list [#tag]" << std::endl;
    std::cout << "  ink stats" << std::endl;
}

int main(int argc, char* argv[]) {
    // --- Database Initialization ---
    const char* home_dir = getenv("HOME");
    if (home_dir == nullptr) {
        std::cerr << "Error: Could not find HOME environment variable." << std::endl;
        return 1;
    }
    std::string db_path = std::string(home_dir) + "/.den_den_ink.db";
    
    sqlite3* db_connection = db::init_database(db_path);

    if (!db_connection) {
        std::cerr << "Failed to initialize the database. Exiting." << std::endl;
        return 1;
    }

    std::vector<std::string> args(argv, argv + argc);

    if (args.size() < 2) {
        show_usage();
        sqlite3_close(db_connection);
        return 1;
    }

    std::string command = args[1];

    std::cout << "Den Den Ink is processing your request..." << std::endl;
    std::cout << "Detected command/start of input: \"" << command << "\"" << std::endl;

    // Command dispatching will go here.

    // --- Cleanup ---
    sqlite3_close(db_connection);
    std::cout << "Database connection closed." << std::endl;

    return 0;
}