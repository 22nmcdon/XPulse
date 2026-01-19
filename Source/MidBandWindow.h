#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"


class MidBandWindow : public juce::Component
{
public:
    MidBandWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);
    ~MidBandWindow() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    //Reference to the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& apvtsRef;
    XPulseAudioProcessor& processorRef;

	// Attachments

    //Gain
    juce::Slider midBandGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midBandGainAttachment;

    //Reverb
    juce::Slider midBandReverbSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midBandReverbAttachment;

    //ComboBox
    juce::ComboBox midReverbBox;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> midReverbBoxAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidBandWindow)
};
