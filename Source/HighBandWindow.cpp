#include "HighBandWindow.h"
#include "PluginProcessor.h"

HighBandWindow::HighBandWindow(XPulseAudioProcessor &processorRef, juce::AudioProcessorValueTreeState& apvts)
	: processorRef(processorRef), apvtsRef(apvts)
{
	//Add Sliders
    addAndMakeVisible(highBandGainSlider);
    addAndMakeVisible(highBandReverbSlider);

	//Add ComboBox
	addAndMakeVisible(highReverbBox);

    // Attach the slider to the APVTS parameter
    highBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "highGain", highBandGainSlider);
    highBandReverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "highReverbMaster", highBandReverbSlider);

	//Attach the ComboBox to the APVTS parameter
	highReverbBoxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "highReverbPreset", highReverbBox);

	// Reverb Slider Listener
    highBandReverbSlider.onValueChange = [this, &processorRef]()
        {
            float master = highBandReverbSlider.getValue();
            processorRef.highBandReverbParameters.wetLevel = processorRef.highBaseWetLevel * master;
            processorRef.highBandReverbParameters.damping = processorRef.highBaseDamping * master;
            processorRef.highBandReverbParameters.width = processorRef.highBaseWidth * master;
            processorRef.highBandReverbParameters.roomSize = processorRef.highBaseRoomSize;
            processorRef.highBandReverbParameters.dryLevel = processorRef.highBaseDryLevel;
            processorRef.highBandReverbParameters.freezeMode = processorRef.highBaseFreezeMode;
            processorRef.highBandReverbProcessor.setParameters(processorRef.highBandReverbParameters);
	    };
    
	//ComboBox Listener
    highReverbBox.onChange = [this, &processorRef]()
       {
           int presetIndex = highReverbBox.getSelectedItemIndex();
           processorRef.loadReverbPreset(presetIndex, 'h');
		};

	// Populate ComboBox with reverb presets
    highReverbBox.addItem("Small Room", 1);
	highReverbBox.addItem("Concert Hall", 2);
	highReverbBox.addItem("Dark Room", 3);
	highReverbBox.addItem("Bright Room", 4);
	highReverbBox.addItem("Ambient Room", 5);

    setSize(400, 200);
}

HighBandWindow::~HighBandWindow() {}

void HighBandWindow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
    // Add custom drawing here
}

void HighBandWindow::resized()
{
    // Layout child components here
	highBandGainSlider.setBounds(10, 10, 200, 30);
    highBandReverbSlider.setBounds(10, 30, 200, 30);
	highReverbBox.setBounds(220, 30, 150, 30);
}
