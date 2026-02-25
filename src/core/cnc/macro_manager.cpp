// MacroManager -- SQLite-backed macro storage with built-in macro initialization.

#include "core/cnc/macro_manager.h"

#include <algorithm>
#include <sstream>

#include "core/database/database.h"

namespace dw {

MacroManager::MacroManager(const std::string& dbPath)
    : m_db(std::make_unique<Database>()) {
    if (!m_db->open(dbPath)) {
        throw std::runtime_error("MacroManager: failed to open database at " + dbPath);
    }
    initSchema();
}

MacroManager::~MacroManager() = default;

void MacroManager::initSchema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS macros (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            gcode TEXT NOT NULL,
            shortcut TEXT DEFAULT '',
            sort_order INTEGER DEFAULT 0,
            built_in INTEGER DEFAULT 0
        );
    )";
    if (!m_db->execute(sql)) {
        throw std::runtime_error("MacroManager: failed to create macros table");
    }
}

std::vector<Macro> MacroManager::getAll() const {
    auto stmt = m_db->prepare(
        "SELECT id, name, gcode, shortcut, sort_order, built_in "
        "FROM macros ORDER BY sort_order ASC, id ASC");

    std::vector<Macro> result;
    while (stmt.step()) {
        Macro m;
        m.id = static_cast<int>(stmt.getInt(0));
        m.name = stmt.getText(1);
        m.gcode = stmt.getText(2);
        m.shortcut = stmt.getText(3);
        m.sortOrder = static_cast<int>(stmt.getInt(4));
        m.builtIn = stmt.getInt(5) != 0;
        result.push_back(std::move(m));
    }
    return result;
}

Macro MacroManager::getById(int id) const {
    auto stmt = m_db->prepare(
        "SELECT id, name, gcode, shortcut, sort_order, built_in "
        "FROM macros WHERE id = ?");
    (void)stmt.bindInt(1, id);

    if (!stmt.step()) {
        throw std::runtime_error("MacroManager: macro not found with id " + std::to_string(id));
    }

    Macro m;
    m.id = static_cast<int>(stmt.getInt(0));
    m.name = stmt.getText(1);
    m.gcode = stmt.getText(2);
    m.shortcut = stmt.getText(3);
    m.sortOrder = static_cast<int>(stmt.getInt(4));
    m.builtIn = stmt.getInt(5) != 0;
    return m;
}

int MacroManager::addMacro(const Macro& macro) {
    auto stmt = m_db->prepare(
        "INSERT INTO macros (name, gcode, shortcut, sort_order, built_in) "
        "VALUES (?, ?, ?, ?, ?)");
    (void)stmt.bindText(1, macro.name);
    (void)stmt.bindText(2, macro.gcode);
    (void)stmt.bindText(3, macro.shortcut);
    (void)stmt.bindInt(4, macro.sortOrder);
    (void)stmt.bindInt(5, macro.builtIn ? 1 : 0);

    if (!stmt.execute()) {
        throw std::runtime_error("MacroManager: failed to insert macro");
    }
    return static_cast<int>(m_db->lastInsertId());
}

void MacroManager::updateMacro(const Macro& macro) {
    auto stmt = m_db->prepare(
        "UPDATE macros SET name = ?, gcode = ?, shortcut = ?, sort_order = ? "
        "WHERE id = ?");
    (void)stmt.bindText(1, macro.name);
    (void)stmt.bindText(2, macro.gcode);
    (void)stmt.bindText(3, macro.shortcut);
    (void)stmt.bindInt(4, macro.sortOrder);
    (void)stmt.bindInt(5, macro.id);

    if (!stmt.execute()) {
        throw std::runtime_error("MacroManager: failed to update macro");
    }
}

void MacroManager::deleteMacro(int id) {
    // Check if built-in
    auto check = m_db->prepare("SELECT built_in FROM macros WHERE id = ?");
    (void)check.bindInt(1, id);
    if (check.step() && check.getInt(0) != 0) {
        throw std::runtime_error("MacroManager: cannot delete built-in macro");
    }

    auto stmt = m_db->prepare("DELETE FROM macros WHERE id = ?");
    (void)stmt.bindInt(1, id);
    if (!stmt.execute()) {
        throw std::runtime_error("MacroManager: failed to delete macro");
    }
}

void MacroManager::reorder(const std::vector<int>& ids) {
    if (!m_db->beginTransaction()) return;

    auto stmt = m_db->prepare("UPDATE macros SET sort_order = ? WHERE id = ?");
    for (size_t i = 0; i < ids.size(); ++i) {
        stmt.reset();
        (void)stmt.bindInt(1, static_cast<int>(i));
        (void)stmt.bindInt(2, ids[i]);
        (void)stmt.execute();
    }

    (void)m_db->commit();
}

std::vector<std::string> MacroManager::parseLines(const Macro& macro) const {
    std::vector<std::string> lines;
    std::istringstream stream(macro.gcode);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue; // skip empty lines
        size_t end = line.find_last_not_of(" \t\r");
        line = line.substr(start, end - start + 1);

        // Skip empty lines and comment-only lines
        if (line.empty()) continue;
        if (line[0] == ';') continue;
        if (line[0] == '(') continue; // G-code comment in parentheses

        lines.push_back(line);
    }
    return lines;
}

void MacroManager::ensureBuiltIns() {
    // Check if built-ins already exist
    auto stmt = m_db->prepare("SELECT COUNT(*) FROM macros WHERE built_in = 1");
    if (stmt.step() && stmt.getInt(0) > 0) {
        return; // Already initialized
    }

    // Built-in 1: Homing Cycle
    {
        Macro m;
        m.name = "Homing Cycle";
        m.gcode = "$H";
        m.sortOrder = 0;
        m.builtIn = true;
        addMacro(m);
    }

    // Built-in 2: Probe Z (Touch Plate)
    {
        Macro m;
        m.name = "Probe Z (Touch Plate)";
        m.gcode = "G91\nG38.2 Z-50 F100\nG90";
        m.sortOrder = 1;
        m.builtIn = true;
        addMacro(m);
    }

    // Built-in 3: Return to Zero
    {
        Macro m;
        m.name = "Return to Zero";
        m.gcode = "G90\nG53 G0 Z0\nG53 G0 X0 Y0";
        m.sortOrder = 2;
        m.builtIn = true;
        addMacro(m);
    }
}

} // namespace dw
