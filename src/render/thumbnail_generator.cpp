#include "thumbnail_generator.h"

#include <algorithm>
#include <cstring>
#include <fstream>

#include <glad/gl.h>

#include "../core/utils/file_utils.h"
#include "../core/utils/log.h"
#include "camera.h"
#include "framebuffer.h"
#include "renderer.h"

namespace dw {

namespace {

// Simple TGA writer for thumbnails (no external dependencies)
bool writeTGA(const Path& path, const ByteBuffer& pixels, int width, int height) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // TGA header (18 bytes)
    uint8_t header[18] = {0};
    header[2] = 2; // Uncompressed RGB
    header[12] = static_cast<uint8_t>(width & 0xFF);
    header[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
    header[14] = static_cast<uint8_t>(height & 0xFF);
    header[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
    header[16] = 32;   // 32 bits per pixel (BGRA)
    header[17] = 0x20; // Top-left origin

    file.write(reinterpret_cast<const char*>(header), 18);

    // Convert RGBA to BGRA and write
    std::vector<uint8_t> bgra(pixels.size());
    for (size_t i = 0; i < pixels.size(); i += 4) {
        bgra[i + 0] = pixels[i + 2]; // B
        bgra[i + 1] = pixels[i + 1]; // G
        bgra[i + 2] = pixels[i + 0]; // R
        bgra[i + 3] = pixels[i + 3]; // A
    }

    file.write(reinterpret_cast<const char*>(bgra.data()),
               static_cast<std::streamsize>(bgra.size()));

    return file.good();
}

} // namespace

ThumbnailGenerator::~ThumbnailGenerator() {
    shutdown();
}

bool ThumbnailGenerator::initialize() {
    if (m_initialized) {
        return true;
    }

    m_initialized = true;
    return true;
}

void ThumbnailGenerator::shutdown() {
    m_initialized = false;
}

bool ThumbnailGenerator::generate(const Mesh& mesh, const Path& outputPath,
                                  const ThumbnailSettings& settings) {
    auto pixels = generateToBuffer(mesh, settings);
    if (pixels.empty()) {
        return false;
    }

    // Use TGA format (simple to write, no dependencies)
    Path tgaPath = outputPath;
    if (file::getExtension(outputPath) != "tga") {
        tgaPath = outputPath.string() + ".tga";
    }

    if (!writeTGA(tgaPath, pixels, settings.width, settings.height)) {
        log::errorf("Thumbnail", "Failed to write: %s", tgaPath.string().c_str());
        return false;
    }

    return true;
}

ByteBuffer ThumbnailGenerator::generateToBuffer(const Mesh& mesh,
                                                const ThumbnailSettings& settings) {
    if (mesh.vertexCount() == 0) {
        return {};
    }

    // Create framebuffer
    Framebuffer fb;
    if (!fb.create(settings.width, settings.height)) {
        log::error("Thumbnail", "Failed to create framebuffer");
        return {};
    }

    // Create temporary renderer
    Renderer renderer;
    if (!renderer.initialize()) {
        log::error("Thumbnail", "Failed to initialize renderer");
        return {};
    }

    renderer.settings().objectColor = settings.objectColor;
    renderer.settings().showGrid = settings.showGrid;
    renderer.settings().showAxis = false;

    // Setup camera to fit the model
    Camera camera;
    camera.setViewport(settings.width, settings.height);

    // Fit camera to view the whole model
    const auto& bounds = mesh.bounds();
    camera.fitToBounds(bounds.min, bounds.max);

    // Render to framebuffer
    fb.bind();

    renderer.setCamera(camera);
    renderer.beginFrame(settings.backgroundColor);
    renderer.renderMesh(mesh, settings.materialTexture);
    renderer.endFrame();

    // Read pixels
    ByteBuffer pixels = fb.readPixels();

    fb.unbind();

    // Cleanup
    renderer.shutdown();

    return pixels;
}

} // namespace dw
