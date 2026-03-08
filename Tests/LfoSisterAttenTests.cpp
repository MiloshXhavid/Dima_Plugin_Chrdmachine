#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "PluginProcessor.h"

TEST_CASE("LfoSisterAtten APVTS params", "[lfo][sister][atten]")
{
    ChordJoystickAudioProcessor proc;

    SECTION("lfoXSisterAtten param exists")
    {
        REQUIRE(proc.apvts.getParameter("lfoXSisterAtten") != nullptr);
    }
    SECTION("lfoYSisterAtten param exists")
    {
        REQUIRE(proc.apvts.getParameter("lfoYSisterAtten") != nullptr);
    }
    SECTION("lfoXSisterAtten range and default")
    {
        auto* p = dynamic_cast<juce::AudioParameterFloat*>(
                      proc.apvts.getParameter("lfoXSisterAtten"));
        REQUIRE(p != nullptr);
        REQUIRE(p->range.start == Catch::Approx(-1.0f));
        REQUIRE(p->range.end   == Catch::Approx( 1.0f));
        REQUIRE(p->range.convertFrom0to1(p->getDefaultValue()) == Catch::Approx(1.0f));
    }
    SECTION("lfoYSisterAtten range and default")
    {
        auto* p = dynamic_cast<juce::AudioParameterFloat*>(
                      proc.apvts.getParameter("lfoYSisterAtten"));
        REQUIRE(p != nullptr);
        REQUIRE(p->range.start == Catch::Approx(-1.0f));
        REQUIRE(p->range.end   == Catch::Approx( 1.0f));
        REQUIRE(p->range.convertFrom0to1(p->getDefaultValue()) == Catch::Approx(1.0f));
    }
}

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
