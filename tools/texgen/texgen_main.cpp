#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <optional>
#include <cstdlib>

#include "core/materials/default_materials.h"
#include "core/materials/gemini_material_service.h"
#include "core/materials/material_archive.h"
#include "core/paths/app_paths.h"
#include "core/utils/log.h"

namespace fs = std::filesystem;
using namespace dw;

int main(int argc, char* argv[]) {
    // Parse arguments
    std::string outputDir = paths::getMaterialsDir().string();
    if (argc > 1) {
        outputDir = argv[1];
    }

    // Get API key from environment
    const char* apiKeyEnv = std::getenv("GEMINI_API_KEY");
    if (!apiKeyEnv) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set\n";
        std::cerr << "Usage: GEMINI_API_KEY=<key> dw_texgen [output_dir]\n";
        return 1;
    }
    std::string apiKey(apiKeyEnv);

    // Create output directory if it doesn't exist
    try {
        fs::create_directories(outputDir);
    } catch (const std::exception& e) {
        std::cerr << "Error creating output directory: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Generating material textures...\n";
    std::cout << "Output directory: " << outputDir << "\n\n";

    // Get default materials
    auto materials = getDefaultMaterials();
    std::cout << "Processing " << materials.size() << " materials...\n\n";

    GeminiMaterialService service;
    int generated = 0;
    int skipped = 0;
    int failed = 0;

    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& material = materials[i];
        std::string archiveName = material.name + ".dwmat";
        fs::path archivePath = fs::path(outputDir) / archiveName;

        // Skip if already exists
        if (fs::exists(archivePath)) {
            std::cout << "[" << (i + 1) << "/" << materials.size() << "] " << material.name
                      << " (skipped - exists)\n";
            ++skipped;
            continue;
        }

        // Generate texture and properties
        std::cout << "[" << (i + 1) << "/" << materials.size() << "] " << material.name
                  << "... ";
        std::cout.flush();

        auto result = service.generate(material.name, apiKey);
        if (!result.success) {
            std::cout << "FAILED: " << result.error << "\n";
            ++failed;
            continue;
        }

        // Create archive
        try {
            auto archiveResult = MaterialArchive::create(
                archivePath.string(),
                result.record.archivePath.string(),
                result.record
            );
            if (!archiveResult.success) {
                std::cout << "FAILED: " << archiveResult.error << "\n";
                ++failed;
                continue;
            }
            std::cout << "OK\n";
            ++generated;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            ++failed;
        }
    }

    // Summary
    std::cout << "\n" << "Done: " << generated << " generated, " << skipped << " skipped, "
              << failed << " failed\n";

    return (failed > 0) ? 1 : 0;
}
