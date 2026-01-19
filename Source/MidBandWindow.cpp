#include "MidBandWindow.h"
#include "PluginProcessor.h"


MidBandWindow::MidBandWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts)
    : processorRef(processorRef), apvtsRef(apvts)
{
	// Add the Sliders
    addAndMakeVisible(midBandGainSlider);
    addAndMakeVisible(midBandReverbSlider);

	//Add the ComboBox
	addAndMakeVisible(midReverbBox);

     
    // Attach the slider to the APVTS parameter
    midBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "midGain", midBandGainSlider);
    midBandReverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "midReverbMaster", midBandReverbSlider);

	// Attach the ComboBox to the APVTS parameter
	midReverbBoxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "midReverbPreset", midReverbBox);

	// Reverb Slider Listener
   midBandReverbSlider.onValueChange = [this, &processorRef]()
        {
		   float master = midBandReverbSlider.getValue();
		   processorRef.midBandReverbParameters.wetLevel = processorRef.midBaseWetLevel * master;
		   processorRef.midBandReverbParameters.damping = processorRef.midBaseDamping * master;
		   processorRef.midBandReverbParameters.width = processorRef.midBaseWidth * master;
		   processorRef.midBandReverbParameters.roomSize = processorRef.midBaseRoomSize;
		   processorRef.midBandReverbParameters.dryLevel = processorRef.midBaseDryLevel;
		   processorRef.midBandReverbParameters.freezeMode = processorRef.midBaseFreezeMode;
		   processorRef.midBandReverbProcessor.setParameters(processorRef.midBandReverbParameters);
	    };

   //ComboBox Listener
   midReverbBox.onChange = [this, &processorRef]()
	   {
		   int presetIndex = midReverbBox.getSelectedItemIndex();
		   processorRef.loadReverbPreset(presetIndex, 'm');
	   };

   //Populate ComboBox with reverb presets
   midReverbBox.addItem("Small Room", 1);
   midReverbBox.addItem("Concert Hall", 2);
   midReverbBox.addItem("Dark Room", 3);
   midReverbBox.addItem("Bright Room", 4);
   midReverbBox.addItem("Ambient Room", 5);

    setSize(400, 200);
}

MidBandWindow::~MidBandWindow() {}

void MidBandWindow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
    // Add custom drawing here
}

void MidBandWindow::resized()
{
    // Layout child components here
    midBandGainSlider.setBounds(10, 10, 200, 30);
    midBandReverbSlider.setBounds(10, 30, 200, 30);
	midReverbBox.setBounds(220, 30, 150, 30);

}
