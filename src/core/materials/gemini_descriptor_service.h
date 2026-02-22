#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dw {

// Result of a descriptor request (AI classification of model thumbnail)
struct DescriptorResult {
    bool success = false;
    std::string error;
    std::string title;
    std::string description;
    std::string hoverNarrative;
    std::vector<std::string> keywords;     // 3-5
    std::vector<std::string> associations; // brands/logos
    std::vector<std::string> categories;   // broad -> specific
};

// Describes models via Gemini API image classification.
// All methods are blocking — call from a worker thread.
class GeminiDescriptorService {
  public:
    DescriptorResult describe(const std::string& thumbnailPath, const std::string& apiKey);

  private:
    // Convert model TGA thumbnail to PNG in-memory
    std::vector<uint8_t> tgaToPng(const std::string& tgaPath);

    // Send PNG image to Gemini for classification, return raw JSON text
    std::string fetchClassification(const std::vector<uint8_t>& imageData,
                                    const std::string& apiKey);

    // Parse Gemini JSON response into DescriptorResult
    DescriptorResult parseClassification(const std::string& json);
};

} // namespace dw
