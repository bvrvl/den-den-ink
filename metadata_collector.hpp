#ifndef METADATA_COLLECTOR_HPP
#define METADATA_COLLECTOR_HPP

#include <string>

// A struct to hold all the metadata for a programming note.
struct ProgMetadata {
    std::string current_directory;
    std::string last_edited_file;
    std::string git_branch;
    std::string git_commit_hash;
};

namespace metadata {
    // Collects all relevant metadata from the current environment.
    ProgMetadata collect_metadata();
}

#endif