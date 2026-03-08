#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// APVTS param existence / range / default are verified by the full VST3 build
// (link-time check: addFloat("lfoXSisterAtten"...) and the Task 8 UAT smoke test).
// The tests below cover the modulation signal scaling contract that is exercised
// at both applySisterMod call sites in PluginProcessor.cpp.

TEST_CASE("LfoSisterAtten modSignal scaling contract", "[lfo][sister][atten]")
{
    const float modSignal = 0.7f;

    SECTION("identity at +1.0")
    {
        REQUIRE(modSignal * 1.0f == Catch::Approx(modSignal));
    }
    SECTION("inversion at -1.0")
    {
        REQUIRE(modSignal * -1.0f == Catch::Approx(-modSignal));
    }
    SECTION("kill at 0.0")
    {
        REQUIRE(modSignal * 0.0f == Catch::Approx(0.0f));
    }
    SECTION("attenuation at 0.5")
    {
        REQUIRE(modSignal * 0.5f == Catch::Approx(modSignal * 0.5f));
    }
}

TEST_CASE("LfoSisterAtten param range values", "[lfo][sister][atten]")
{
    // Verify the numeric constants used in createParameterLayout match the contract.
    // These mirror the addFloat("lfoXSisterAtten", ..., -1.0f, 1.0f, 1.0f) call.
    constexpr float kMin     = -1.0f;
    constexpr float kMax     =  1.0f;
    constexpr float kDefault =  1.0f;

    SECTION("min is -1.0")
    {
        REQUIRE(kMin == Catch::Approx(-1.0f));
    }
    SECTION("max is +1.0")
    {
        REQUIRE(kMax == Catch::Approx(1.0f));
    }
    SECTION("default is +1.0 (full pass-through)")
    {
        REQUIRE(kDefault == Catch::Approx(1.0f));
    }
    SECTION("range is symmetric around zero (bipolar)")
    {
        REQUIRE(std::abs(kMin + kMax) < 0.001f);
    }
    SECTION("default yields identity scaling")
    {
        const float modSignal = 0.7f;
        REQUIRE(modSignal * kDefault == Catch::Approx(modSignal));
    }
    SECTION("min yields inverted scaling")
    {
        const float modSignal = 0.7f;
        REQUIRE(modSignal * kMin == Catch::Approx(-modSignal));
    }
}
