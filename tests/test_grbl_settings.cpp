// Tests for GrblSettings — parsing, validation, JSON round-trip
#include <gtest/gtest.h>

#include "core/cnc/grbl_settings.h"

using namespace dw;

// --- Single line parsing ---

TEST(GrblSettings, ParseSingleLine) {
    GrblSettings settings;
    EXPECT_TRUE(settings.parseLine("$0=10"));
    const auto* s = settings.get(0);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->id, 0);
    EXPECT_FLOAT_EQ(s->value, 10.0f);
    EXPECT_EQ(s->description, "Step pulse time");
    EXPECT_EQ(s->units, "microseconds");
}

TEST(GrblSettings, ParseLineWithFloat) {
    GrblSettings settings;
    EXPECT_TRUE(settings.parseLine("$11=0.010"));
    const auto* s = settings.get(11);
    ASSERT_NE(s, nullptr);
    EXPECT_FLOAT_EQ(s->value, 0.01f);
    EXPECT_EQ(s->description, "Junction deviation");
    EXPECT_EQ(s->units, "mm");
}

TEST(GrblSettings, ParseLineRejectsInvalid) {
    GrblSettings settings;
    EXPECT_FALSE(settings.parseLine(""));
    EXPECT_FALSE(settings.parseLine("ok"));
    EXPECT_FALSE(settings.parseLine("$"));
    EXPECT_FALSE(settings.parseLine("$=10"));
    EXPECT_FALSE(settings.parseLine("$abc=10"));
    EXPECT_FALSE(settings.parseLine("$0=abc"));
    EXPECT_TRUE(settings.empty());
}

TEST(GrblSettings, ParseLineUnknownSetting) {
    GrblSettings settings;
    EXPECT_TRUE(settings.parseLine("$200=42"));
    const auto* s = settings.get(200);
    ASSERT_NE(s, nullptr);
    EXPECT_FLOAT_EQ(s->value, 42.0f);
    EXPECT_EQ(s->description, "Unknown setting");
}

// --- Full $$ response parsing ---

TEST(GrblSettings, ParseFullResponse) {
    const std::string response =
        "$0=10\n"
        "$1=25\n"
        "$2=0\n"
        "$3=0\n"
        "$4=0\n"
        "$5=0\n"
        "$6=0\n"
        "$10=1\n"
        "$11=0.010\n"
        "$12=0.002\n"
        "$13=0\n"
        "$20=0\n"
        "$21=0\n"
        "$22=0\n"
        "$23=0\n"
        "$24=25.000\n"
        "$25=500.000\n"
        "$26=250\n"
        "$27=1.000\n"
        "$30=1000\n"
        "$31=0\n"
        "$32=0\n"
        "$100=250.000\n"
        "$101=250.000\n"
        "$102=250.000\n"
        "$110=500.000\n"
        "$111=500.000\n"
        "$112=500.000\n"
        "$120=10.000\n"
        "$121=10.000\n"
        "$122=10.000\n"
        "$130=200.000\n"
        "$131=200.000\n"
        "$132=200.000\n";

    GrblSettings settings;
    int count = settings.parseSettingsResponse(response);
    EXPECT_EQ(count, 34);

    // Spot check several values
    EXPECT_FLOAT_EQ(settings.get(0)->value, 10.0f);
    EXPECT_FLOAT_EQ(settings.get(11)->value, 0.01f);
    EXPECT_FLOAT_EQ(settings.get(100)->value, 250.0f);
    EXPECT_FLOAT_EQ(settings.get(130)->value, 200.0f);
}

TEST(GrblSettings, ParseResponseIgnoresNonSettingLines) {
    const std::string response =
        "[VER:1.1h:20190825]\n"
        "[OPT:V,15,128]\n"
        "$0=10\n"
        "$1=25\n"
        "ok\n";

    GrblSettings settings;
    int count = settings.parseSettingsResponse(response);
    EXPECT_EQ(count, 2);
}

