#include <gtest/gtest.h>

#include "core/paths/path_resolver.h"
#include "core/config/config.h"

using namespace dw;

TEST(PathResolver, AbsolutePathPassesThrough) {
    Path abs("/some/absolute/path/file.stl");
    Path result = PathResolver::resolve(abs, PathCategory::Support);
    EXPECT_EQ(result, abs);
}

TEST(PathResolver, RelativePathGetsResolved) {
    Path rel("ab/cd/abcd1234.stl");
    Path result = PathResolver::resolve(rel, PathCategory::Support);
    // Should be category root + relative
    EXPECT_TRUE(result.is_absolute());
    EXPECT_TRUE(result.string().find("abcd1234.stl") != std::string::npos);
}

TEST(PathResolver, EmptyPathReturnsEmpty) {
    Path empty;
    EXPECT_TRUE(PathResolver::resolve(empty, PathCategory::Models).empty());
    EXPECT_TRUE(PathResolver::makeStorable(empty, PathCategory::Models).empty());
}

TEST(PathResolver, MakeStorableInsideRoot) {
    Path root = PathResolver::categoryRoot(PathCategory::Support);
    Path absFile = root / "ab" / "cd" / "test.stl";
    Path stored = PathResolver::makeStorable(absFile, PathCategory::Support);
    EXPECT_TRUE(stored.is_relative());
    EXPECT_EQ(stored, Path("ab/cd/test.stl"));
}

TEST(PathResolver, MakeStorableOutsideRoot) {
    Path absFile("/completely/different/location/file.stl");
    Path stored = PathResolver::makeStorable(absFile, PathCategory::Support);
    EXPECT_TRUE(stored.is_absolute());
    EXPECT_EQ(stored, absFile);
}

TEST(PathResolver, RoundTrip) {
    Path root = PathResolver::categoryRoot(PathCategory::GCode);
    Path absFile = root / "myfile.nc";

    Path stored = PathResolver::makeStorable(absFile, PathCategory::GCode);
    EXPECT_TRUE(stored.is_relative());

    Path resolved = PathResolver::resolve(stored, PathCategory::GCode);
    EXPECT_EQ(resolved, absFile);
}

TEST(PathResolver, CategoryRootsAreAbsolute) {
    EXPECT_TRUE(PathResolver::categoryRoot(PathCategory::Models).is_absolute());
    EXPECT_TRUE(PathResolver::categoryRoot(PathCategory::Projects).is_absolute());
    EXPECT_TRUE(PathResolver::categoryRoot(PathCategory::Materials).is_absolute());
    EXPECT_TRUE(PathResolver::categoryRoot(PathCategory::GCode).is_absolute());
    EXPECT_TRUE(PathResolver::categoryRoot(PathCategory::Support).is_absolute());
}
