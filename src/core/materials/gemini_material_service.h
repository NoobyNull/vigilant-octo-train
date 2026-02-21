#pragma once

#include <string>
#include <vector>

#include "../types.h"
#include "material.h"

namespace dw {

// Result of a material generation request
struct GenerateResult {
    bool success = false;
    std::string error;
    Path dwmatPath;        // path to created .dwmat archive
    MaterialRecord record; // parsed properties
};

// Generates materials via Gemini API (texture image + CNC properties).
// All methods are blocking — call from a worker thread.
class GeminiMaterialService {
  public:
    GenerateResult generate(const std::string& prompt, const std::string& apiKey);

  private:
    // Fetch CNC properties JSON from Gemini
    std::string fetchProperties(const std::string& prompt, const std::string& apiKey);

    // Fetch texture PNG bytes from Gemini
    std::vector<uint8_t> fetchTexture(const std::string& prompt, const std::string& apiKey);

    // Parse Gemini JSON response into a MaterialRecord
    MaterialRecord parseProperties(const std::string& json, const std::string& name);
};

} // namespace dw
