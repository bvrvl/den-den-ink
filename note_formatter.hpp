#ifndef NOTE_FORMATTER_HPP
#define NOTE_FORMATTER_HPP

#include <vector>
#include "database.hpp" // For the FullNote struct

namespace formatter {
    void print_notes(const std::vector<FullNote>& notes);
}

#endif