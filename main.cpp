#include <iostream>
#include <string>
#include <vector>

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
    // Convert C-style arguments to a vector of strings for easier handling
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() < 2) {
        show_usage();
        return 1;
    }

    // The first argument after the program name is our command or note text
    std::string command = args[1];

    std::cout << "Den Den Ink is processing your request..." << std::endl;
    std::cout << "Detected command/start of input: \"" << command << "\"" << std::endl;


    return 0;
}