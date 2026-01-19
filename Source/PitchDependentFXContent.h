#pragma once
#include <JuceHeader.h>
#include "LowBandWindow.h"
#include "MidBandWindow.h"
#include "HighBandWindow.h"
#include "PluginProcessor.h"


class PitchDependentFXContent : public juce::Component
{
public:
    PitchDependentFXContent(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);
    ~PitchDependentFXContent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
	//Band Windows
    LowBandWindow lowBandWindow;
    MidBandWindow midBandWindow;
    HighBandWindow highBandWindow;

    //Buttons to open Band Windows
	juce::TextButton lowBandButton, midBandButton, highBandButton;

    juce::Label titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDependentFXContent)
};
