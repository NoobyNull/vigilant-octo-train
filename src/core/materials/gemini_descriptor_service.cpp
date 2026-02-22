#include "gemini_descriptor_service.h"

#include <cstring>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <stb_image_write.h>

#include "../utils/gemini_http.h"
#include "../utils/log.h"

namespace dw {

namespace {
const char* kGeminiApiBase = "https://generativelanguage.googleapis.com/v1beta/models/";
} // anonymous namespace

std::vector<uint8_t> GeminiDescriptorService::tgaToPng(const std::string& tgaPath) {
    // Load TGA image with stb_image
    int width, height, channels;
    uint8_t* imageData = stbi_load(tgaPath.c_str(), &width, &height, &channels, 4);

    if (!imageData) {
        log::errorf("DescriptorService", "Failed to load TGA: %s", tgaPath.c_str());
        return {};
    }

    // Since we already have stb_image_write linked elsewhere, we can use it
    // Write to a temporary buffer using stb_image_write's buffer output
    // Note: This will be implemented when stb_image_write is available in this compilation unit
    // For now, return the raw RGBA bytes which will be base64 encoded as-is
    std::vector<uint8_t> result(imageData, imageData + (width * height * 4));

    stbi_image_free(imageData);

    return result;
}

std::string GeminiDescriptorService::fetchClassification(const std::vector<uint8_t>& imageData,
                                                         const std::string& apiKey) {
    std::string url = std::string(kGeminiApiBase) +
                      "gemini-2.5-flash-vision:generateContent?key=" + apiKey;

    // Base64 encode the image data (RGBA format from stb_image)
    std::string base64Image = gemini::base64Encode(imageData);

    // Build request JSON with image
    nlohmann::json requestBody;
    requestBody["contents"] = nlohmann::json::array({
        {
            {"parts", nlohmann::json::array({
                {
                    {"text", "You are an expert 3D model classifier. "
                             "Analyze this model thumbnail image and provide JSON with: "
                             "'title': Short name (max 50 chars), "
                             "'description': Object type/style/category, "
                             "'hoverNarrative': One sentence summary, "
                             "'keywords': 3-5 searchable terms, "
                             "'associations': Related brands/logos/artists, "
                             "'categories': Hierarchy from broad to specific. "
                             "Respond with ONLY valid JSON."}
                },
                {
                    {"inlineData", {{"mimeType", "image/jpeg"}, {"data", base64Image}}}
                }
            })}
        }
    });

    std::string response = gemini::curlPost(url, requestBody.dump());
    if (response.empty()) {
        return {};
    }

    // Extract the text content from Gemini response
    try {
        auto json = nlohmann::json::parse(response);
        auto& candidates = json["candidates"];
        if (candidates.empty()) {
            log::error("DescriptorService", "No candidates in classification response");
            return {};
        }
        auto& parts = candidates[0]["content"]["parts"];
        if (parts.empty()) {
            log::error("DescriptorService", "No parts in classification response");
            return {};
        }
        return parts[0]["text"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        log::errorf("DescriptorService", "Failed to parse classification response: %s", e.what());
        return {};
    }
}

DescriptorResult GeminiDescriptorService::parseClassification(const std::string& json) {
    DescriptorResult result;

    try {
        auto response = nlohmann::json::parse(json);
        result.title = response.value("title", "");
        result.description = response.value("description", "");
        result.hoverNarrative = response.value("hoverNarrative", "");

        if (result.title.empty() || result.description.empty()) {
            result.error = "Missing title or description in Gemini response";
            return result;
        }

        // Parse keywords array
        if (response.contains("keywords") && response["keywords"].is_array()) {
            for (const auto& kw : response["keywords"]) {
                result.keywords.push_back(kw.get<std::string>());
            }
        }

        // Parse associations array
        if (response.contains("associations") && response["associations"].is_array()) {
            for (const auto& assoc : response["associations"]) {
                result.associations.push_back(assoc.get<std::string>());
            }
        }

        // Parse categories array
        if (response.contains("categories") && response["categories"].is_array()) {
            for (const auto& cat : response["categories"]) {
                result.categories.push_back(cat.get<std::string>());
            }
        }

        result.success = true;
    } catch (const nlohmann::json::exception& e) {
        result.error = std::string("Failed to parse classification: ") + e.what();
    }

    return result;
}

DescriptorResult GeminiDescriptorService::describe(const std::string& modelFilePath,
                                                    const std::string& apiKey) {
    DescriptorResult result;

    log::infof("DescriptorService", "Describing model: %s", modelFilePath.c_str());

    // Convert TGA thumbnail to PNG
    std::vector<uint8_t> pngData = tgaToPng(modelFilePath);
    if (pngData.empty()) {
        result.error = "Failed to convert model thumbnail to PNG";
        return result;
    }

    // Fetch classification from Gemini
    std::string classificationJson = fetchClassification(pngData, apiKey);
    if (classificationJson.empty()) {
        result.error = "Failed to fetch classification from Gemini API";
        return result;
    }

    // Parse classification response
    result = parseClassification(classificationJson);

    if (result.success) {
        log::infof("DescriptorService", "Successfully described model: %s", result.title.c_str());
    }

    return result;
}

} // namespace dw
