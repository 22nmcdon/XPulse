#include "SpectralMorphFXContent.h"

SpectralMorphFXContent::SpectralMorphFXContent(juce::AudioProcessorValueTreeState& apvts)
{
    titleLabel.setText("Spectral Morph FX", juce::dontSendNotification);
    addAndMakeVisible(titleLabel);
}

SpectralMorphFXContent::~SpectralMorphFXContent() {}

void SpectralMorphFXContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void SpectralMorphFXContent::resized()
{
    titleLabel.setBounds(10, 10, 200, 30);
}
