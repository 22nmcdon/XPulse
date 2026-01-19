#pragma once
#include <JuceHeader.h>
#include "PitchDependentFXContent.h"
#include "HighBandWindow.h"
#include "MidBandWindow.h"
#include "LowBandWindow.h"
#include "PluginProcessor.h"


class PitchDependentFXEditor : public juce::DocumentWindow
{
public:
    PitchDependentFXEditor(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);
    ~PitchDependentFXEditor() override;

    void closeButtonPressed() override;

private:
    std::unique_ptr<PitchDependentFXContent> pitchDependentContext;
    std::unique_ptr<HighBandWindow> highBandWindow;
    std::unique_ptr<MidBandWindow> midBandWindow;
    std::unique_ptr<LowBandWindow> lowBandWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDependentFXEditor)
};
