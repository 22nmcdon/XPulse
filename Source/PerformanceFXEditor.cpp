#include "PerformanceFXEditor.h"

PerformanceFXEditor::PerformanceFXEditor(juce::AudioProcessorValueTreeState& apvts)
    : juce::DocumentWindow("Performance FX Editor", juce::Colours::lightgrey, juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    setSize(900, 300);
    setVisible(true);
}

PerformanceFXEditor::~PerformanceFXEditor() {}

void PerformanceFXEditor::closeButtonPressed()
{
    setVisible(false); // Hide the window when the close button is pressed
}

