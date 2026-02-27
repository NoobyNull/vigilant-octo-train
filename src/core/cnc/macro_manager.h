#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace dw {

// Forward declarations
class Database;

// A user-defined or built-in macro containing G-code to send to the CNC controller.
struct Macro {
    int id = -1;
    std::string name;
    std::string gcode;          // Multi-line G-code text
    std::string shortcut;       // e.g. "Ctrl+1", empty if none
    int sortOrder = 0;
    bool builtIn = false;       // Built-ins are non-deletable but editable
};

// Manages macro storage in SQLite with CRUD operations, built-in macro
// initialization, and G-code line parsing for sequential execution.
class MacroManager {
  public:
    explicit MacroManager(const std::string& dbPath);
    ~MacroManager();

    MacroManager(const MacroManager&) = delete;
    MacroManager& operator=(const MacroManager&) = delete;

    // CRUD
    std::vector<Macro> getAll() const;
    Macro getById(int id) const;
    int addMacro(const Macro& macro);           // returns new id
    void updateMacro(const Macro& macro);
    void deleteMacro(int id);                    // throws if builtIn
    void reorder(const std::vector<int>& ids);   // set sort_order by position

    // Execution -- splits gcode by newlines, sends each non-empty/non-comment line
    // Returns vector of lines that will be sent (for preview)
    std::vector<std::string> parseLines(const Macro& macro) const;

    // Expand M98 Pxxxx references in parsed lines, replacing with the referenced
    // macro's lines recursively. Throws std::runtime_error if recursion exceeds maxDepth.
    // NOTE: Callers should call expandLines(parseLines(macro)) before sending to
    // CncController to resolve M98 Pxxxx nested macro references.
    std::vector<std::string> expandLines(const std::vector<std::string>& lines,
                                          int maxDepth = MAX_NEST_DEPTH) const;

    static constexpr int MAX_NEST_DEPTH = 16;

    // Built-in initialization (called on first run or reset)
    void ensureBuiltIns();

  private:
    void initSchema();  // CREATE TABLE IF NOT EXISTS
    std::vector<std::string> expandLinesRecursive(
        const std::vector<std::string>& lines, int depth, int maxDepth) const;
    std::unique_ptr<Database> m_db;
};

} // namespace dw
