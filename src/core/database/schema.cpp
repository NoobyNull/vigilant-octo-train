#include "schema.h"

#include "../paths/app_paths.h"
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
        if (version < CURRENT_VERSION) {
            log::infof("Schema", "Migrating from version %d to %d", version, CURRENT_VERSION);
            if (!migrate(db, version)) {
                log::error("Schema", "Migration failed");
                return false;
            }
            return true;
        }
        log::warningf("Schema",
                      "Version mismatch (have %d, want %d) - attempting table creation",
                      version,
                      CURRENT_VERSION);
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

    // Materials table - wood species and material properties
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS materials (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT NOT NULL DEFAULT 'hardwood',
            archive_path TEXT,
            janka_hardness REAL DEFAULT 0,
            feed_rate REAL DEFAULT 0,
            spindle_speed REAL DEFAULT 0,
            depth_of_cut REAL DEFAULT 0,
            cost_per_board_foot REAL DEFAULT 0,
            grain_direction_deg REAL DEFAULT 0,
            thumbnail_path TEXT,
            imported_at TEXT DEFAULT CURRENT_TIMESTAMP
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
            tags TEXT DEFAULT '[]',
            material_id INTEGER DEFAULT NULL,
            orient_yaw REAL DEFAULT NULL,
            orient_matrix TEXT DEFAULT NULL,
            camera_distance REAL DEFAULT NULL,
            camera_pitch REAL DEFAULT NULL,
            camera_yaw REAL DEFAULT NULL,
            camera_target_x REAL DEFAULT NULL,
            camera_target_y REAL DEFAULT NULL,
            camera_target_z REAL DEFAULT NULL,
            descriptor_title TEXT DEFAULT NULL,
            descriptor_description TEXT DEFAULT NULL,
            descriptor_hover TEXT DEFAULT NULL,
            tag_status INTEGER DEFAULT 0
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
            notes TEXT DEFAULT '',
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

    // Project-GCode junction table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS project_gcode (
            project_id INTEGER NOT NULL,
            gcode_id INTEGER NOT NULL,
            sort_order INTEGER DEFAULT 0,
            added_at TEXT DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (project_id, gcode_id),
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
            FOREIGN KEY (gcode_id) REFERENCES gcode_files(id) ON DELETE CASCADE
        )
    )")) {
        return false;
    }

    // Cut plans table
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS cut_plans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project_id INTEGER,
            name TEXT NOT NULL,
            algorithm TEXT NOT NULL,
            sheet_config TEXT NOT NULL,
            parts TEXT NOT NULL,
            result TEXT NOT NULL,
            allow_rotation INTEGER DEFAULT 1,
            kerf REAL DEFAULT 0,
            margin REAL DEFAULT 0,
            sheets_used INTEGER DEFAULT 0,
            efficiency REAL DEFAULT 0,
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

    // Categories table (2-level hierarchy via parent_id)
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            parent_id INTEGER DEFAULT NULL REFERENCES categories(id) ON DELETE CASCADE,
            sort_order INTEGER DEFAULT 0,
            UNIQUE(name, parent_id)
        )
    )")) {
        return false;
    }

    // Model-categories junction table (many-to-many)
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS model_categories (
            model_id INTEGER NOT NULL REFERENCES models(id) ON DELETE CASCADE,
            category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE CASCADE,
            PRIMARY KEY (model_id, category_id)
        )
    )")) {
        return false;
    }

    // FTS5 virtual table (external content from models)
    if (!db.execute(R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS models_fts USING fts5(
            name,
            tags,
            content='models',
            content_rowid='id',
            tokenize='unicode61'
        )
    )")) {
        return false;
    }

    // FTS5 triggers -- keep index in sync with models table
    // AFTER INSERT: add to FTS
    (void)db.execute(R"(
        CREATE TRIGGER IF NOT EXISTS models_fts_ai AFTER INSERT ON models BEGIN
            INSERT INTO models_fts(rowid, name, tags) VALUES (new.id, new.name, new.tags);
        END
    )");

    // BEFORE UPDATE: delete old tokens (MUST be BEFORE, not AFTER -- see Pitfall 2)
    (void)db.execute(R"(
        CREATE TRIGGER IF NOT EXISTS models_fts_bu BEFORE UPDATE ON models BEGIN
            INSERT INTO models_fts(models_fts, rowid, name, tags)
            VALUES ('delete', old.id, old.name, old.tags);
        END
    )");

    // AFTER UPDATE: insert new tokens
    (void)db.execute(R"(
        CREATE TRIGGER IF NOT EXISTS models_fts_au AFTER UPDATE ON models BEGIN
            INSERT INTO models_fts(rowid, name, tags) VALUES (new.id, new.name, new.tags);
        END
    )");

    // AFTER DELETE: delete tokens
    (void)db.execute(R"(
        CREATE TRIGGER IF NOT EXISTS models_fts_ad AFTER DELETE ON models BEGIN
            INSERT INTO models_fts(models_fts, rowid, name, tags)
            VALUES ('delete', old.id, old.name, old.tags);
        END
    )");

    // CNC tools table — router bit definitions
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS cnc_tools (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL DEFAULT 'flat_end_mill',
            diameter REAL DEFAULT 0,
            flute_count INTEGER DEFAULT 2,
            max_rpm REAL DEFAULT 24000,
            max_doc REAL DEFAULT 0,
            shank_diameter REAL DEFAULT 0.25,
            notes TEXT DEFAULT '',
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        return false;
    }

    // Per-tool-per-material cutting parameters
    if (!db.execute(R"(
        CREATE TABLE IF NOT EXISTS tool_material_params (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tool_id INTEGER NOT NULL,
            material_id INTEGER NOT NULL,
            feed_rate REAL DEFAULT 0,
            spindle_speed REAL DEFAULT 0,
            depth_of_cut REAL DEFAULT 0,
            chip_load REAL DEFAULT 0,
            UNIQUE(tool_id, material_id),
            FOREIGN KEY (tool_id) REFERENCES cnc_tools(id) ON DELETE CASCADE,
            FOREIGN KEY (material_id) REFERENCES materials(id) ON DELETE CASCADE
        )
    )")) {
        return false;
    }

    // Create indexes for common queries (best-effort)
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_materials_name ON materials(name)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_materials_category ON materials(category)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_hash ON models(hash)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_name ON models(name)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_models_format ON models(file_format)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_project_models_project ON "
                     "project_models(project_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_project_models_model ON "
                     "project_models(model_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cost_estimates_project ON "
                     "cost_estimates(project_id)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_project_gcode_project ON project_gcode(project_id)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_project_gcode_gcode ON project_gcode(gcode_id)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_cut_plans_project ON cut_plans(project_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_hash ON gcode_files(hash)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_name ON gcode_files(name)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_operation_groups_model ON operation_groups(model_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_gcode_group_members_group ON "
                     "gcode_group_members(group_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_categories_parent ON categories(parent_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_model_categories_model ON "
                     "model_categories(model_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_model_categories_category ON "
                     "model_categories(category_id)");
    (void)db.execute(
        "CREATE INDEX IF NOT EXISTS idx_models_tag_status ON models(tag_status)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cnc_tools_name ON cnc_tools(name)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cnc_tools_type ON cnc_tools(type)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_tool_material_tool ON "
                     "tool_material_params(tool_id)");
    (void)db.execute("CREATE INDEX IF NOT EXISTS idx_tool_material_material ON "
                     "tool_material_params(material_id)");

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

