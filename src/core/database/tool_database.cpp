#include "tool_database.h"

#include "../utils/log.h"
#include "../utils/uuid.h"

namespace dw {

bool ToolDatabase::open(const Path& path) {
    if (!m_db.open(path.string())) {
        log::errorf("ToolDatabase", "Failed to open: %s", path.string().c_str());
        return false;
    }

    // Check if schema exists (version table present)
    auto check = m_db.prepare(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='version'");
    bool hasSchema = check.step();

    if (!hasSchema) {
        if (!initializeSchema(m_db)) {
            log::error("ToolDatabase", "Failed to initialize schema");
            m_db.close();
            return false;
        }
    }

    log::infof("ToolDatabase", "Opened: %s", path.string().c_str());
    return true;
}

bool ToolDatabase::initializeSchema(Database& db) {
    if (!db.isOpen())
        return false;

    Transaction txn(db);

    // Exact DDL from real Vectric .vtdb files (verified with sqlite_master)

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "version" (
            "version" INTEGER NOT NULL UNIQUE,
            PRIMARY KEY("version")
        )
    )"))
        return false;

    if (!db.execute("INSERT OR IGNORE INTO version (version) VALUES (1)"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "migration" (
            "version"    INTEGER NOT NULL,
            "subversion" INTEGER NOT NULL,
            "name"       TEXT NOT NULL,
            "checksum"   TEXT NOT NULL,
            PRIMARY KEY("version","subversion")
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "material" (
            "id"   TEXT NOT NULL UNIQUE PRIMARY KEY,
            "name" TEXT NOT NULL UNIQUE
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "machine" (
            "id"                  TEXT NOT NULL UNIQUE PRIMARY KEY,
            "name"                TEXT NOT NULL UNIQUE,
            "make"                TEXT,
            "model"               TEXT,
            "controller_type"     TEXT,
            "dimensions_units"    INTEGER,
            "max_width"           REAL,
            "max_height"          REAL,
            "support_rotary"      INTEGER,
            "support_tool_change" INTEGER,
            "has_laser_head"      INTEGER,
            "spindle_power_watts" REAL DEFAULT 0,
            "max_rpm"             INTEGER DEFAULT 24000,
            "drive_type"          INTEGER DEFAULT 0
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "tool_geometry" (
            "id"                TEXT NOT NULL UNIQUE PRIMARY KEY,
            "name_format"       TEXT NOT NULL,
            "notes"             TEXT,
            "tool_type"         INTEGER NOT NULL,
            "units"             INTEGER NOT NULL,
            "diameter"          REAL,
            "included_angle"    REAL,
            "flat_diameter"     REAL,
            "num_flutes"        INTEGER,
            "flute_length"      REAL,
            "thread_pitch"      REAL,
            "outline"           BLOB,
            "tip_radius"        REAL,
            "laser_watt"        INTEGER,
            "custom_attributes" TEXT,
            "tooth_size"        REAL,
            "tooth_offset"      REAL,
            "neck_length"       REAL,
            "tooth_height"      REAL,
            "threaded_length"   REAL
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "tool_cutting_data" (
            "id"              TEXT NOT NULL UNIQUE PRIMARY KEY,
            "rate_units"      INTEGER NOT NULL,
            "feed_rate"       REAL,
            "plunge_rate"     REAL,
            "spindle_speed"   INTEGER,
            "spindle_dir"     INTEGER,
            "stepdown"        REAL,
            "stepover"        REAL,
            "clear_stepover"  REAL,
            "thread_depth"    REAL,
            "thread_step_in"  REAL,
            "laser_power"     REAL,
            "laser_passes"    INTEGER,
            "laser_burn_rate" REAL,
            "line_width"      REAL,
            "length_units"    INTEGER NOT NULL DEFAULT 0,
            "tool_number"     INTEGER,
            "laser_kerf"      INTEGER,
            "notes"           TEXT
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "tool_entity" (
            "id"                   TEXT NOT NULL UNIQUE,
            "material_id"          TEXT,
            "machine_id"           TEXT,
            "tool_geometry_id"     TEXT,
            "tool_cutting_data_id" TEXT NOT NULL,
            PRIMARY KEY("tool_geometry_id","material_id","machine_id"),
            FOREIGN KEY("material_id")          REFERENCES "material"("id"),
            FOREIGN KEY("machine_id")           REFERENCES "machine"("id"),
            FOREIGN KEY("tool_geometry_id")     REFERENCES "tool_geometry"("id"),
            FOREIGN KEY("tool_cutting_data_id") REFERENCES "tool_cutting_data"("id")
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "tool_tree_entry" (
            "id"               TEXT NOT NULL UNIQUE,
            "parent_group_id"  TEXT,
            "sibling_order"    INTEGER NOT NULL,
            "tool_geometry_id" TEXT UNIQUE,
            "name"             TEXT,
            "notes"            TEXT,
            "expanded"         INTEGER,
            PRIMARY KEY("id","parent_group_id","sibling_order"),
            FOREIGN KEY("tool_geometry_id") REFERENCES "tool_geometry"("id"),
            FOREIGN KEY("parent_group_id")  REFERENCES "tool_tree_entry"("id")
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "tool_name_format" (
            "id"        TEXT NOT NULL,
            "tool_type" INTEGER NOT NULL UNIQUE,
            "format"    TEXT NOT NULL,
            PRIMARY KEY("id","tool_type")
        )
    )"))
        return false;

    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS "upload_data" (
            "id"            INTEGER NOT NULL UNIQUE PRIMARY KEY,
            "date_uploaded" INTEGER NOT NULL
        )
    )"))
        return false;

    // Migrate older databases missing extended machine columns.
    // ALTER TABLE ... ADD COLUMN is a no-op error when the column already exists;
    // we silently ignore failures here so both fresh and legacy DBs work.
    (void)db.execute("ALTER TABLE machine ADD COLUMN spindle_power_watts REAL DEFAULT 0");
    (void)db.execute("ALTER TABLE machine ADD COLUMN max_rpm INTEGER DEFAULT 24000");
    (void)db.execute("ALTER TABLE machine ADD COLUMN drive_type INTEGER DEFAULT 0");

    if (!txn.commit()) {
        log::error("ToolDatabase", "Failed to commit schema");
        return false;
    }

    return true;
}

// --- Machine CRUD ---

bool ToolDatabase::insertMachine(const VtdbMachine& m) {
    std::string id = m.id.empty() ? uuid::generate() : m.id;
    auto stmt = m_db.prepare(R"(
        INSERT OR IGNORE INTO machine
            (id, name, make, model, controller_type, dimensions_units,
             max_width, max_height, support_rotary, support_tool_change, has_laser_head,
             spindle_power_watts, max_rpm, drive_type)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    (void)stmt.bindText(2, m.name);
    (void)stmt.bindText(3, m.make);
    (void)stmt.bindText(4, m.model);
    (void)stmt.bindText(5, m.controller_type);
    (void)stmt.bindInt(6, m.dimensions_units);
    (void)stmt.bindDouble(7, m.max_width);
    (void)stmt.bindDouble(8, m.max_height);
    (void)stmt.bindInt(9, m.support_rotary);
    (void)stmt.bindInt(10, m.support_tool_change);
    (void)stmt.bindInt(11, m.has_laser_head);
    (void)stmt.bindDouble(12, m.spindle_power_watts);
    (void)stmt.bindInt(13, m.max_rpm);
    (void)stmt.bindInt(14, static_cast<int>(m.drive_type));
    return stmt.execute();
}

bool ToolDatabase::updateMachine(const VtdbMachine& m) {
    auto stmt = m_db.prepare(R"(
        UPDATE machine SET
            name=?, make=?, model=?, controller_type=?, dimensions_units=?,
            max_width=?, max_height=?, support_rotary=?, support_tool_change=?,
            has_laser_head=?, spindle_power_watts=?, max_rpm=?, drive_type=?
        WHERE id=?
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, m.name);
    (void)stmt.bindText(2, m.make);
    (void)stmt.bindText(3, m.model);
    (void)stmt.bindText(4, m.controller_type);
    (void)stmt.bindInt(5, m.dimensions_units);
    (void)stmt.bindDouble(6, m.max_width);
    (void)stmt.bindDouble(7, m.max_height);
    (void)stmt.bindInt(8, m.support_rotary);
    (void)stmt.bindInt(9, m.support_tool_change);
    (void)stmt.bindInt(10, m.has_laser_head);
    (void)stmt.bindDouble(11, m.spindle_power_watts);
    (void)stmt.bindInt(12, m.max_rpm);
    (void)stmt.bindInt(13, static_cast<int>(m.drive_type));
    (void)stmt.bindText(14, m.id);
    return stmt.execute();
}

static VtdbMachine rowToMachine(Statement& stmt) {
    VtdbMachine m;
    m.id = stmt.getText(0);
    m.name = stmt.getText(1);
    m.make = stmt.getText(2);
    m.model = stmt.getText(3);
    m.controller_type = stmt.getText(4);
    m.dimensions_units = static_cast<int>(stmt.getInt(5));
    m.max_width = stmt.getDouble(6);
    m.max_height = stmt.getDouble(7);
    m.support_rotary = static_cast<int>(stmt.getInt(8));
    m.support_tool_change = static_cast<int>(stmt.getInt(9));
    m.has_laser_head = static_cast<int>(stmt.getInt(10));
    m.spindle_power_watts = stmt.getDouble(11);
    m.max_rpm = static_cast<int>(stmt.getInt(12));
    m.drive_type = static_cast<DriveType>(stmt.getInt(13));
    return m;
}

static constexpr const char* kMachineSelect =
    "SELECT id, name, make, model, controller_type, dimensions_units, "
    "max_width, max_height, support_rotary, support_tool_change, has_laser_head, "
    "spindle_power_watts, max_rpm, drive_type FROM machine";

std::vector<VtdbMachine> ToolDatabase::findAllMachines() {
    std::vector<VtdbMachine> result;
    auto stmt = m_db.prepare(std::string(kMachineSelect) + " ORDER BY name");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        result.push_back(rowToMachine(stmt));
    }
    return result;
}

std::optional<VtdbMachine> ToolDatabase::findMachineById(const std::string& id) {
    auto stmt = m_db.prepare(std::string(kMachineSelect) + " WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id) || !stmt.step())
        return std::nullopt;
    return rowToMachine(stmt);
}

// --- Material CRUD ---

bool ToolDatabase::insertMaterial(const VtdbMaterial& m) {
    std::string id = m.id.empty() ? uuid::generate() : m.id;
    auto stmt = m_db.prepare("INSERT OR IGNORE INTO material (id, name) VALUES (?, ?)");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    (void)stmt.bindText(2, m.name);
    return stmt.execute();
}

std::vector<VtdbMaterial> ToolDatabase::findAllMaterials() {
    std::vector<VtdbMaterial> result;
    auto stmt = m_db.prepare("SELECT id, name FROM material ORDER BY name");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        VtdbMaterial m;
        m.id = stmt.getText(0);
        m.name = stmt.getText(1);
        result.push_back(m);
    }
    return result;
}

std::optional<VtdbMaterial> ToolDatabase::findMaterialById(const std::string& id) {
    auto stmt = m_db.prepare("SELECT id, name FROM material WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id) || !stmt.step())
        return std::nullopt;
    VtdbMaterial m;
    m.id = stmt.getText(0);
    m.name = stmt.getText(1);
    return m;
}

std::optional<VtdbMaterial> ToolDatabase::findMaterialByName(const std::string& name) {
    auto stmt = m_db.prepare("SELECT id, name FROM material WHERE name = ?");
    if (!stmt.isValid() || !stmt.bindText(1, name) || !stmt.step())
        return std::nullopt;
    VtdbMaterial m;
    m.id = stmt.getText(0);
    m.name = stmt.getText(1);
    return m;
}

// --- Tool Geometry CRUD ---

bool ToolDatabase::insertGeometry(const VtdbToolGeometry& g) {
    std::string id = g.id.empty() ? uuid::generate() : g.id;
    auto stmt = m_db.prepare(R"(
        INSERT OR IGNORE INTO tool_geometry
            (id, name_format, notes, tool_type, units, diameter, included_angle,
             flat_diameter, num_flutes, flute_length, thread_pitch, outline,
             tip_radius, laser_watt, custom_attributes, tooth_size, tooth_offset,
             neck_length, tooth_height, threaded_length)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    (void)stmt.bindText(2, g.name_format);
    (void)stmt.bindText(3, g.notes);
    (void)stmt.bindInt(4, static_cast<int>(g.tool_type));
    (void)stmt.bindInt(5, static_cast<int>(g.units));
    (void)stmt.bindDouble(6, g.diameter);
    (void)stmt.bindDouble(7, g.included_angle);
    (void)stmt.bindDouble(8, g.flat_diameter);
    (void)stmt.bindInt(9, g.num_flutes);
    (void)stmt.bindDouble(10, g.flute_length);
    (void)stmt.bindDouble(11, g.thread_pitch);
    if (!g.outline.empty())
        (void)stmt.bindBlob(12, g.outline.data(), static_cast<int>(g.outline.size()));
    else
        (void)stmt.bindNull(12);
    (void)stmt.bindDouble(13, g.tip_radius);
    (void)stmt.bindInt(14, g.laser_watt);
    (void)stmt.bindText(15, g.custom_attributes);
    (void)stmt.bindDouble(16, g.tooth_size);
    (void)stmt.bindDouble(17, g.tooth_offset);
    (void)stmt.bindDouble(18, g.neck_length);
    (void)stmt.bindDouble(19, g.tooth_height);
    (void)stmt.bindDouble(20, g.threaded_length);
    return stmt.execute();
}

std::optional<VtdbToolGeometry> ToolDatabase::findGeometryById(const std::string& id) {
    auto stmt = m_db.prepare(
        "SELECT id, name_format, notes, tool_type, units, diameter, included_angle, "
        "flat_diameter, num_flutes, flute_length, thread_pitch, outline, tip_radius, "
        "laser_watt, custom_attributes, tooth_size, tooth_offset, neck_length, "
        "tooth_height, threaded_length FROM tool_geometry WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id) || !stmt.step())
        return std::nullopt;
    VtdbToolGeometry g;
    g.id = stmt.getText(0);
    g.name_format = stmt.getText(1);
    g.notes = stmt.getText(2);
    g.tool_type = static_cast<VtdbToolType>(stmt.getInt(3));
    g.units = static_cast<VtdbUnits>(stmt.getInt(4));
    g.diameter = stmt.getDouble(5);
    g.included_angle = stmt.getDouble(6);
    g.flat_diameter = stmt.getDouble(7);
    g.num_flutes = static_cast<int>(stmt.getInt(8));
    g.flute_length = stmt.getDouble(9);
    g.thread_pitch = stmt.getDouble(10);
    g.outline = stmt.getBlob(11);
    g.tip_radius = stmt.getDouble(12);
    g.laser_watt = static_cast<int>(stmt.getInt(13));
    g.custom_attributes = stmt.getText(14);
    g.tooth_size = stmt.getDouble(15);
    g.tooth_offset = stmt.getDouble(16);
    g.neck_length = stmt.getDouble(17);
    g.tooth_height = stmt.getDouble(18);
    g.threaded_length = stmt.getDouble(19);
    return g;
}

std::vector<VtdbToolGeometry> ToolDatabase::findAllGeometries() {
    std::vector<VtdbToolGeometry> result;
    auto stmt = m_db.prepare(
        "SELECT id, name_format, notes, tool_type, units, diameter, included_angle, "
        "flat_diameter, num_flutes, flute_length, thread_pitch, outline, tip_radius, "
        "laser_watt, custom_attributes, tooth_size, tooth_offset, neck_length, "
        "tooth_height, threaded_length FROM tool_geometry ORDER BY name_format");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        VtdbToolGeometry g;
        g.id = stmt.getText(0);
        g.name_format = stmt.getText(1);
        g.notes = stmt.getText(2);
        g.tool_type = static_cast<VtdbToolType>(stmt.getInt(3));
        g.units = static_cast<VtdbUnits>(stmt.getInt(4));
        g.diameter = stmt.getDouble(5);
        g.included_angle = stmt.getDouble(6);
        g.flat_diameter = stmt.getDouble(7);
        g.num_flutes = static_cast<int>(stmt.getInt(8));
        g.flute_length = stmt.getDouble(9);
        g.thread_pitch = stmt.getDouble(10);
        g.outline = stmt.getBlob(11);
        g.tip_radius = stmt.getDouble(12);
        g.laser_watt = static_cast<int>(stmt.getInt(13));
        g.custom_attributes = stmt.getText(14);
        g.tooth_size = stmt.getDouble(15);
        g.tooth_offset = stmt.getDouble(16);
        g.neck_length = stmt.getDouble(17);
        g.tooth_height = stmt.getDouble(18);
        g.threaded_length = stmt.getDouble(19);
        result.push_back(g);
    }
    return result;
}

bool ToolDatabase::updateGeometry(const VtdbToolGeometry& g) {
    auto stmt = m_db.prepare(R"(
        UPDATE tool_geometry SET
            name_format=?, notes=?, tool_type=?, units=?, diameter=?,
            included_angle=?, flat_diameter=?, num_flutes=?, flute_length=?,
            thread_pitch=?, outline=?, tip_radius=?, laser_watt=?,
            custom_attributes=?, tooth_size=?, tooth_offset=?, neck_length=?,
            tooth_height=?, threaded_length=?
        WHERE id=?
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, g.name_format);
    (void)stmt.bindText(2, g.notes);
    (void)stmt.bindInt(3, static_cast<int>(g.tool_type));
    (void)stmt.bindInt(4, static_cast<int>(g.units));
    (void)stmt.bindDouble(5, g.diameter);
    (void)stmt.bindDouble(6, g.included_angle);
    (void)stmt.bindDouble(7, g.flat_diameter);
    (void)stmt.bindInt(8, g.num_flutes);
    (void)stmt.bindDouble(9, g.flute_length);
    (void)stmt.bindDouble(10, g.thread_pitch);
    if (!g.outline.empty())
        (void)stmt.bindBlob(11, g.outline.data(), static_cast<int>(g.outline.size()));
    else
        (void)stmt.bindNull(11);
    (void)stmt.bindDouble(12, g.tip_radius);
    (void)stmt.bindInt(13, g.laser_watt);
    (void)stmt.bindText(14, g.custom_attributes);
    (void)stmt.bindDouble(15, g.tooth_size);
    (void)stmt.bindDouble(16, g.tooth_offset);
    (void)stmt.bindDouble(17, g.neck_length);
    (void)stmt.bindDouble(18, g.tooth_height);
    (void)stmt.bindDouble(19, g.threaded_length);
    (void)stmt.bindText(20, g.id);
    return stmt.execute();
}

bool ToolDatabase::removeGeometry(const std::string& id) {
    auto stmt = m_db.prepare("DELETE FROM tool_geometry WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id)) return false;
    return stmt.execute();
}

// --- Cutting Data CRUD ---

bool ToolDatabase::insertCuttingData(const VtdbCuttingData& cd) {
    std::string id = cd.id.empty() ? uuid::generate() : cd.id;
    auto stmt = m_db.prepare(R"(
        INSERT OR IGNORE INTO tool_cutting_data
            (id, rate_units, feed_rate, plunge_rate, spindle_speed, spindle_dir,
             stepdown, stepover, clear_stepover, thread_depth, thread_step_in,
             laser_power, laser_passes, laser_burn_rate, line_width, length_units,
             tool_number, laser_kerf, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    (void)stmt.bindInt(2, cd.rate_units);
    (void)stmt.bindDouble(3, cd.feed_rate);
    (void)stmt.bindDouble(4, cd.plunge_rate);
    (void)stmt.bindInt(5, cd.spindle_speed);
    (void)stmt.bindInt(6, cd.spindle_dir);
    (void)stmt.bindDouble(7, cd.stepdown);
    (void)stmt.bindDouble(8, cd.stepover);
    (void)stmt.bindDouble(9, cd.clear_stepover);
    (void)stmt.bindDouble(10, cd.thread_depth);
    (void)stmt.bindDouble(11, cd.thread_step_in);
    (void)stmt.bindDouble(12, cd.laser_power);
    (void)stmt.bindInt(13, cd.laser_passes);
    (void)stmt.bindDouble(14, cd.laser_burn_rate);
    (void)stmt.bindDouble(15, cd.line_width);
    (void)stmt.bindInt(16, cd.length_units);
    (void)stmt.bindInt(17, cd.tool_number);
    (void)stmt.bindInt(18, cd.laser_kerf);
    (void)stmt.bindText(19, cd.notes);
    return stmt.execute();
}

std::optional<VtdbCuttingData> ToolDatabase::findCuttingDataById(const std::string& id) {
    auto stmt = m_db.prepare(
        "SELECT id, rate_units, feed_rate, plunge_rate, spindle_speed, spindle_dir, "
        "stepdown, stepover, clear_stepover, thread_depth, thread_step_in, "
        "laser_power, laser_passes, laser_burn_rate, line_width, length_units, "
        "tool_number, laser_kerf, notes FROM tool_cutting_data WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id) || !stmt.step())
        return std::nullopt;
    VtdbCuttingData cd;
    cd.id = stmt.getText(0);
    cd.rate_units = static_cast<int>(stmt.getInt(1));
    cd.feed_rate = stmt.getDouble(2);
    cd.plunge_rate = stmt.getDouble(3);
    cd.spindle_speed = static_cast<int>(stmt.getInt(4));
    cd.spindle_dir = static_cast<int>(stmt.getInt(5));
    cd.stepdown = stmt.getDouble(6);
    cd.stepover = stmt.getDouble(7);
    cd.clear_stepover = stmt.getDouble(8);
    cd.thread_depth = stmt.getDouble(9);
    cd.thread_step_in = stmt.getDouble(10);
    cd.laser_power = stmt.getDouble(11);
    cd.laser_passes = static_cast<int>(stmt.getInt(12));
    cd.laser_burn_rate = stmt.getDouble(13);
    cd.line_width = stmt.getDouble(14);
    cd.length_units = static_cast<int>(stmt.getInt(15));
    cd.tool_number = static_cast<int>(stmt.getInt(16));
    cd.laser_kerf = static_cast<int>(stmt.getInt(17));
    cd.notes = stmt.getText(18);
    return cd;
}

bool ToolDatabase::updateCuttingData(const VtdbCuttingData& cd) {
    auto stmt = m_db.prepare(R"(
        UPDATE tool_cutting_data SET
            rate_units=?, feed_rate=?, plunge_rate=?, spindle_speed=?, spindle_dir=?,
            stepdown=?, stepover=?, clear_stepover=?, thread_depth=?, thread_step_in=?,
            laser_power=?, laser_passes=?, laser_burn_rate=?, line_width=?, length_units=?,
            tool_number=?, laser_kerf=?, notes=?
        WHERE id=?
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindInt(1, cd.rate_units);
    (void)stmt.bindDouble(2, cd.feed_rate);
    (void)stmt.bindDouble(3, cd.plunge_rate);
    (void)stmt.bindInt(4, cd.spindle_speed);
    (void)stmt.bindInt(5, cd.spindle_dir);
    (void)stmt.bindDouble(6, cd.stepdown);
    (void)stmt.bindDouble(7, cd.stepover);
    (void)stmt.bindDouble(8, cd.clear_stepover);
    (void)stmt.bindDouble(9, cd.thread_depth);
    (void)stmt.bindDouble(10, cd.thread_step_in);
    (void)stmt.bindDouble(11, cd.laser_power);
    (void)stmt.bindInt(12, cd.laser_passes);
    (void)stmt.bindDouble(13, cd.laser_burn_rate);
    (void)stmt.bindDouble(14, cd.line_width);
    (void)stmt.bindInt(15, cd.length_units);
    (void)stmt.bindInt(16, cd.tool_number);
    (void)stmt.bindInt(17, cd.laser_kerf);
    (void)stmt.bindText(18, cd.notes);
    (void)stmt.bindText(19, cd.id);
    return stmt.execute();
}

bool ToolDatabase::removeCuttingData(const std::string& id) {
    auto stmt = m_db.prepare("DELETE FROM tool_cutting_data WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id)) return false;
    return stmt.execute();
}

// --- Tool Entity ---

bool ToolDatabase::insertEntity(const VtdbToolEntity& e) {
    std::string id = e.id.empty() ? uuid::generate() : e.id;
    auto stmt = m_db.prepare(R"(
        INSERT OR IGNORE INTO tool_entity
            (id, material_id, machine_id, tool_geometry_id, tool_cutting_data_id)
        VALUES (?, ?, ?, ?, ?)
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    if (e.material_id.empty())
        (void)stmt.bindNull(2);
    else
        (void)stmt.bindText(2, e.material_id);
    (void)stmt.bindText(3, e.machine_id);
    (void)stmt.bindText(4, e.tool_geometry_id);
    (void)stmt.bindText(5, e.tool_cutting_data_id);
    return stmt.execute();
}

std::vector<VtdbToolEntity> ToolDatabase::findEntitiesForGeometry(const std::string& geomId) {
    std::vector<VtdbToolEntity> result;
    auto stmt = m_db.prepare(
        "SELECT id, material_id, machine_id, tool_geometry_id, tool_cutting_data_id "
        "FROM tool_entity WHERE tool_geometry_id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, geomId)) return result;
    while (stmt.step()) {
        VtdbToolEntity e;
        e.id = stmt.getText(0);
        e.material_id = stmt.isNull(1) ? "" : stmt.getText(1);
        e.machine_id = stmt.getText(2);
        e.tool_geometry_id = stmt.getText(3);
        e.tool_cutting_data_id = stmt.getText(4);
        result.push_back(e);
    }
    return result;
}

std::vector<VtdbToolEntity> ToolDatabase::findEntitiesForMaterial(const std::string& materialId) {
    std::vector<VtdbToolEntity> result;
    auto stmt = m_db.prepare(
        "SELECT id, material_id, machine_id, tool_geometry_id, tool_cutting_data_id "
        "FROM tool_entity WHERE material_id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, materialId)) return result;
    while (stmt.step()) {
        VtdbToolEntity e;
        e.id = stmt.getText(0);
        e.material_id = stmt.isNull(1) ? "" : stmt.getText(1);
        e.machine_id = stmt.getText(2);
        e.tool_geometry_id = stmt.getText(3);
        e.tool_cutting_data_id = stmt.getText(4);
        result.push_back(e);
    }
    return result;
}

bool ToolDatabase::removeEntity(const std::string& id) {
    auto stmt = m_db.prepare("DELETE FROM tool_entity WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id)) return false;
    return stmt.execute();
}

// --- Tree Entry ---

bool ToolDatabase::insertTreeEntry(const VtdbTreeEntry& te) {
    std::string id = te.id.empty() ? uuid::generate() : te.id;
    auto stmt = m_db.prepare(R"(
        INSERT OR IGNORE INTO tool_tree_entry
            (id, parent_group_id, sibling_order, tool_geometry_id, name, notes, expanded)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    if (!stmt.isValid()) return false;
    (void)stmt.bindText(1, id);
    if (te.parent_group_id.empty())
        (void)stmt.bindNull(2);
    else
        (void)stmt.bindText(2, te.parent_group_id);
    (void)stmt.bindInt(3, te.sibling_order);
    if (te.tool_geometry_id.empty())
        (void)stmt.bindNull(4);
    else
        (void)stmt.bindText(4, te.tool_geometry_id);
    if (te.name.empty())
        (void)stmt.bindNull(5);
    else
        (void)stmt.bindText(5, te.name);
    (void)stmt.bindText(6, te.notes);
    (void)stmt.bindInt(7, te.expanded);
    return stmt.execute();
}

std::vector<VtdbTreeEntry> ToolDatabase::findChildrenOf(const std::string& parentId) {
    std::vector<VtdbTreeEntry> result;
    auto stmt = m_db.prepare(
        "SELECT id, parent_group_id, sibling_order, tool_geometry_id, name, notes, expanded "
        "FROM tool_tree_entry WHERE parent_group_id = ? ORDER BY sibling_order");
    if (!stmt.isValid() || !stmt.bindText(1, parentId)) return result;
    while (stmt.step()) {
        VtdbTreeEntry te;
        te.id = stmt.getText(0);
        te.parent_group_id = stmt.isNull(1) ? "" : stmt.getText(1);
        te.sibling_order = static_cast<int>(stmt.getInt(2));
        te.tool_geometry_id = stmt.isNull(3) ? "" : stmt.getText(3);
        te.name = stmt.isNull(4) ? "" : stmt.getText(4);
        te.notes = stmt.getText(5);
        te.expanded = static_cast<int>(stmt.getInt(6));
        result.push_back(te);
    }
    return result;
}

std::vector<VtdbTreeEntry> ToolDatabase::findRootEntries() {
    std::vector<VtdbTreeEntry> result;
    auto stmt = m_db.prepare(
        "SELECT id, parent_group_id, sibling_order, tool_geometry_id, name, notes, expanded "
        "FROM tool_tree_entry WHERE parent_group_id IS NULL ORDER BY sibling_order");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        VtdbTreeEntry te;
        te.id = stmt.getText(0);
        te.parent_group_id = "";
        te.sibling_order = static_cast<int>(stmt.getInt(2));
        te.tool_geometry_id = stmt.isNull(3) ? "" : stmt.getText(3);
        te.name = stmt.isNull(4) ? "" : stmt.getText(4);
        te.notes = stmt.getText(5);
        te.expanded = static_cast<int>(stmt.getInt(6));
        result.push_back(te);
    }
    return result;
}

std::vector<VtdbTreeEntry> ToolDatabase::getAllTreeEntries() {
    std::vector<VtdbTreeEntry> result;
    auto stmt = m_db.prepare(
        "SELECT id, parent_group_id, sibling_order, tool_geometry_id, name, notes, expanded "
        "FROM tool_tree_entry ORDER BY sibling_order");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        VtdbTreeEntry te;
        te.id = stmt.getText(0);
        te.parent_group_id = stmt.isNull(1) ? "" : stmt.getText(1);
        te.sibling_order = static_cast<int>(stmt.getInt(2));
        te.tool_geometry_id = stmt.isNull(3) ? "" : stmt.getText(3);
        te.name = stmt.isNull(4) ? "" : stmt.getText(4);
        te.notes = stmt.getText(5);
        te.expanded = static_cast<int>(stmt.getInt(6));
        result.push_back(te);
    }
    return result;
}

bool ToolDatabase::updateTreeEntry(const VtdbTreeEntry& te) {
    auto stmt = m_db.prepare(R"(
        UPDATE tool_tree_entry SET
            parent_group_id=?, sibling_order=?, tool_geometry_id=?, name=?, notes=?, expanded=?
        WHERE id=?
    )");
    if (!stmt.isValid()) return false;
    if (te.parent_group_id.empty())
        (void)stmt.bindNull(1);
    else
        (void)stmt.bindText(1, te.parent_group_id);
    (void)stmt.bindInt(2, te.sibling_order);
    if (te.tool_geometry_id.empty())
        (void)stmt.bindNull(3);
    else
        (void)stmt.bindText(3, te.tool_geometry_id);
    if (te.name.empty())
        (void)stmt.bindNull(4);
    else
        (void)stmt.bindText(4, te.name);
    (void)stmt.bindText(5, te.notes);
    (void)stmt.bindInt(6, te.expanded);
    (void)stmt.bindText(7, te.id);
    return stmt.execute();
}

bool ToolDatabase::removeTreeEntry(const std::string& id) {
    auto stmt = m_db.prepare("DELETE FROM tool_tree_entry WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindText(1, id)) return false;
    return stmt.execute();
}

// --- Name Format ---

std::vector<ToolDatabase::NameFormat> ToolDatabase::findAllNameFormats() {
    std::vector<NameFormat> result;
    auto stmt = m_db.prepare("SELECT id, tool_type, format FROM tool_name_format");
    if (!stmt.isValid()) return result;
    while (stmt.step()) {
        NameFormat nf;
        nf.id = stmt.getText(0);
        nf.tool_type = static_cast<int>(stmt.getInt(1));
        nf.format = stmt.getText(2);
        result.push_back(nf);
    }
    return result;
}

// --- High-level ---

std::optional<VtdbToolView> ToolDatabase::getToolView(const std::string& geomId,
                                                       const std::string& materialId,
                                                       const std::string& machineId) {
    auto geom = findGeometryById(geomId);
    if (!geom) return std::nullopt;

    // Find entity for this combination, preferring specific material over NULL
    auto stmt = m_db.prepare(
        "SELECT id, material_id, machine_id, tool_geometry_id, tool_cutting_data_id "
        "FROM tool_entity WHERE tool_geometry_id = ? AND "
        "(material_id = ? OR material_id IS NULL) AND machine_id = ? "
        "ORDER BY material_id DESC LIMIT 1");
    if (!stmt.isValid()) return std::nullopt;
    (void)stmt.bindText(1, geomId);
    (void)stmt.bindText(2, materialId);
    (void)stmt.bindText(3, machineId);

    if (!stmt.step()) return std::nullopt;

    std::string cdId = stmt.getText(4);
    auto cd = findCuttingDataById(cdId);
    if (!cd) return std::nullopt;

    VtdbToolView view;
    view.geometry = *geom;
    view.cutting_data = *cd;

    auto mat = findMaterialById(materialId);
    if (mat) view.material = *mat;

    auto mach = findMachineById(machineId);
    if (mach) view.machine = *mach;

    return view;
}

int ToolDatabase::importFromVtdb(const Path& externalPath) {
    Database extDb;
    if (!extDb.open(externalPath.string())) {
        log::errorf("ToolDatabase", "Cannot open external vtdb: %s", externalPath.string().c_str());
        return -1;
    }

    Transaction txn(m_db);
    int importedGeometries = 0;

    // Import in dependency order: material -> machine -> geometry -> cutting_data -> entity -> tree

    // 1. Materials
    {
        auto s = extDb.prepare("SELECT id, name FROM material");
        while (s.step()) {
            VtdbMaterial m;
            m.id = s.getText(0);
            m.name = s.getText(1);
            insertMaterial(m);
        }
    }

    // 2. Machines
    {
        // Check if external DB has our extended columns
        auto colCheck = extDb.prepare(
            "SELECT COUNT(*) FROM pragma_table_info('machine') WHERE name='spindle_power_watts'");
        bool hasExtended = colCheck.step() && colCheck.getInt(0) > 0;

        std::string machQuery =
            "SELECT id, name, make, model, controller_type, dimensions_units, "
            "max_width, max_height, support_rotary, support_tool_change, has_laser_head";
        if (hasExtended)
            machQuery += ", spindle_power_watts, max_rpm, drive_type";
        machQuery += " FROM machine";

        auto s = extDb.prepare(machQuery);
        while (s.step()) {
            VtdbMachine m;
            m.id = s.getText(0);
            m.name = s.getText(1);
            m.make = s.getText(2);
            m.model = s.getText(3);
            m.controller_type = s.getText(4);
            m.dimensions_units = static_cast<int>(s.getInt(5));
            m.max_width = s.getDouble(6);
            m.max_height = s.getDouble(7);
            m.support_rotary = static_cast<int>(s.getInt(8));
            m.support_tool_change = static_cast<int>(s.getInt(9));
            m.has_laser_head = static_cast<int>(s.getInt(10));
            if (hasExtended) {
                m.spindle_power_watts = s.getDouble(11);
                m.max_rpm = static_cast<int>(s.getInt(12));
                m.drive_type = static_cast<DriveType>(s.getInt(13));
            }
            insertMachine(m);
        }
    }

    // 3. Tool geometry
    {
        auto s = extDb.prepare(
            "SELECT id, name_format, notes, tool_type, units, diameter, included_angle, "
            "flat_diameter, num_flutes, flute_length, thread_pitch, outline, tip_radius, "
            "laser_watt, custom_attributes, tooth_size, tooth_offset, neck_length, "
            "tooth_height, threaded_length FROM tool_geometry");
        while (s.step()) {
            VtdbToolGeometry g;
            g.id = s.getText(0);
            g.name_format = s.getText(1);
            g.notes = s.getText(2);
            g.tool_type = static_cast<VtdbToolType>(s.getInt(3));
            g.units = static_cast<VtdbUnits>(s.getInt(4));
            g.diameter = s.getDouble(5);
            g.included_angle = s.getDouble(6);
            g.flat_diameter = s.getDouble(7);
            g.num_flutes = static_cast<int>(s.getInt(8));
            g.flute_length = s.getDouble(9);
            g.thread_pitch = s.getDouble(10);
            g.outline = s.getBlob(11);
            g.tip_radius = s.getDouble(12);
            g.laser_watt = static_cast<int>(s.getInt(13));
            g.custom_attributes = s.getText(14);
            g.tooth_size = s.getDouble(15);
            g.tooth_offset = s.getDouble(16);
            g.neck_length = s.getDouble(17);
            g.tooth_height = s.getDouble(18);
            g.threaded_length = s.getDouble(19);
            if (insertGeometry(g))
                importedGeometries++;
        }
    }

    // 4. Cutting data
    {
        auto s = extDb.prepare(
            "SELECT id, rate_units, feed_rate, plunge_rate, spindle_speed, spindle_dir, "
            "stepdown, stepover, clear_stepover, thread_depth, thread_step_in, "
            "laser_power, laser_passes, laser_burn_rate, line_width, length_units, "
            "tool_number, laser_kerf, notes FROM tool_cutting_data");
        while (s.step()) {
            VtdbCuttingData cd;
            cd.id = s.getText(0);
            cd.rate_units = static_cast<int>(s.getInt(1));
            cd.feed_rate = s.getDouble(2);
            cd.plunge_rate = s.getDouble(3);
            cd.spindle_speed = static_cast<int>(s.getInt(4));
            cd.spindle_dir = static_cast<int>(s.getInt(5));
            cd.stepdown = s.getDouble(6);
            cd.stepover = s.getDouble(7);
            cd.clear_stepover = s.getDouble(8);
            cd.thread_depth = s.getDouble(9);
            cd.thread_step_in = s.getDouble(10);
            cd.laser_power = s.getDouble(11);
            cd.laser_passes = static_cast<int>(s.getInt(12));
            cd.laser_burn_rate = s.getDouble(13);
            cd.line_width = s.getDouble(14);
            cd.length_units = static_cast<int>(s.getInt(15));
            cd.tool_number = static_cast<int>(s.getInt(16));
            cd.laser_kerf = static_cast<int>(s.getInt(17));
            cd.notes = s.getText(18);
            insertCuttingData(cd);
        }
    }

    // 5. Tool entities
    {
        auto s = extDb.prepare(
            "SELECT id, material_id, machine_id, tool_geometry_id, tool_cutting_data_id "
            "FROM tool_entity");
        while (s.step()) {
            VtdbToolEntity e;
            e.id = s.getText(0);
            e.material_id = s.isNull(1) ? "" : s.getText(1);
            e.machine_id = s.getText(2);
            e.tool_geometry_id = s.getText(3);
            e.tool_cutting_data_id = s.getText(4);
            insertEntity(e);
        }
    }

    // 6. Tree entries
    {
        auto s = extDb.prepare(
            "SELECT id, parent_group_id, sibling_order, tool_geometry_id, name, notes, expanded "
            "FROM tool_tree_entry");
        while (s.step()) {
            VtdbTreeEntry te;
            te.id = s.getText(0);
            te.parent_group_id = s.isNull(1) ? "" : s.getText(1);
            te.sibling_order = static_cast<int>(s.getInt(2));
            te.tool_geometry_id = s.isNull(3) ? "" : s.getText(3);
            te.name = s.isNull(4) ? "" : s.getText(4);
            te.notes = s.getText(5);
            te.expanded = static_cast<int>(s.getInt(6));
            insertTreeEntry(te);
        }
    }

    if (!txn.commit()) {
        log::error("ToolDatabase", "Failed to commit import");
        return -1;
    }

    log::infof("ToolDatabase", "Imported %d geometries from %s",
               importedGeometries, externalPath.string().c_str());
    return importedGeometries;
}

} // namespace dw
