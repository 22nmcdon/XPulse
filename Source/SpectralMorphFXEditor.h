#pragma once
#include <JuceHeader.h>

class SpectralMorphFXEditor : public juce::DocumentWindow
{
public:
    SpectralMorphFXEditor(juce::AudioProcessorValueTreeState& apvts);
    ~SpectralMorphFXEditor() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralMorphFXEditor)
};
