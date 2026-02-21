#include "PluginEditor.h"
#include "PluginProcessor.h"

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p)
    , processorRef(p)
{
    setSize(600, 400);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void PluginEditor::resized()
{
}
