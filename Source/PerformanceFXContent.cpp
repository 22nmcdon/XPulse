#include "PerformanceFXContent.h"

PerformanceFXContent::PerformanceFXContent(juce::AudioProcessorValueTreeState& apvts)
{
    titleLabel.setText("Performance FX", juce::dontSendNotification);
    addAndMakeVisible(titleLabel);
}

PerformanceFXContent::~PerformanceFXContent() {}

void PerformanceFXContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void PerformanceFXContent::resized()
{
    titleLabel.setBounds(10, 10, 200, 30);
}
