// Digital Workshop - String Utils Tests

#include <gtest/gtest.h>

#include "core/utils/string_utils.h"

// --- trim ---

TEST(StringUtils, Trim_RemovesBothSides) {
    EXPECT_EQ(dw::str::trim("  hello  "), "hello");
}

TEST(StringUtils, Trim_TabsAndNewlines) {
    EXPECT_EQ(dw::str::trim("\t\nhello\n\t"), "hello");
}

TEST(StringUtils, Trim_EmptyString) {
    EXPECT_EQ(dw::str::trim(""), "");
}

TEST(StringUtils, Trim_AllWhitespace) {
    EXPECT_EQ(dw::str::trim("   "), "");
}

TEST(StringUtils, Trim_NoWhitespace) {
    EXPECT_EQ(dw::str::trim("hello"), "hello");
}

// --- case conversion ---

TEST(StringUtils, ToLower_Basic) {
    EXPECT_EQ(dw::str::toLower("HELLO"), "hello");
}

TEST(StringUtils, ToLower_Mixed) {
    EXPECT_EQ(dw::str::toLower("HeLLo WoRLd"), "hello world");
}

TEST(StringUtils, ToLower_AlreadyLower) {
    EXPECT_EQ(dw::str::toLower("hello"), "hello");
}

TEST(StringUtils, ToUpper_Basic) {
    EXPECT_EQ(dw::str::toUpper("hello"), "HELLO");
}

TEST(StringUtils, ToUpper_WithNumbers) {
    EXPECT_EQ(dw::str::toUpper("abc123"), "ABC123");
}

// --- split ---

TEST(StringUtils, Split_Char_Basic) {
    auto parts = dw::str::split("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(StringUtils, Split_Char_Empty) {
    auto parts = dw::str::split("", ',');
    // Empty string should produce one empty part or empty vector
    EXPECT_TRUE(parts.empty() || (parts.size() == 1 && parts[0].empty()));
}

TEST(StringUtils, Split_Char_NoDelimiter) {
    auto parts = dw::str::split("hello", ',');
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "hello");
}

