#pragma once

#include <string>
#include <vector>

#include "database.h"

namespace dw {

// Repository for "My Toolbox" — user's curated subset of tool geometries
// from the .vtdb tool library. Stored in the main app database.
class ToolboxRepository {
  public:
    explicit ToolboxRepository(Database& db);

    // Add a tool geometry to the toolbox (optional display name override)
    bool addTool(const std::string& geometryId, const std::string& displayName = "");

    // Remove a tool from the toolbox
    bool removeTool(const std::string& geometryId);

    // Check if a geometry is in the toolbox
    bool hasTool(const std::string& geometryId) const;

    // Get all geometry IDs in the toolbox
    std::vector<std::string> getAllGeometryIds() const;

    // Get/set the display name override for a toolbox entry
    std::string getDisplayName(const std::string& geometryId) const;
    bool setDisplayName(const std::string& geometryId, const std::string& name);

  private:
    Database& m_db;
};

} // namespace dw
