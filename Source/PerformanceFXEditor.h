#pragma once
#include <JuceHeader.h>
//Deprecated

class PerformanceFXEditor : public juce::DocumentWindow
{
public:
    PerformanceFXEditor(juce::AudioProcessorValueTreeState& apvts);
    ~PerformanceFXEditor() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceFXEditor)
};
