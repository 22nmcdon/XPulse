#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"


class HighBandWindow : public juce::Component
{
public:

    HighBandWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);
    ~HighBandWindow() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
	//Reference to the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& apvtsRef;
    XPulseAudioProcessor& processorRef;

	// Slider attachments

    //Gain
    juce::Slider highBandGainSlider;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highBandGainAttachment;
    
    //Reverb
    juce::Slider highBandReverbSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highBandReverbAttachment;

    //ComboBox
	juce::ComboBox highReverbBox;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> highReverbBoxAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HighBandWindow)
};
