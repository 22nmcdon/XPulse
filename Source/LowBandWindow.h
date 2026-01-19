#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class LowBandWindow : public juce::Component
{
public:
	//Attaches the APVTS to the LowBandWindow
    LowBandWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);

    ~LowBandWindow() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //Reference to the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& apvtsRef;
    XPulseAudioProcessor& processorRef;

    //Slider Attachments

    //Gain
    juce::Slider lowBandGainSlider;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowBandGainAttachment;

    //Reverb
    juce::Slider lowBandReverbSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowBandReverbAttachment;

    //ComboBox
	juce::ComboBox lowReverbBox;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowReverbBoxAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowBandWindow)
};
