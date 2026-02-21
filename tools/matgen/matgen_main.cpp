// dw_matgen — CLI tool to generate .dwmat archives for all built-in materials.
// Calls the Gemini API to produce AI-generated textures and CNC properties,
// then writes .dwmat files to resources/materials/ for bundling with the app.
//
// Usage: dw_matgen [OUTPUT_DIR]
//   - Reads GEMINI_API_KEY from environment (required)
//   - OUTPUT_DIR defaults to resources/materials/ relative to the repo root

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include "core/materials/default_materials.h"
#include "core/materials/gemini_material_service.h"
#include "core/utils/file_utils.h"
#include "core/utils/log.h"

using namespace dw;

int main(int argc, char* argv[]) {
    log::setLevel(log::Level::Info);

    // Get API key from environment
    const char* apiKey = std::getenv("GEMINI_API_KEY");
    if (apiKey == nullptr || apiKey[0] == '\0') {
        std::cerr << "Error: GEMINI_API_KEY environment variable is not set.\n";
        std::cerr << "Usage: GEMINI_API_KEY=AIza... " << argv[0] << " [OUTPUT_DIR]\n";
        return 1;
    }

    // Determine output directory
    Path outputDir = "resources/materials";
    if (argc > 1) {
        outputDir = argv[1];
    }

    if (!file::createDirectories(outputDir)) {
        std::cerr << "Error: could not create output directory: " << outputDir << "\n";
        return 1;
    }

    auto materials = getDefaultMaterials();
    const int total = static_cast<int>(materials.size());
    int generated = 0;
    int skipped = 0;
    int failed = 0;

    GeminiMaterialService service;

    for (int i = 0; i < total; ++i) {
        const auto& mat = materials[i];
        Path outPath = outputDir / (mat.name + ".dwmat");

        // Skip if already generated (resume support)
        if (file::isFile(outPath)) {
            std::cout << "[" << (i + 1) << "/" << total << "] " << mat.name
                      << "... SKIPPED (already exists)\n";
            ++skipped;
            continue;
        }

        std::cout << "[" << (i + 1) << "/" << total << "] Generating " << mat.name << "... "
                  << std::flush;

        auto start = std::chrono::steady_clock::now();
        auto result = service.generate(mat.name, apiKey);
        auto elapsed = std::chrono::steady_clock::now() - start;
        double secs =
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;

        if (!result.success) {
            std::cout << "FAILED (" << result.error << ")\n";
            ++failed;
            continue;
        }

        // The service writes to getMaterialsDir() by default — move to our output dir
        if (result.dwmatPath != outPath && file::exists(result.dwmatPath)) {
            if (!file::move(result.dwmatPath, outPath)) {
                std::cout << "FAILED (could not move to output dir)\n";
                ++failed;
                continue;
            }
        }

        std::cout << "OK (" << secs << "s)\n";
        ++generated;
    }

    std::cout << "\nDone: " << generated << " generated, " << skipped << " skipped, " << failed
              << " failed (out of " << total << " materials)\n";

    return failed > 0 ? 1 : 0;
}
