# Den Den Ink 🐌

Den Den Ink is a fast, simple, and thematic command-line tool for taking notes, inspired by the loyal communication snails of *One Piece*. It's built in C++ for performance and uses SQLite for robust, local storage.

## Features

-   **General & Programming Notes**: Supports standard notes and specialized notes for developers that automatically capture context like the current directory and Git branch.
-   **Flexible Tagging**: Add `#tags` anywhere in your note—before, after, or even inside the text.
-   **Powerful Search**: Quickly find notes by text and tags.
-   **Quick Listing**: List recent notes or filter by a specific tag.
-   **Insightful Stats**: Get an overview of your note-taking habits, including top tags and project activity.
-   **Thematic Flair**: Fun, snail-themed confirmations and icons.

## Installation

You can install Den Den Ink by building it from the source.

### Prerequisites

You will need:
- A C++17 compliant compiler (like `clang++` on macOS or `g++` on Linux).
- The SQLite3 development libraries.

**On macOS (with Homebrew):**
```bash
brew install sqlite
```
**On Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install libsqlite3-dev build-essential
```

###Build from Source
1. Clone the repository
```bash
git clone https://github.com/bvrvl/den-den-ink.git
cd den-den-ink
```

2. Compile the application
```bash
clang++ -std=c++17 -o ink main.cpp database.cpp metadata_collector.cpp note_formatter.cpp stats_engine.cpp -lsqlite3
```

3. Make it globally accesible:
```bash
sudo mv ink /usr/local/bin/
```

## Usage
Here are the 5 core commands:
1. Add a General Note:
```bash
# Tags can be anywhere
ink "This is a general note" #idea
ink #shopping "Buy milk and eggs"
ink "Remember to call #mom"
```
2. Add a Programming Note
*(The p command must immediately follow ink)*
```bash
ink p "Refactored the database module" #cpp #refactor
```
3. Search for Notes
```bash
# Search by text
ink search "database"

# Search by text and tag
ink search "refactor" #cpp
```
4. List Notes
```bash
# List the last 10 notes
ink list

# List all notes with a specific tag
ink list #shopping
```
5. Show Statistics
```bash
ink stats
```

