#include "PitchDependentFXContent.h"
#include "LowBandWindow.h"
#include "MidBandWindow.h"
#include "HighBandWindow.h"
#include "PluginProcessor.h"



PitchDependentFXContent::PitchDependentFXContent(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts) 
    : lowBandWindow(processorRef, apvts), midBandWindow(processorRef, apvts), highBandWindow(processorRef, apvts)
{
    titleLabel.setText("Pitch Dependent FX", juce::dontSendNotification);
    addAndMakeVisible(titleLabel);
    setSize(900, 300);

    // Add buttons
    lowBandButton.setButtonText("Low Band");
    lowBandButton.onClick = [this]() {
        lowBandWindow.setVisible(true);
        midBandWindow.setVisible(false);
        highBandWindow.setVisible(false);
        };
    addAndMakeVisible(lowBandButton);

    midBandButton.setButtonText("Mid Band");
    midBandButton.onClick = [this]() {
        lowBandWindow.setVisible(false);
        midBandWindow.setVisible(true);
        highBandWindow.setVisible(false);
        };
    addAndMakeVisible(midBandButton);

    highBandButton.setButtonText("High Band");
    highBandButton.onClick = [this]() {
        lowBandWindow.setVisible(false);
        midBandWindow.setVisible(false);
        highBandWindow.setVisible(true);
        };
    addAndMakeVisible(highBandButton);

    // Add band windows as children, but only one will be visible at a time
    addAndMakeVisible(lowBandWindow);
    addAndMakeVisible(midBandWindow);
    addAndMakeVisible(highBandWindow);

    // Start with only one visible
    lowBandWindow.setVisible(true);
    midBandWindow.setVisible(false);
    highBandWindow.setVisible(false);
}
PitchDependentFXContent::~PitchDependentFXContent() {}


void PitchDependentFXContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);

}

void PitchDependentFXContent::resized()
{
    titleLabel.setBounds(10, 10, 200, 30);
    lowBandButton.setBounds(10, 60, 100, 30);
    midBandButton.setBounds(120, 60, 100, 30);
    highBandButton.setBounds(230, 60, 100, 30);

    // Position the band window component(s)
    lowBandWindow.setBounds(10, 100, 400, 200);
    midBandWindow.setBounds(10, 100, 400, 200);
    highBandWindow.setBounds(10, 100, 400, 200);
}



