#include "schema.h"

#include "../utils/log.h"

namespace dw {

bool Schema::initialize(Database& db) {
    if (!db.isOpen()) {
        log::error("Schema", "Cannot initialize: database not open");
        return false;
    }

    // Check if already initialized
    if (isInitialized(db)) {
        int version = getVersion(db);
        if (version == CURRENT_VERSION) {
            log::debug("Schema", "Already up to date");
            return true;
        }
        // Note: We don't do migrations, just recreate if needed
        log::warningf("Schema", "Version mismatch: %d vs %d", version, CURRENT_VERSION);
    }

    return createTables(db);
}

bool Schema::isInitialized(Database& db) {
    auto stmt =
        db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_version'");
    return stmt.step();
}

int Schema::getVersion(Database& db) {
    auto stmt = db.prepare("SELECT version FROM schema_version LIMIT 1");
    if (stmt.step()) {
        return static_cast<int>(stmt.getInt(0));
    }
    return 0;
}

bool Schema::setVersion(Database& db, int version) {
    if (!db.execute("DELETE FROM schema_version")) {
        return false;
    }
    auto stmt = db.prepare("INSERT INTO schema_version (version) VALUES (?)");
    if (!stmt.bindInt(1, version)) {
        return false;
    }
    return stmt.execute();
}

bool Schema::createTables(Database& db) {
    Transaction txn(db);

    // Schema version table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER NOT NULL
        )
    )")) {
        return false;
    }

    // Models table - the library backbone
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS models (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hash TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_format TEXT NOT NULL,
            file_size INTEGER DEFAULT 0,
            vertex_count INTEGER DEFAULT 0,
            triangle_count INTEGER DEFAULT 0,
            bounds_min_x REAL DEFAULT 0,
            bounds_min_y REAL DEFAULT 0,
            bounds_min_z REAL DEFAULT 0,
            bounds_max_x REAL DEFAULT 0,
            bounds_max_y REAL DEFAULT 0,
            bounds_max_z REAL DEFAULT 0,
            thumbnail_path TEXT,
            imported_at TEXT DEFAULT CURRENT_TIMESTAMP,
            tags TEXT DEFAULT '[]'
        )
    )")) {
        return false;
    }

    // Projects table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            file_path TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            modified_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        return false;
    }

    // Project-Model junction table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS project_models (
            project_id INTEGER NOT NULL,
            model_id INTEGER NOT NULL,
            sort_order INTEGER DEFAULT 0,
            added_at TEXT DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (project_id, model_id),
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
            FOREIGN KEY (model_id) REFERENCES models(id) ON DELETE CASCADE
        )
    )")) {
        return false;
    }

    // Cost estimates table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS cost_estimates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            project_id INTEGER,
            items TEXT DEFAULT '[]',
            subtotal REAL DEFAULT 0,
            tax_rate REAL DEFAULT 0,
            tax_amount REAL DEFAULT 0,
            discount_rate REAL DEFAULT 0,
            discount_amount REAL DEFAULT 0,
            total REAL DEFAULT 0,
            notes TEXT DEFAULT '',
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            modified_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE SET NULL
        )
    )")) {
        return false;
    }

    // G-code files table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS gcode_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hash TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_size INTEGER DEFAULT 0,
            bounds_min_x REAL DEFAULT 0,
            bounds_min_y REAL DEFAULT 0,
            bounds_min_z REAL DEFAULT 0,
            bounds_max_x REAL DEFAULT 0,
            bounds_max_y REAL DEFAULT 0,
            bounds_max_z REAL DEFAULT 0,
            total_distance REAL DEFAULT 0,
            estimated_time REAL DEFAULT 0,
            feed_rates TEXT DEFAULT '[]',
            tool_numbers TEXT DEFAULT '[]',
            imported_at TEXT DEFAULT CURRENT_TIMESTAMP,
            thumbnail_path TEXT
        )
    )")) {
        return false;
    }

    // Operation groups table (hierarchy: model -> groups -> ordered gcode)
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS operation_groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            model_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            sort_order INTEGER DEFAULT 0,
            FOREIGN KEY (model_id) REFERENCES models(id) ON DELETE CASCADE
        )
    )")) {
        return false;
    }

    // G-code group members junction table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS gcode_group_members (
            group_id INTEGER NOT NULL,
            gcode_id INTEGER NOT NULL,
            sort_order INTEGER DEFAULT 0,
            PRIMARY KEY (group_id, gcode_id),
            FOREIGN KEY (group_id) REFERENCES operation_groups(id) ON DELETE CASCADE,
            FOREIGN KEY (gcode_id) REFERENCES gcode_files(id) ON DELETE CASCADE
        )
    )")) {
        return false;
    }

    // G-code templates table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS gcode_templates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            groups TEXT NOT NULL
        )
    )")) {
        return false;
    }

    // Seed CNC Router Basic template
    if (!db.execute(R"(
        INSERT OR IGNORE INTO gcode_templates (name, groups)
        VALUES ('CNC Router Basic', '["Roughing","Finishing","Profiling","Drilling"]')
    )")) {
        return false;
    }

    // Create indexes for common queries (best-effort)
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_hash ON models(hash)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_name ON models(name)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_format ON models(file_format)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_project_models_project ON "
                     "project_models(project_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_project_models_model ON "
                     "project_models(model_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cost_estimates_project ON "
                     "cost_estimates(project_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_hash ON gcode_files(hash)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_name ON gcode_files(name)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_operation_groups_model ON operation_groups(model_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_group_members_group ON "
                     "gcode_group_members(group_id)");

    // Set schema version
    if (!setVersion(db, CURRENT_VERSION)) {
        txn.rollback();
        return false;
    }

    if (!txn.commit()) {
        log::error("Schema", "Failed to commit creation");
        return false;
    }

    log::info("Schema", "Database schema initialized successfully");
    return true;
}

} // namespace dw
