/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchDependentFXEditor.h"
#include "PerformanceFXEditor.h"
#include "SpectralMorphFXEditor.h"
#include "TextureBlendFXEditor.h"

class HostProcessor;
//==============================================================================
/**
*/
class XPulseAudioProcessorEditor  : public juce::AudioProcessorEditor
                                   
{
public:
	//Custom functions to open each FX Engine window
	void openPitchDependentFXWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts);
	void openPerformanceFXWindow(juce::AudioProcessorValueTreeState& apvts);
	void openSpectralMorphFXWindow(juce::AudioProcessorValueTreeState& apvts);
	void openTextureBlendFXWindow(juce::AudioProcessorValueTreeState& apvts);

    XPulseAudioProcessorEditor (XPulseAudioProcessor&);
    ~XPulseAudioProcessorEditor() override;

	

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
	//Creates Seperate Windows for each FX Engine
	std::unique_ptr<PitchDependentFXEditor> pitchDependentFXWindowPtr;
	std::unique_ptr<PerformanceFXEditor> performanceFXWindowPtr;
	std::unique_ptr<SpectralMorphFXEditor> spectralMorphFXWindowPtr;
	std::unique_ptr<TextureBlendFXEditor> textureBlendFXWindowPtr;

	// Buttons to open each FX Engine window
	juce::TextButton pitchDependentFXButton{ "Pitch Dependent FX" };
	juce::TextButton performanceFXButton{ "Performance FX" };
	juce::TextButton spectralMorphFXButton{ "Spectral Morph FX" };
	juce::TextButton textureBlendFXButton{ "Texture Blend FX" };

	//Hosted Plugin Editor 
	std::unique_ptr<juce::AudioProcessorEditor> hostedEditor_;

	juce::ComboBox pluginChoice;
	juce::TextButton refreshPluginListButton{ "Refresh List" };
	juce::TextButton loadPluginButton{ "Load" };
	juce::TextButton unLoadPluginButton{ "Unload" };
	juce::TextButton replacePluginButton{ "Replace" };

	std::vector <juce::PluginDescription> cachedDescs;

	void refreshHostedEditor();


    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    XPulseAudioProcessor& audioProcessor;

	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XPulseAudioProcessorEditor)
};
