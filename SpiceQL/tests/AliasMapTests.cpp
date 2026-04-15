#include <gtest/gtest.h>
#include "Fixtures.h"
#include "alias_map.h"

using namespace SpiceQL;

TEST_F(AliasMapTest, DefaultLoad) {
	// Test the default load from source tree
	load_aliases(defaultAliasMapFile.string());
	EXPECT_EQ(get_mission("LRO_LROCNACL"), "lroc");
}

TEST_F(AliasMapTest, LoadFromSpecificPath) {
	// Verify test file exists
	ASSERT_TRUE(fs::exists(testAliasMapFile)) 
			<< "Test data file not found at: " << testAliasMapFile;
	EXPECT_NO_THROW(load_aliases(testAliasMapFile.string()));
	EXPECT_EQ(get_mission("Fake"), "test_mission");
	EXPECT_THROW(get_mission("MRO"), std::invalid_argument);
	EXPECT_EQ(get_aliases().size(), 2);
}

TEST_F(AliasMapTest, InvalidAlias) {
	// Test error handling for missing alias
	load_aliases(testAliasMapFile.string());
	EXPECT_THROW(get_mission("bad_alias"), std::invalid_argument);
}

TEST_F(AliasMapTest, EnsureInit) {
	// Get mission without explicit loading, will load default automatically
	setenv("SPICEQL_DEV_ALIAS", "true", 1);
	EXPECT_EQ(get_mission("CH2_OHRC"), "ohrc");
}

TEST_F(AliasMapTest, PathParamOverridesEnvironment) {
	setenv("SPICEQL_DEV_ALIAS", "true", 1);
	EXPECT_EQ(get_mission("CH2_TMC_NADIR"), "tmc2");

	// load test alias map
	EXPECT_NO_THROW(load_aliases(testAliasMapFile.string()));
	EXPECT_THROW(get_mission("CH2_TMC_NADIR"), std::invalid_argument);
}