TEST(GrblSettings, ParseResponseHandlesCRLF) {
    const std::string response = "$0=10\r\n$1=25\r\n";
    GrblSettings settings;
    int count = settings.parseSettingsResponse(response);
    EXPECT_EQ(count, 2);
    EXPECT_FLOAT_EQ(settings.get(0)->value, 10.0f);
    EXPECT_FLOAT_EQ(settings.get(1)->value, 25.0f);
}

// --- Validation ---

TEST(GrblSettings, SetValidValue) {
    GrblSettings settings;
    settings.parseLine("$0=10");
    EXPECT_TRUE(settings.set(0, 50.0f));
    EXPECT_FLOAT_EQ(settings.get(0)->value, 50.0f);
    EXPECT_TRUE(settings.get(0)->modified);
}

TEST(GrblSettings, SetRejectsOutOfRange) {
    GrblSettings settings;
    settings.parseLine("$0=10");
    EXPECT_FALSE(settings.set(0, 2.0f));   // min is 3
    EXPECT_FALSE(settings.set(0, 256.0f)); // max is 255
    EXPECT_FLOAT_EQ(settings.get(0)->value, 10.0f); // unchanged
}

TEST(GrblSettings, SetRejectsInvalidBoolean) {
    GrblSettings settings;
    settings.parseLine("$4=0");
    EXPECT_FALSE(settings.set(4, 2.0f));  // boolean: only 0 or 1
    EXPECT_TRUE(settings.set(4, 1.0f));
    EXPECT_FLOAT_EQ(settings.get(4)->value, 1.0f);
}

TEST(GrblSettings, SetCreatesNewSetting) {
    GrblSettings settings;
    EXPECT_TRUE(settings.set(200, 42.0f)); // unknown setting, no range check
    const auto* s = settings.get(200);
    ASSERT_NE(s, nullptr);
    EXPECT_FLOAT_EQ(s->value, 42.0f);
    EXPECT_TRUE(s->modified);
}

// --- Grouping ---

TEST(GrblSettings, GroupAssignment) {
    EXPECT_EQ(grblSettingGroup(0), GrblSettingGroup::General);
    EXPECT_EQ(grblSettingGroup(6), GrblSettingGroup::General);
    EXPECT_EQ(grblSettingGroup(10), GrblSettingGroup::Motion);
    EXPECT_EQ(grblSettingGroup(22), GrblSettingGroup::Limits);
    EXPECT_EQ(grblSettingGroup(30), GrblSettingGroup::Spindle);
    EXPECT_EQ(grblSettingGroup(100), GrblSettingGroup::StepsPerMm);
    EXPECT_EQ(grblSettingGroup(110), GrblSettingGroup::FeedRates);
    EXPECT_EQ(grblSettingGroup(120), GrblSettingGroup::Acceleration);
    EXPECT_EQ(grblSettingGroup(130), GrblSettingGroup::MaxTravel);
    EXPECT_EQ(grblSettingGroup(200), GrblSettingGroup::Unknown);
}

TEST(GrblSettings, GetGroupedReturnsCorrectOrder) {
    GrblSettings settings;
    settings.parseLine("$130=200");
    settings.parseLine("$0=10");
    settings.parseLine("$110=500");

    auto grouped = settings.getGrouped();
    ASSERT_EQ(grouped.size(), 3u);
    EXPECT_EQ(grouped[0].first, GrblSettingGroup::General);
    EXPECT_EQ(grouped[1].first, GrblSettingGroup::FeedRates);
    EXPECT_EQ(grouped[2].first, GrblSettingGroup::MaxTravel);
}

// --- JSON round-trip ---

