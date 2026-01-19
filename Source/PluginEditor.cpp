/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PitchDependentFXEditor.h"
#include "PerformanceFXEditor.h"
#include "SpectralMorphFXEditor.h"
#include "TextureBlendFXEditor.h"
#include "HostProcessor.h"


//==============================================================================
XPulseAudioProcessorEditor::XPulseAudioProcessorEditor (XPulseAudioProcessor& processorRef)
	: AudioProcessorEditor(&processorRef), audioProcessor(processorRef)
{
	//Sets the size of the plugin window
	setSize(1250, 600);

	//Button onClick functions to open each FX Engine window :: Also attaches the APVTS to the Windows
	pitchDependentFXButton.onClick = [this]() { openPitchDependentFXWindow(audioProcessor, audioProcessor.parameters); };
	addAndMakeVisible(pitchDependentFXButton);
	performanceFXButton.onClick = [this]() { openPerformanceFXWindow(audioProcessor.parameters); };
	addAndMakeVisible(performanceFXButton);
	spectralMorphFXButton.onClick = [this]() { openSpectralMorphFXWindow(audioProcessor.parameters); };
	addAndMakeVisible(spectralMorphFXButton);
	textureBlendFXButton.onClick = [this]() { openTextureBlendFXWindow(audioProcessor.parameters); };
	addAndMakeVisible(textureBlendFXButton);


	//Hosted Plugin Editor Components
	addAndMakeVisible(pluginChoice);
	addAndMakeVisible(refreshPluginListButton);
	addAndMakeVisible(loadPluginButton);
	addAndMakeVisible(unLoadPluginButton);
	addAndMakeVisible(replacePluginButton);

	refreshPluginListButton.onClick = [this]()
		{
			juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
				"XPulse", "Refresh clicked!");


			audioProcessor.getHostProcessor().scanForPlugins();

			pluginChoice.clear();
			cachedDescs.clear();

			auto types = audioProcessor.getHostProcessor().getKnownPluginList().getTypes();

			int id = 1;
			for (auto& t : types)
			{
				cachedDescs.push_back(t);
				pluginChoice.addItem(t.name, id++);
			}
		};

	loadPluginButton.onClick = [this]()
		{
			int idx = pluginChoice.getSelectedId() - 1;
			if (idx >= 0 && idx < (int)cachedDescs.size())
			{
				audioProcessor.getHostProcessor().loadPlugin(cachedDescs[(size_t)idx]);
				refreshHostedEditor();
			}
		};

	unLoadPluginButton.onClick = [this]()
		{
			hostedEditor_.reset();
			audioProcessor.getHostProcessor().unloadPlugin();
			refreshHostedEditor();
		};

	replacePluginButton.onClick = [this]()
		{
			int idx = pluginChoice.getSelectedId() - 1;
			if (idx >= 0 && idx < (int)cachedDescs.size())
			{
				hostedEditor_.reset();
				audioProcessor.getHostProcessor().replacePlugin(cachedDescs[(size_t)idx]);
				refreshHostedEditor();
			}
		};
	//Hosted Plugin Editor

	hostedEditor_ = audioProcessor.getHostProcessor().createHostedEditor();

	if (hostedEditor_)
	{
		addAndMakeVisible(*hostedEditor_);
		resized();
	}

	
}

XPulseAudioProcessorEditor::~XPulseAudioProcessorEditor()
{
}

//==============================================================================
void XPulseAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
	g.fillAll(juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

    
}

//Component Layout
void XPulseAudioProcessorEditor::resized()
{
	//Buttons to open each band window
	pitchDependentFXButton.setBounds(10, 10, 130, 30);
	performanceFXButton.setBounds(10, 50, 130, 30);
	spectralMorphFXButton.setBounds(10, 90, 130, 30);
	textureBlendFXButton.setBounds(10, 130, 130, 30);

	// Hosted Plugin Controls
	pluginChoice.setBounds(160, 10, 300, 30);
	refreshPluginListButton.setBounds(470, 10, 120, 30);
	loadPluginButton.setBounds(600, 10, 80, 30);
	unLoadPluginButton.setBounds(690, 10, 80, 30);
	replacePluginButton.setBounds(780, 10, 80, 30);

	// Hosted plugin editor 
	if (hostedEditor_)
		hostedEditor_->setBounds(160, 50, getWidth() - 170, getHeight() - 60);

}

// Below are custom functions for our editor class
#pragma region Custom Functions
#pragma region Host Functions
void XPulseAudioProcessorEditor::refreshHostedEditor()
{
	// Remove old editor component first
	if (hostedEditor_)
	{
		removeChildComponent(hostedEditor_.get());
		hostedEditor_.reset();
	}

	hostedEditor_ = audioProcessor.getHostProcessor().createHostedEditor();

	if (hostedEditor_)
		addAndMakeVisible(*hostedEditor_);

	resized();
	repaint();
}
#pragma endregion
void XPulseAudioProcessorEditor::openPitchDependentFXWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts)
{
	if (pitchDependentFXWindowPtr == nullptr) // If the window is not already open
	{
		pitchDependentFXWindowPtr = std::make_unique<PitchDependentFXEditor>(processorRef, apvts);
		pitchDependentFXWindowPtr->setVisible(true);
	}
	else
	{
		pitchDependentFXWindowPtr->setVisible(true);
		pitchDependentFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
	}
}
void XPulseAudioProcessorEditor::openPerformanceFXWindow(juce::AudioProcessorValueTreeState& apvts)
{
	if (performanceFXWindowPtr == nullptr) // If the window is not already open
	{
		performanceFXWindowPtr = std::make_unique<PerformanceFXEditor>(apvts);
		performanceFXWindowPtr->setVisible(true);
	}
	else
	{
		performanceFXButton.setVisible(true);
		performanceFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
	}
}
void XPulseAudioProcessorEditor::openSpectralMorphFXWindow(juce::AudioProcessorValueTreeState& apvts)
{
	if (spectralMorphFXWindowPtr == nullptr) // If the window is not already open
	{
		spectralMorphFXWindowPtr = std::make_unique<SpectralMorphFXEditor>(apvts);
		spectralMorphFXWindowPtr->setVisible(true);
	}
	else
	{
		spectralMorphFXButton.setVisible(true);
		spectralMorphFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
	}
}
void XPulseAudioProcessorEditor::openTextureBlendFXWindow(juce::AudioProcessorValueTreeState& apvts)
{
	if (textureBlendFXWindowPtr == nullptr) // If the window is not already open
	{
		textureBlendFXWindowPtr = std::make_unique<TextureBlendFXEditor>(apvts);
		textureBlendFXWindowPtr->setVisible(true);
	}
	else
	{
		textureBlendFXButton.setVisible(true);
		textureBlendFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
	}
}


#pragma endregion