#include "metadata_collector.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <filesystem>
#include <algorithm>  
#include <chrono>

// Define the namespace alias for convenience
namespace fs = std::filesystem;

namespace metadata {

// Helper function to execute a shell command and get its output.
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    // Using a unique_ptr to ensure pclose is called automatically
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    // Trim trailing newline characters which shell commands often add
    if (!result.empty() && result[result.length() - 1] == '\n') {
        result.erase(result.length() - 1);
    }
    return result;
}

// Finds the most recently edited file in the current directory (recursively).
std::string get_last_edited_file(const fs::path& directory) {
    fs::file_time_type latest_time;
    fs::path latest_file_path;
    bool has_found_file = false;

    try {
        // Iterate through all files and subdirectories
        for (const auto& entry : fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied)) {
            // Ignore directories, symlinks, and anything inside a .git folder
            if (entry.is_regular_file() && entry.path().string().find(".git") == std::string::npos) {
                if (!has_found_file) {
                    latest_time = entry.last_write_time();
                    latest_file_path = entry.path();
                    has_found_file = true;
                } else {
                    if (entry.last_write_time() > latest_time) {
                        latest_time = entry.last_write_time();
                        latest_file_path = entry.path();
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // This can happen if a directory is deleted while we are scanning it
        std::cerr << "Filesystem error while scanning: " << e.what() << std::endl;
        return "N/A";
    }

    if (!has_found_file) {
        return "N/A";
    }
    
    // Return just the filename part of the path
    return latest_file_path.filename().string();
}

ProgMetadata collect_metadata() {
    ProgMetadata data;
    fs::path current_path;

    try {
        current_path = fs::current_path();
    } catch(const fs::filesystem_error& e) {
        std::cerr << "Error getting current path: " << e.what() << std::endl;
        // Return empty metadata if we can't even get the path
        return data;
    }
    
    // 1. Get current working directory
    data.current_directory = current_path.string();

    // 2. Check if we are in a Git repository
    if (fs::exists(current_path / ".git")) {
        data.git_branch = exec("git rev-parse --abbrev-ref HEAD");
        data.git_commit_hash = exec("git rev-parse HEAD");
    } else {
        data.git_branch = "N/A";
        data.git_commit_hash = "N/A";
    }

    // 3. Get last edited file
    data.last_edited_file = get_last_edited_file(current_path);

    return data;
}

}