TEST(GrblSettings, JsonRoundTrip) {
    GrblSettings original;
    original.parseLine("$0=10");
    original.parseLine("$1=25");
    original.parseLine("$11=0.010");
    original.parseLine("$100=250.000");
    original.parseLine("$110=500.000");

    std::string json = original.toJsonString();
    EXPECT_FALSE(json.empty());

    GrblSettings restored;
    EXPECT_TRUE(restored.fromJsonString(json));

    // Verify all values match
    for (const auto& [id, setting] : original.getAll()) {
        const auto* r = restored.get(id);
        ASSERT_NE(r, nullptr) << "Missing setting $" << id;
        EXPECT_FLOAT_EQ(r->value, setting.value) << "Mismatch for $" << id;
        EXPECT_EQ(r->description, setting.description);
    }
}

TEST(GrblSettings, FromJsonRejectsInvalid) {
    GrblSettings settings;
    EXPECT_FALSE(settings.fromJsonString(""));
    EXPECT_FALSE(settings.fromJsonString("{}"));
    EXPECT_FALSE(settings.fromJsonString("{\"settings\": \"not_array\"}"));
}

TEST(GrblSettings, FromJsonIgnoresItemsWithoutId) {
    GrblSettings settings;
    EXPECT_TRUE(settings.fromJsonString(
        R"({"settings": [{"value": 10}, {"id": 1, "value": 25}]})"));
    EXPECT_EQ(settings.getAll().size(), 1u);
    EXPECT_NE(settings.get(1), nullptr);
}

// --- Diff ---

TEST(GrblSettings, DiffDetectsChanges) {
    GrblSettings a, b;
    a.parseLine("$0=10");
    a.parseLine("$1=25");
    b.parseLine("$0=10");
    b.parseLine("$1=30");

    auto diffs = a.diff(b);
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].first.id, 1);  // current
    EXPECT_FLOAT_EQ(diffs[0].first.value, 25.0f);
    EXPECT_FLOAT_EQ(diffs[0].second.value, 30.0f); // other
}

TEST(GrblSettings, DiffDetectsNewSettings) {
    GrblSettings a, b;
    a.parseLine("$0=10");
    b.parseLine("$0=10");
    b.parseLine("$1=25"); // only in b

    auto diffs = a.diff(b);
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].second.id, 1);
}

// --- Command building ---

TEST(GrblSettings, BuildSetCommandInteger) {
    std::string cmd = GrblSettings::buildSetCommand(0, 10.0f);
    EXPECT_EQ(cmd, "$0=10\n");
}

TEST(GrblSettings, BuildSetCommandFloat) {
    std::string cmd = GrblSettings::buildSetCommand(11, 0.01f);
    EXPECT_EQ(cmd, "$11=0.010\n");
}

TEST(GrblSettings, BuildSetCommandLargeValue) {
    std::string cmd = GrblSettings::buildSetCommand(110, 5000.0f);
    EXPECT_EQ(cmd, "$110=5000\n");
}

// --- Clear and empty ---

TEST(GrblSettings, ClearRemovesAll) {
    GrblSettings settings;
    settings.parseLine("$0=10");
    EXPECT_FALSE(settings.empty());
    settings.clear();
    EXPECT_TRUE(settings.empty());
    EXPECT_EQ(settings.get(0), nullptr);
}

// --- Metadata applied correctly ---

TEST(GrblSettings, MetadataAppliedOnParse) {
    GrblSettings settings;
    settings.parseLine("$24=25.000");
    const auto* s = settings.get(24);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->description, "Homing locate feed rate");
    EXPECT_EQ(s->units, "mm/min");
    EXPECT_FLOAT_EQ(s->min, 1.0f);
    EXPECT_FLOAT_EQ(s->max, 10000.0f);
    EXPECT_FALSE(s->isBitmask);
    EXPECT_FALSE(s->isBoolean);
}

TEST(GrblSettings, BooleanMetadata) {
    GrblSettings settings;
    settings.parseLine("$22=1");
    const auto* s = settings.get(22);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->isBoolean);
    EXPECT_EQ(s->description, "Homing cycle enable");
}

TEST(GrblSettings, BitmaskMetadata) {
    GrblSettings settings;
    settings.parseLine("$2=3");
    const auto* s = settings.get(2);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->isBitmask);
    EXPECT_EQ(s->description, "Step port invert mask");
}
