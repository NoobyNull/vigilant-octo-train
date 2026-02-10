#include "gcode_loader.h"

#include <algorithm>
#include <set>

#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

LoadResult GCodeLoader::load(const Path& path) {
    // Read file content
    auto content = file::readText(path);
    if (!content || content->empty()) {
        return LoadResult{nullptr, "Failed to read file or file is empty"};
    }

    // Parse G-code
    gcode::Parser parser;
    gcode::Program program = parser.parse(*content);

    if (!parser.lastError().empty()) {
        return LoadResult{nullptr, "Parse error: " + parser.lastError()};
    }

    return processProgram(program);
}

LoadResult GCodeLoader::loadFromBuffer(const ByteBuffer& data) {
    // Convert byte buffer to string
    std::string content(reinterpret_cast<const char*>(data.data()), data.size());

    // Parse G-code
    gcode::Parser parser;
    gcode::Program program = parser.parse(content);

    if (!parser.lastError().empty()) {
        return LoadResult{nullptr, "Parse error: " + parser.lastError()};
    }

    return processProgram(program);
}

bool GCodeLoader::supports(const std::string& extension) const {
    return extension == "gcode" || extension == "nc" || extension == "ngc" || extension == "tap";
}

std::vector<std::string> GCodeLoader::extensions() const {
    return {"gcode", "nc", "ngc", "tap"};
}

LoadResult GCodeLoader::processProgram(const gcode::Program& program) {
    // Check if program has toolpath
    if (program.path.empty()) {
        return LoadResult{nullptr, "No toolpath found in G-code"};
    }

    // Analyze program for statistics
    gcode::Analyzer analyzer;
    gcode::Statistics stats = analyzer.analyze(program);

    // Extract metadata
    m_lastMetadata = extractMetadata(program, stats);

    // Convert toolpath to mesh
    MeshPtr mesh = toolpathToMesh(program.path);
    if (!mesh) {
        return LoadResult{nullptr, "Failed to convert toolpath to mesh"};
    }

    return LoadResult{mesh, ""};
}

MeshPtr GCodeLoader::toolpathToMesh(const std::vector<gcode::PathSegment>& path) {
    auto mesh = std::make_shared<Mesh>();

    // Estimate vertex count: 4 vertices per segment (quad = 2 triangles)
    mesh->reserve(static_cast<u32>(path.size() * 4), static_cast<u32>(path.size() * 6));

    for (const auto& segment : path) {
        // Calculate direction vector
        Vec3 direction = segment.end - segment.start;
        f32 length = glm::length(direction);

        // Skip degenerate segments
        if (length < 0.0001f) {
            continue;
        }

        direction = glm::normalize(direction);

        // Choose width based on move type
        f32 width = segment.isRapid ? 0.2f : 0.5f;

        // Find perpendicular direction for extrusion
        Vec3 up = Vec3{0.0f, 0.0f, 1.0f};
        Vec3 perpendicular;

        // Handle vertical segments (direction parallel to up)
        if (std::abs(glm::dot(direction, up)) > 0.999f) {
            // Use X-axis as fallback perpendicular
            perpendicular = Vec3{1.0f, 0.0f, 0.0f};
        } else {
            // Cross product gives perpendicular vector
            perpendicular = glm::normalize(glm::cross(direction, up));
        }

        // Scale perpendicular by half-width
        perpendicular = perpendicular * (width * 0.5f);

        // Create quad vertices (4 corners of extruded segment)
        Vec3 p0 = segment.start - perpendicular;
        Vec3 p1 = segment.start + perpendicular;
        Vec3 p2 = segment.end + perpendicular;
        Vec3 p3 = segment.end - perpendicular;

        // Calculate normal (pointing outward from quad)
        Vec3 normal = glm::normalize(glm::cross(perpendicular, direction));

        // Encode rapid/cutting in texCoord.x: 1.0 = rapid, 0.0 = cutting
        f32 moveTypeFlag = segment.isRapid ? 1.0f : 0.0f;

        // Add vertices
        u32 baseIndex = mesh->vertexCount();
        mesh->addVertex(Vertex{p0, normal, Vec2{moveTypeFlag, 0.0f}});
        mesh->addVertex(Vertex{p1, normal, Vec2{moveTypeFlag, 0.0f}});
        mesh->addVertex(Vertex{p2, normal, Vec2{moveTypeFlag, 1.0f}});
        mesh->addVertex(Vertex{p3, normal, Vec2{moveTypeFlag, 1.0f}});

        // Add two triangles (quad)
        mesh->addTriangle(baseIndex + 0, baseIndex + 1, baseIndex + 2);
        mesh->addTriangle(baseIndex + 0, baseIndex + 2, baseIndex + 3);
    }

    // Recalculate bounds
    mesh->recalculateBounds();

    return mesh;
}

GCodeMetadata GCodeLoader::extractMetadata(const gcode::Program& program,
                                           const gcode::Statistics& stats) {
    GCodeMetadata metadata;

    // Copy bounds
    metadata.boundsMin = stats.boundsMin;
    metadata.boundsMax = stats.boundsMax;

    // Copy statistics
    metadata.totalDistance = stats.totalPathLength;
    metadata.estimatedTime = stats.estimatedTime;

    // Collect unique feed rates
    std::set<f32> uniqueFeedRates;
    for (const auto& cmd : program.commands) {
        if (cmd.hasF()) {
            uniqueFeedRates.insert(cmd.f);
        }
    }
    metadata.feedRates = std::vector<f32>(uniqueFeedRates.begin(), uniqueFeedRates.end());
    std::sort(metadata.feedRates.begin(), metadata.feedRates.end());

    // Collect unique tool numbers
    std::set<int> uniqueToolNumbers;
    for (const auto& cmd : program.commands) {
        if (cmd.hasT()) {
            uniqueToolNumbers.insert(cmd.t);
        }
    }
    metadata.toolNumbers = std::vector<int>(uniqueToolNumbers.begin(), uniqueToolNumbers.end());
    std::sort(metadata.toolNumbers.begin(), metadata.toolNumbers.end());

    return metadata;
}

} // namespace dw
