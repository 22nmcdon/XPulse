#include "LowBandWindow.h"
#include "PluginProcessor.h"

LowBandWindow::LowBandWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts)
    : processorRef(processorRef), apvtsRef(apvts)
{
	// Add Sliders 
	addAndMakeVisible(lowBandGainSlider);
    addAndMakeVisible(lowBandReverbSlider);

	// Add ComboBox
	addAndMakeVisible(lowReverbBox);

	
	// Attach components to the APVTS parameter
    lowBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lowGain", lowBandGainSlider);
    lowBandReverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lowReverbMaster", lowBandReverbSlider);

	// Attach ComboBox to the APVTS parameter
	lowReverbBoxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "lowReverbPreset", lowReverbBox);

	// Reverb Slider Listener
    lowBandReverbSlider.onValueChange = [this, &processorRef]()
        {
            float master = lowBandReverbSlider.getValue();
            processorRef.lowBandReverbParameters.wetLevel = processorRef.lowBaseWetLevel * master;
            processorRef.lowBandReverbParameters.damping = processorRef.lowBaseDamping * master;
            processorRef.lowBandReverbParameters.width = processorRef.lowBaseWidth * master;
            processorRef.lowBandReverbParameters.roomSize = processorRef.lowBaseRoomSize;
            processorRef.lowBandReverbParameters.dryLevel = processorRef.lowBaseDryLevel;
            processorRef.lowBandReverbParameters.freezeMode = processorRef.lowBaseFreezeMode;
            processorRef.lowBandReverbProcessor.setParameters(processorRef.lowBandReverbParameters);
        };

	// ComboBox Listener
    lowReverbBox.onChange = [this, &processorRef]()
       {
           int presetIndex = lowReverbBox.getSelectedItemIndex();
           processorRef.loadReverbPreset(presetIndex, 'l');
		};

	//Populate ComboBox with reverb presets
	lowReverbBox.addItem("Small Room", 1);
	lowReverbBox.addItem("Concert Hall", 2);
	lowReverbBox.addItem("Dark Room", 3);
	lowReverbBox.addItem("Bright Room", 4);
	lowReverbBox.addItem("Ambient Room", 5);


    setSize(400, 200);
}

LowBandWindow::~LowBandWindow() {}

void LowBandWindow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
    // Add custom drawing here
}

void LowBandWindow::resized()
{
    // Layout child components here
	lowBandGainSlider.setBounds(10, 10, 200, 30);
    lowBandReverbSlider.setBounds(10, 30, 200, 30);
	lowReverbBox.setBounds(220, 30, 150, 30);
}
