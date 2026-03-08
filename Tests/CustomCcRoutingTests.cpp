#include <catch2/catch_test_macros.hpp>
// CustomCcRoutingHelpers.h created in Plan 02 (Wave 1).
#include "../Source/CustomCcRoutingHelpers.h"

TEST_CASE("ccDestToNumber - custom index 19 returns supplied value", "[cc-routing]")
{
    // CC-CUSTOM-01: index 19 = "Custom CC..." slot; must return customCcVal
    REQUIRE(ccDestToNumber(19, 42) == 42);
    REQUIRE(ccDestToNumber(19, 0)  == 0);
    REQUIRE(ccDestToNumber(19, 127) == 127);
}

TEST_CASE("ccDestToNumber - named CC indices unchanged", "[cc-routing]")
{
    // CC-CUSTOM-02: existing indices 0-18 must be unaffected
    REQUIRE(ccDestToNumber(0,  0) == -1);   // Off
    REQUIRE(ccDestToNumber(1,  0) == 1);    // CC1 Mod Wheel
    REQUIRE(ccDestToNumber(13, 0) == 74);   // CC74 Filter Cut
    REQUIRE(ccDestToNumber(18, 0) == 93);   // CC93 Chorus (last named)
}

TEST_CASE("resolveFilterCcMode - custom mode index 25 returns custom value", "[cc-routing]")
{
    // CC-CUSTOM-03: mode 25 = kNumLfoTargetModes(7) + kNumFilterCcModes(18) = custom slot
    // Must return custom value, NOT kFilterCcNums[18] (OOB)
    REQUIRE(resolveFilterCcMode(25, 42) == 42);

    // Existing named CC modes still work:
    REQUIRE(resolveFilterCcMode(7,  0) == 1);    // CC1 (index 0 in kFilterCcNums)
    REQUIRE(resolveFilterCcMode(24, 0) == 93);   // CC93 (index 17 in kFilterCcNums)

    // LFO/Gate target modes return -1 (not CC modes)
    REQUIRE(resolveFilterCcMode(0,  0) == -1);
    REQUIRE(resolveFilterCcMode(6,  0) == -1);
}
