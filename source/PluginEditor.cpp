#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // M1: Set window size to 400×600px (fixed, as per UI spec)
    // UI controls will be added in M5
    setSize (400, 600);
    setResizable (false, false);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // M1: Black chassis background (per UI spec)
    // Proper chassis color and textures will be added in M5
    g.fillAll (juce::Colours::black);

    // M1: Temporary title for testing
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawText ("MUSE", getLocalBounds().removeFromTop (80), juce::Justification::centred, false);

    g.setFont (12.0f);
    g.setColour (juce::Colours::lightgrey);
    auto infoArea = getLocalBounds().reduced (20);
    g.drawText ("M1: Scaffold - Parameters defined",
                infoArea.removeFromTop (120).removeFromBottom (20),
                juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    // M1: Simple layout for inspector button
    // Full UI layout will be implemented in M5
    auto area = getLocalBounds();
    inspectButton.setBounds (area.removeFromBottom (60).reduced (150, 10));
}
