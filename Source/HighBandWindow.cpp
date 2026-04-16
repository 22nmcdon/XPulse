#include "HighBandWindow.h"
#include "PluginProcessor.h"
//Deprecated

HighBandWindow::HighBandWindow(XPulseAudioProcessor &processorRef, juce::AudioProcessorValueTreeState& apvts)
	: processorRef(processorRef), apvtsRef(apvts)
{
	
}

HighBandWindow::~HighBandWindow() {}

void HighBandWindow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
    // Add custom drawing here
}

void HighBandWindow::resized()
{
   
}
