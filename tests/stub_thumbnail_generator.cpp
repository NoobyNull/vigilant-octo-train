// Stub for ThumbnailGenerator — avoids GL dependency in tests
// The real implementation is in src/render/thumbnail_generator.cpp and requires OpenGL.

#include "render/thumbnail_generator.h"

namespace dw {

ThumbnailGenerator::~ThumbnailGenerator() {}
bool ThumbnailGenerator::initialize() {
    return false;
}
void ThumbnailGenerator::shutdown() {}
bool ThumbnailGenerator::generate(const Mesh&, const Path&, const ThumbnailSettings&) {
    return false;
}
ByteBuffer ThumbnailGenerator::generateToBuffer(const Mesh&, const ThumbnailSettings&) {
    return {};
}

} // namespace dw