bool Schema::migrate(Database& db, int fromVersion) {
    Transaction txn(db);

    if (fromVersion < 5) {
        // v5: Add orient columns to models table
        (void)db.execute("ALTER TABLE models ADD COLUMN orient_yaw REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN orient_matrix TEXT DEFAULT NULL");
        log::info("Schema", "Added orient_yaw and orient_matrix columns to models");
    }

    if (fromVersion < 6) {
        // v6: Add camera state columns to models table
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_distance REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_pitch REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_yaw REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_target_x REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_target_y REAL DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN camera_target_z REAL DEFAULT NULL");
        log::info("Schema", "Added camera state columns to models");
    }

    if (fromVersion < 7) {
        // v7: Categories, model_categories junction, FTS5 virtual table + triggers
        (void)db.execute(R"(
            CREATE TABLE IF NOT EXISTS categories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                parent_id INTEGER DEFAULT NULL REFERENCES categories(id) ON DELETE CASCADE,
                sort_order INTEGER DEFAULT 0,
                UNIQUE(name, parent_id)
            )
        )");
        (void)db.execute(R"(
            CREATE TABLE IF NOT EXISTS model_categories (
                model_id INTEGER NOT NULL REFERENCES models(id) ON DELETE CASCADE,
                category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE CASCADE,
                PRIMARY KEY (model_id, category_id)
            )
        )");
        (void)db.execute(R"(
            CREATE VIRTUAL TABLE IF NOT EXISTS models_fts USING fts5(
                name, tags, content='models', content_rowid='id', tokenize='unicode61'
            )
        )");
        // FTS5 triggers
        (void)db.execute(R"(
            CREATE TRIGGER IF NOT EXISTS models_fts_ai AFTER INSERT ON models BEGIN
                INSERT INTO models_fts(rowid, name, tags) VALUES (new.id, new.name, new.tags);
            END
        )");
        (void)db.execute(R"(
            CREATE TRIGGER IF NOT EXISTS models_fts_bu BEFORE UPDATE ON models BEGIN
                INSERT INTO models_fts(models_fts, rowid, name, tags)
                VALUES ('delete', old.id, old.name, old.tags);
            END
        )");
        (void)db.execute(R"(
            CREATE TRIGGER IF NOT EXISTS models_fts_au AFTER UPDATE ON models BEGIN
                INSERT INTO models_fts(rowid, name, tags) VALUES (new.id, new.name, new.tags);
            END
        )");
        (void)db.execute(R"(
            CREATE TRIGGER IF NOT EXISTS models_fts_ad AFTER DELETE ON models BEGIN
                INSERT INTO models_fts(models_fts, rowid, name, tags)
                VALUES ('delete', old.id, old.name, old.tags);
            END
        )");
        // Indexes
        (void)db.execute(
            "CREATE INDEX IF NOT EXISTS idx_categories_parent ON categories(parent_id)");
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_model_categories_model ON "
                         "model_categories(model_id)");
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_model_categories_category ON "
                         "model_categories(category_id)");
        // Rebuild FTS index for existing rows
        (void)db.execute("INSERT INTO models_fts(models_fts) VALUES('rebuild')");
        log::info("Schema", "Added categories, model_categories, and FTS5 index");
    }

    if (fromVersion < 8) {
        // v8: Add AI descriptor fields to models table
        (void)db.execute("ALTER TABLE models ADD COLUMN descriptor_title TEXT DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN descriptor_description TEXT DEFAULT NULL");
        (void)db.execute("ALTER TABLE models ADD COLUMN descriptor_hover TEXT DEFAULT NULL");
        log::info(
            "Schema",
            "Added descriptor_title, descriptor_description, descriptor_hover columns to models");
    }

    if (fromVersion < 9) {
        // v9: project_gcode junction, cut_plans table, notes column on projects
        const char* v9[] = {
            "CREATE TABLE IF NOT EXISTS project_gcode ("
            "project_id INTEGER NOT NULL, gcode_id INTEGER NOT NULL, "
            "sort_order INTEGER DEFAULT 0, added_at TEXT DEFAULT CURRENT_TIMESTAMP, "
            "PRIMARY KEY (project_id, gcode_id), "
            "FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE, "
            "FOREIGN KEY (gcode_id) REFERENCES gcode_files(id) ON DELETE CASCADE)",
            "CREATE INDEX IF NOT EXISTS idx_project_gcode_project ON project_gcode(project_id)",
            "CREATE INDEX IF NOT EXISTS idx_project_gcode_gcode ON project_gcode(gcode_id)",
            "CREATE TABLE IF NOT EXISTS cut_plans ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, project_id INTEGER, "
            "name TEXT NOT NULL, algorithm TEXT NOT NULL, "
            "sheet_config TEXT NOT NULL, parts TEXT NOT NULL, result TEXT NOT NULL, "
            "allow_rotation INTEGER DEFAULT 1, kerf REAL DEFAULT 0, margin REAL DEFAULT 0, "
            "sheets_used INTEGER DEFAULT 0, efficiency REAL DEFAULT 0, "
            "created_at TEXT DEFAULT CURRENT_TIMESTAMP, modified_at TEXT DEFAULT CURRENT_TIMESTAMP, "
            "FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE SET NULL)",
            "CREATE INDEX IF NOT EXISTS idx_cut_plans_project ON cut_plans(project_id)",
            "ALTER TABLE projects ADD COLUMN notes TEXT DEFAULT ''",
        };
        for (const auto* sql : v9) {
            if (!db.execute(sql)) return false;
        }
        log::info("Schema", "Added project_gcode, cut_plans tables and notes column on projects");
    }

    if (fromVersion < 10) {
        // v10: Rewrite absolute paths to relative (strip known prefixes)
        // This allows category directories to be relocatable.
        auto stripPrefix = [&db](const char* table,
                                  const char* column,
                                  const std::string& prefix) {
            if (prefix.empty())
                return;
            // Ensure prefix ends with /
            std::string pfx = prefix;
            if (pfx.back() != '/')
                pfx += '/';

            // UPDATE table SET column = SUBSTR(column, len+1)
            // WHERE column LIKE 'prefix/%' AND column NOT LIKE '/%' -- already relative? skip
            std::string sql = "UPDATE ";
            sql += table;
            sql += " SET ";
            sql += column;
            sql += " = SUBSTR(";
            sql += column;
            sql += ", ";
            sql += std::to_string(pfx.size() + 1);
            sql += ") WHERE ";
            sql += column;
            sql += " LIKE '";
            sql += pfx;
            sql += "%'";

            (void)db.execute(sql);
        };

        // Strip blob store prefix from models.file_path
        std::string blobDir = paths::getBlobStoreDir().string();
        std::string dataDir = paths::getDataDir().string();

        stripPrefix("models", "file_path", blobDir);
        stripPrefix("models", "file_path", dataDir + "/models");
        stripPrefix("gcode_files", "file_path", blobDir);
        stripPrefix("gcode_files", "file_path", dataDir + "/gcode");
        stripPrefix("materials", "archive_path", dataDir + "/materials");

        log::info("Schema",
                  "v10: Rewrote absolute paths to relative for models, gcode_files, materials");
    }

    if (fromVersion < 11) {
        (void)db.execute("ALTER TABLE models ADD COLUMN tag_status INTEGER DEFAULT 0");
        (void)db.execute(
            "CREATE INDEX IF NOT EXISTS idx_models_tag_status ON models(tag_status)");
        log::info("Schema", "v11: Added tag_status column to models");
    }

    if (fromVersion < 12) {
        // v12: CNC tools and per-tool-per-material cutting parameters
        if (!db.execute(R"(
            CREATE TABLE IF NOT EXISTS cnc_tools (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                type TEXT NOT NULL DEFAULT 'flat_end_mill',
                diameter REAL DEFAULT 0,
                flute_count INTEGER DEFAULT 2,
                max_rpm REAL DEFAULT 24000,
                max_doc REAL DEFAULT 0,
                shank_diameter REAL DEFAULT 0.25,
                notes TEXT DEFAULT '',
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        )"))
            return false;
        if (!db.execute(R"(
            CREATE TABLE IF NOT EXISTS tool_material_params (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                tool_id INTEGER NOT NULL,
                material_id INTEGER NOT NULL,
                feed_rate REAL DEFAULT 0,
                spindle_speed REAL DEFAULT 0,
                depth_of_cut REAL DEFAULT 0,
                chip_load REAL DEFAULT 0,
                UNIQUE(tool_id, material_id),
                FOREIGN KEY (tool_id) REFERENCES cnc_tools(id) ON DELETE CASCADE,
                FOREIGN KEY (material_id) REFERENCES materials(id) ON DELETE CASCADE
            )
        )"))
            return false;
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cnc_tools_name ON cnc_tools(name)");
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_cnc_tools_type ON cnc_tools(type)");
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_tool_material_tool ON "
                         "tool_material_params(tool_id)");
        (void)db.execute("CREATE INDEX IF NOT EXISTS idx_tool_material_material ON "
                         "tool_material_params(material_id)");
        log::info("Schema", "v12: Added cnc_tools and tool_material_params tables");
    }

    if (!setVersion(db, CURRENT_VERSION)) {
        txn.rollback();
        return false;
    }

    if (!txn.commit()) {
        log::error("Schema", "Failed to commit migration");
        return false;
    }

    log::infof("Schema", "Migrated to version %d", CURRENT_VERSION);
    return true;
}

} // namespace dw