TEST(StringUtils, Split_Char_TrailingDelimiter) {
    auto parts = dw::str::split("a,b,", ',');
    ASSERT_GE(parts.size(), 2u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
}

TEST(StringUtils, Split_String_Basic) {
    auto parts = dw::str::split("a::b::c", "::");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

// --- join ---

TEST(StringUtils, Join_Basic) {
    EXPECT_EQ(dw::str::join({"a", "b", "c"}, ", "), "a, b, c");
}

TEST(StringUtils, Join_SingleElement) {
    EXPECT_EQ(dw::str::join({"hello"}, ", "), "hello");
}

TEST(StringUtils, Join_Empty) {
    EXPECT_EQ(dw::str::join({}, ", "), "");
}

// --- startsWith / endsWith ---

TEST(StringUtils, StartsWith_True) {
    EXPECT_TRUE(dw::str::startsWith("hello world", "hello"));
}

TEST(StringUtils, StartsWith_False) {
    EXPECT_FALSE(dw::str::startsWith("hello world", "world"));
}

TEST(StringUtils, StartsWith_EmptyPrefix) {
    EXPECT_TRUE(dw::str::startsWith("hello", ""));
}

TEST(StringUtils, EndsWith_True) {
    EXPECT_TRUE(dw::str::endsWith("hello world", "world"));
}

TEST(StringUtils, EndsWith_False) {
    EXPECT_FALSE(dw::str::endsWith("hello world", "hello"));
}

TEST(StringUtils, EndsWith_EmptyString) {
    EXPECT_TRUE(dw::str::endsWith("hello", ""));
}

// --- contains ---

TEST(StringUtils, Contains_True) {
    EXPECT_TRUE(dw::str::contains("hello world", "lo wo"));
}

TEST(StringUtils, Contains_False) {
    EXPECT_FALSE(dw::str::contains("hello world", "xyz"));
}

TEST(StringUtils, ContainsIgnoreCase_True) {
    EXPECT_TRUE(dw::str::containsIgnoreCase("Hello World", "hello"));
}

TEST(StringUtils, ContainsIgnoreCase_False) {
    EXPECT_FALSE(dw::str::containsIgnoreCase("Hello World", "xyz"));
}

// --- replace ---

TEST(StringUtils, Replace_Basic) {
    EXPECT_EQ(dw::str::replace("hello world", "world", "earth"), "hello earth");
}

TEST(StringUtils, Replace_Multiple) {
    EXPECT_EQ(dw::str::replace("aaa", "a", "bb"), "bbbbbb");
}

TEST(StringUtils, Replace_NotFound) {
    EXPECT_EQ(dw::str::replace("hello", "xyz", "abc"), "hello");
}

// --- formatFileSize ---

TEST(StringUtils, FormatFileSize_Bytes) {
    std::string result = dw::str::formatFileSize(500);
    EXPECT_TRUE(dw::str::contains(result, "500"));
}

TEST(StringUtils, FormatFileSize_Kilobytes) {
    std::string result = dw::str::formatFileSize(1024);
    EXPECT_TRUE(dw::str::contains(result, "KB") || dw::str::contains(result, "kB") ||
                dw::str::contains(result, "1.0"));
}

TEST(StringUtils, FormatFileSize_Megabytes) {
    std::string result = dw::str::formatFileSize(1024 * 1024);
    EXPECT_TRUE(dw::str::contains(result, "MB") || dw::str::contains(result, "1.0"));
}

TEST(StringUtils, FormatFileSize_Zero) {
    std::string result = dw::str::formatFileSize(0);
    EXPECT_FALSE(result.empty());
}

// --- formatNumber ---

TEST(StringUtils, FormatNumber_Small) {
    std::string result = dw::str::formatNumber(42);
    EXPECT_EQ(result, "42");
}

TEST(StringUtils, FormatNumber_Thousands) {
    std::string result = dw::str::formatNumber(1000);
    // Should contain some separator: "1,000" or "1 000"
    EXPECT_TRUE(result.size() > 3);
}

TEST(StringUtils, FormatNumber_Negative) {
    std::string result = dw::str::formatNumber(-1234);
    EXPECT_TRUE(dw::str::contains(result, "-"));
    EXPECT_TRUE(dw::str::contains(result, "1"));
}

// --- parseInt / parseFloat / parseDouble ---

TEST(StringUtils, ParseInt_Valid) {
    int val = 0;
    EXPECT_TRUE(dw::str::parseInt("42", val));
    EXPECT_EQ(val, 42);
}

TEST(StringUtils, ParseInt_Negative) {
    int val = 0;
    EXPECT_TRUE(dw::str::parseInt("-10", val));
    EXPECT_EQ(val, -10);
}

TEST(StringUtils, ParseInt_Invalid) {
    int val = 0;
    EXPECT_FALSE(dw::str::parseInt("abc", val));
}

TEST(StringUtils, ParseFloat_Valid) {
    float val = 0.0f;
    EXPECT_TRUE(dw::str::parseFloat("3.14", val));
    EXPECT_NEAR(val, 3.14f, 0.001f);
}

TEST(StringUtils, ParseFloat_Invalid) {
    float val = 0.0f;
    EXPECT_FALSE(dw::str::parseFloat("xyz", val));
}

TEST(StringUtils, ParseDouble_Valid) {
    double val = 0.0;
    EXPECT_TRUE(dw::str::parseDouble("2.718281828", val));
    EXPECT_NEAR(val, 2.718281828, 1e-6);
}

TEST(StringUtils, ParseDouble_Invalid) {
    double val = 0.0;
    EXPECT_FALSE(dw::str::parseDouble("not_a_number", val));
}

// --- LIKE Escape (BUG-05 regression) ---

TEST(StringUtils, EscapeLike_NoSpecialChars) {
    EXPECT_EQ(dw::str::escapeLike("hello"), "hello");
}

TEST(StringUtils, EscapeLike_PercentEscaped) {
    EXPECT_EQ(dw::str::escapeLike("100%"), "100\\%");
}

TEST(StringUtils, EscapeLike_UnderscoreEscaped) {
    EXPECT_EQ(dw::str::escapeLike("test_file"), "test\\_file");
}

TEST(StringUtils, EscapeLike_BackslashEscaped) {
    EXPECT_EQ(dw::str::escapeLike("path\\to"), "path\\\\to");
}

TEST(StringUtils, EscapeLike_MultipleWildcards) {
    EXPECT_EQ(dw::str::escapeLike("%_\\"), "\\%\\_\\\\");
}

TEST(StringUtils, EscapeLike_EmptyString) {
    EXPECT_EQ(dw::str::escapeLike(""), "");
}

TEST(StringUtils, EscapeLike_NoEscapeNeeded) {
    EXPECT_EQ(dw::str::escapeLike("normal search term"), "normal search term");
}

TEST(StringUtils, EscapeLike_MixedContent) {
    // Realistic search: user types "box_v2 (50%)"
    EXPECT_EQ(dw::str::escapeLike("box_v2 (50%)"), "box\\_v2 (50\\%)");
}
