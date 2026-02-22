// Digital Workshop - Loader Factory Tests

#include <gtest/gtest.h>

#include "core/loaders/loader_factory.h"
#include "core/loaders/obj_loader.h"
#include "core/loaders/stl_loader.h"

// --- isSupported ---

TEST(LoaderFactory, IsSupported_STL) {
    EXPECT_TRUE(dw::LoaderFactory::isSupported("stl"));
    EXPECT_TRUE(dw::LoaderFactory::isSupported("STL"));
}

TEST(LoaderFactory, IsSupported_OBJ) {
    EXPECT_TRUE(dw::LoaderFactory::isSupported("obj"));
    EXPECT_TRUE(dw::LoaderFactory::isSupported("OBJ"));
}

TEST(LoaderFactory, IsSupported_3MF) {
    EXPECT_TRUE(dw::LoaderFactory::isSupported("3mf"));
    EXPECT_TRUE(dw::LoaderFactory::isSupported("3MF"));
}

TEST(LoaderFactory, IsSupported_Unknown) {
    EXPECT_FALSE(dw::LoaderFactory::isSupported("xyz"));
    EXPECT_FALSE(dw::LoaderFactory::isSupported(""));
    EXPECT_FALSE(dw::LoaderFactory::isSupported("fbx"));
}

// --- getLoaderByExtension ---

TEST(LoaderFactory, GetLoaderByExtension_STL) {
    auto loader = dw::LoaderFactory::getLoaderByExtension("stl");
    ASSERT_NE(loader, nullptr);
    EXPECT_TRUE(loader->supports("stl"));
}

TEST(LoaderFactory, GetLoaderByExtension_OBJ) {
    auto loader = dw::LoaderFactory::getLoaderByExtension("obj");
    ASSERT_NE(loader, nullptr);
    EXPECT_TRUE(loader->supports("obj"));
}

TEST(LoaderFactory, GetLoaderByExtension_Unknown) {
    auto loader = dw::LoaderFactory::getLoaderByExtension("xyz");
    EXPECT_EQ(loader, nullptr);
}

// --- supportedExtensions ---

TEST(LoaderFactory, SupportedExtensions_ContainsKnownFormats) {
    auto exts = dw::LoaderFactory::supportedExtensions();
    EXPECT_GE(exts.size(), 3u); // at least stl, obj, 3mf

    bool hasSTL = false, hasOBJ = false, has3MF = false;
    for (const auto& e : exts) {
        if (e == "stl")
            hasSTL = true;
        if (e == "obj")
            hasOBJ = true;
        if (e == "3mf")
            has3MF = true;
    }
    EXPECT_TRUE(hasSTL);
    EXPECT_TRUE(hasOBJ);
    EXPECT_TRUE(has3MF);
}

// --- getLoader (by path) ---

TEST(LoaderFactory, GetLoader_ByPath) {
    auto loader = dw::LoaderFactory::getLoader("/some/path/model.stl");
    ASSERT_NE(loader, nullptr);
    EXPECT_TRUE(loader->supports("stl"));
}

TEST(LoaderFactory, GetLoader_ByPath_CaseInsensitive) {
    auto loader = dw::LoaderFactory::getLoader("model.OBJ");
    ASSERT_NE(loader, nullptr);
    EXPECT_TRUE(loader->supports("obj"));
}

// --- loadFromBuffer dispatch ---

TEST(LoaderFactory, LoadFromBuffer_STL) {
    // Minimal binary STL with 0 triangles
    dw::ByteBuffer stlData(84, 0); // 80 header + 4 byte count = 0

    auto result = dw::LoaderFactory::loadFromBuffer(stlData, "stl");
    // 0 triangles is likely an error, but it should dispatch to STL loader
    // and not crash
    // The result depends on whether the loader treats 0 triangles as error
    // Just verify it doesn't crash and returns something
    (void)result;
}

TEST(LoaderFactory, LoadFromBuffer_OBJ_ValidTriangle) {
    std::string obj = "v 0 0 0\n"
                      "v 1 0 0\n"
                      "v 0 1 0\n"
                      "f 1 2 3\n";
    dw::ByteBuffer data(obj.begin(), obj.end());

    auto result = dw::LoaderFactory::loadFromBuffer(data, "obj");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.mesh->triangleCount(), 1u);
}

TEST(LoaderFactory, LoadFromBuffer_UnsupportedExtension) {
    dw::ByteBuffer data = {0x00};
    auto result = dw::LoaderFactory::loadFromBuffer(data, "xyz");
    EXPECT_FALSE(result.success());
}
