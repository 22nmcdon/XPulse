#include "SpectralMorphFXEditor.h"

SpectralMorphFXEditor::SpectralMorphFXEditor(juce::AudioProcessorValueTreeState& apvts)
    : juce::DocumentWindow("Spectral Morph FX Editor", juce::Colours::lightgrey, juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    setSize(900, 300);
    setVisible(true);
}

SpectralMorphFXEditor::~SpectralMorphFXEditor() {}

void SpectralMorphFXEditor::closeButtonPressed()
{
    setVisible(false); // Hide the window when the close button is pressed
}

