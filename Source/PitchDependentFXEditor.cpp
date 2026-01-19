#include "PitchDependentFXEditor.h"
#include "PitchDependentFXContent.h"
#include "PluginProcessor.h"

PitchDependentFXEditor::PitchDependentFXEditor(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts)
    : juce::DocumentWindow("Pitch Dependent FX Editor", juce::Colours::lightgrey, juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    setSize(900, 300);
    setVisible(true);
    setContentOwned(new PitchDependentFXContent(processorRef, apvts), true);

}

PitchDependentFXEditor::~PitchDependentFXEditor() {}

void PitchDependentFXEditor::closeButtonPressed()
{
    setVisible(false); // Hide the window when the close button is pressed
}

