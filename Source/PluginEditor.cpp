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
#include "BinaryData.h"

// This extra class is used to create a separate window for hosting plugin editors when the user clicks the "MIDI" button in the band controls.
// It inherits from DocumentWindow, which provides a standard window with a title bar and close button.
// The window takes ownership of the plugin editor component and calls a provided callback function when the close button is pressed, 
// allowing the owner to clean up resources appropriately.
class HostedPluginWindow : public juce::DocumentWindow
{
public:
	HostedPluginWindow(const juce::String& title,
		std::unique_ptr<juce::AudioProcessorEditor> editor,
		std::function<void()> onCloseFn)
		: juce::DocumentWindow(title,
			juce::Colours::darkgrey,
			juce::DocumentWindow::closeButton),
		onClose(std::move(onCloseFn))
	{
		setUsingNativeTitleBar(true);
		setResizable(true, true);

		// Window owns the editor component
		setContentOwned(editor.release(), true);

		centreAroundComponent(juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse(), getWidth(), getHeight());
		setVisible(true);
		toFront(true);
	}

	void closeButtonPressed() override
	{
		// Don’t delete ourselves directly inside the close event.
		// Ask the owner to reset the unique_ptr on the message thread.
		auto cb = onClose;
		juce::MessageManager::callAsync([cb]() { if (cb) cb(); });
	}

private:
	std::function<void()> onClose;
};

// This is the constructor for the main plugin editor class. It initializes the editor with a reference to the audio processor
// and sets up the user interface components, including custom buttons and sliders for controlling the pitch-dependent effects. 
// The constructor also loads custom images for the buttons and applies a custom look-and-feel to the editor.
XPulseAudioProcessorEditor::XPulseAudioProcessorEditor(XPulseAudioProcessor& processorRef)
	: AudioProcessorEditor(&processorRef), audioProcessor(processorRef), apvts(processorRef.parameters)
{
	//Sets the size of the plugin window
	setSize(1250, 650);

#pragma region PitchDependentFX 

#pragma region Initializations
	// Add band group components to the editor (for visual separation)
	addAndMakeVisible(lowBandGroup);
	addAndMakeVisible(midBandGroup);
	addAndMakeVisible(highBandGroup);
	addAndMakeVisible(bandSplitControlsGroup);

	setLookAndFeel(&pluginLookAndFeel);
	// Custom Image Assignments
	//Button Images
	auto offImg = juce::ImageCache::getFromMemory(BinaryData::bypassOff_png, BinaryData::bypassOff_pngSize);
	auto offHoverImg = juce::ImageCache::getFromMemory(BinaryData::bypassOffHover_png, BinaryData::bypassOffHover_pngSize);
	auto onImg = juce::ImageCache::getFromMemory(BinaryData::bypassOn_png, BinaryData::bypassOn_pngSize);
	auto onHoverImg = juce::ImageCache::getFromMemory(BinaryData::bypassOnHover_png, BinaryData::bypassOnHover_pngSize);

	auto midiDisplayOnImg = juce::ImageCache::getFromMemory(BinaryData::midiOn_png, BinaryData::midiOn_pngSize);
	auto midiDisplayOnHoverImg = juce::ImageCache::getFromMemory(BinaryData::midiOnHover_png, BinaryData::midiOnHover_pngSize);
	auto midiDisplayOffImg = juce::ImageCache::getFromMemory(BinaryData::midiOff_png, BinaryData::midiOff_pngSize);
	auto midiDisplayOffHoverImg = juce::ImageCache::getFromMemory(BinaryData::midiOffHover_png, BinaryData::midiOffHover_pngSize);

#pragma endregion


#pragma region LowBand
	//LowBand Components
	const int lowBand = 0;

	lowBypassButton.setClickingTogglesState(true);
	lowBypassButton.setImages(onImg, onHoverImg, offImg, offHoverImg);
	addAndMakeVisible(lowBypassButton);
	lowBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "lowGain", lowBypassButton);

	//Bus1 Logic
	//Button
	lowBypassBus1Button.setClickingTogglesState(true);
	lowBypassBus1Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(lowBypassBus1Button);
	//Knob
	addAndMakeVisible(lowBandBus1LevelSlider);

	auto& low1 = lowBandBus1LevelSlider.getSlider();
	low1.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	low1.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	low1.setRange(0.0, 1.0, 0.001);
	low1.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	lowBandBus1LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	lowBandBus1LevelSlider.setAsPercentKnob();

	// Bus2 Logic
	//Button
	lowBypassBus2Button.setClickingTogglesState(true);
	lowBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(lowBypassBus2Button);
	//Knob
	addAndMakeVisible(lowBandBus2LevelSlider);

	auto& low2 = lowBandBus2LevelSlider.getSlider();
	low2.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	low2.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	low2.setRange(0.0, 1.0, 0.001);
	low2.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	lowBandBus2LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	lowBandBus2LevelSlider.setAsPercentKnob();

	// Bus3 Logic
	addAndMakeVisible(lowBandBus3LevelSlider);

	auto& low3 = lowBandBus3LevelSlider.getSlider();
	low3.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	low3.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	low3.setRange(0.0, 1.0, 0.001);
	low3.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	lowBandBus3LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	lowBandBus3LevelSlider.setAsPercentKnob();


	//Apply Send Amount Based on Bypass State
	auto applyLowSend = [this, lowBand]()
		{
			if (lowBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(lowBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(lowBand, 0, (float)lowBandBus1LevelSlider.getSlider().getValue());

			}

		};

	lowBandBus1LevelSlider.getSlider().onValueChange = applyLowSend;
	lowBypassBus1Button.onClick = applyLowSend;

	// Bus2 callback
	auto applyLowSend2 = [this, lowBand]()
		{
			if (lowBypassBus2Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(lowBand, 1, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(lowBand, 1, (float)lowBandBus2LevelSlider.getSlider().getValue());
			}
		};
	lowBandBus2LevelSlider.getSlider().onValueChange = applyLowSend2;
	lowBypassBus2Button.onClick = applyLowSend2;

	// Bus3 callback
	auto applyLowSend3 = [this, lowBand]()
		{
			if (lowBypassBus3Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(lowBand, 2, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(lowBand, 2, (float)lowBandBus3LevelSlider.getSlider().getValue());
			}
		};
	lowBandBus3LevelSlider.getSlider().onValueChange = applyLowSend3;
	lowBypassBus3Button.onClick = applyLowSend3;
#pragma endregion

#pragma region MidBand
	//MidBand Components
	const int midBand = 1;

	midBypassButton.setClickingTogglesState(true);
	midBypassButton.setImages(onImg, onHoverImg, offImg, offHoverImg);
	addAndMakeVisible(midBypassButton);
	midBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "midGain", midBypassButton);

	//Bus1 Logic
	//Button
	midBypassBus1Button.setClickingTogglesState(true);
	midBypassBus1Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(midBypassBus1Button);
	//Knob
	addAndMakeVisible(midBandBus1LevelSlider);

	auto& mid1 = midBandBus1LevelSlider.getSlider();
	mid1.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	mid1.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	mid1.setRange(0.0, 1.0, 0.001);
	mid1.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	midBandBus1LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	midBandBus1LevelSlider.setAsPercentKnob();

	// Bus2 Logic
	//Button
	midBypassBus2Button.setClickingTogglesState(true);
	midBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(midBypassBus2Button);
	//Knob
	addAndMakeVisible(midBandBus2LevelSlider);

	auto& mid2 = midBandBus2LevelSlider.getSlider();
	mid2.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	mid2.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	mid2.setRange(0.0, 1.0, 0.001);
	mid2.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	midBandBus2LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	midBandBus2LevelSlider.setAsPercentKnob();

	// Bus3 Logic
	//Button
	midBypassBus3Button.setClickingTogglesState(true);
	midBypassBus3Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(midBypassBus3Button);
	//Knob
	addAndMakeVisible(midBandBus3LevelSlider);

	auto& mid3 = midBandBus3LevelSlider.getSlider();
	mid3.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	mid3.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	mid3.setRange(0.0, 1.0, 0.001);
	mid3.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	midBandBus3LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	midBandBus3LevelSlider.setAsPercentKnob();

	//Apply Send Amount Based on Bypass State
	auto applyMidSend = [this, midBand]()
		{
			if (midBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(midBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(midBand, 0, (float)midBandBus1LevelSlider.getSlider().getValue());
			}
		};

	midBandBus1LevelSlider.getSlider().onValueChange = applyMidSend;
	midBypassBus1Button.onClick = applyMidSend;

	// Bus2 callback
	auto applyMidSend2 = [this, midBand]()
		{
			if (midBypassBus2Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(midBand, 1, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(midBand, 1, (float)midBandBus2LevelSlider.getSlider().getValue());
			}
		};
	midBandBus2LevelSlider.getSlider().onValueChange = applyMidSend2;
	midBypassBus2Button.onClick = applyMidSend2;

	// Bus3 callback
	auto applyMidSend3 = [this, midBand]()
		{
			if (midBypassBus3Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(midBand, 2, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(midBand, 2, (float)midBandBus3LevelSlider.getSlider().getValue());
			}
		};
	midBandBus3LevelSlider.getSlider().onValueChange = applyMidSend3;
	midBypassBus3Button.onClick = applyMidSend3;
#pragma endregion

#pragma region HighBand
	//HighBand Components
	const int highBand = 2;

	highBypassButton.setClickingTogglesState(true);
	highBypassButton.setImages(onImg, onHoverImg, offImg, offHoverImg);
	addAndMakeVisible(highBypassButton);
	highBandGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "highGain", highBypassButton);

	//Bus1 Logic
	//Button
	highBypassBus1Button.setClickingTogglesState(true);
	highBypassBus1Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(highBypassBus1Button);
	//Knob
	addAndMakeVisible(highBandBus1LevelSlider);

	auto& high1 = highBandBus1LevelSlider.getSlider();
	high1.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	high1.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	high1.setRange(0.0, 1.0, 0.001);
	high1.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	highBandBus1LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	highBandBus1LevelSlider.setAsPercentKnob();

	// Bus2 Logic
	//Button
	highBypassBus2Button.setClickingTogglesState(true);
	highBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(highBypassBus2Button);
	//Knob
	addAndMakeVisible(highBandBus2LevelSlider);

	auto& high2 = highBandBus2LevelSlider.getSlider();
	high2.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	high2.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	high2.setRange(0.0, 1.0, 0.001);
	high2.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	highBandBus2LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	highBandBus2LevelSlider.setAsPercentKnob();


	// Bus3 Logic
	//Button
	highBypassBus3Button.setClickingTogglesState(true);
	highBypassBus3Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(highBypassBus3Button);
	//Knob
	addAndMakeVisible(highBandBus3LevelSlider);

	auto& high3 = highBandBus3LevelSlider.getSlider();
	high3.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	high3.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	high3.setRange(0.0, 1.0, 0.001);
	high3.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	// Label style + percent formatting
	highBandBus3LevelSlider.setLabelStyle(pluginLookAndFeel.getPixelFont(14.0f),
		pluginLookAndFeel.uiTextColour);
	highBandBus3LevelSlider.setAsPercentKnob();

	//Apply Send Amount Based on Bypass State
	auto applyHighSend = [this, highBand]()
		{
			if (highBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(highBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(highBand, 0, (float)highBandBus1LevelSlider.getSlider().getValue());
			}
		};

	highBandBus1LevelSlider.getSlider().onValueChange = applyHighSend;
	highBypassBus1Button.onClick = applyHighSend;

	// Bus2 callback
	auto applyHighSend2 = [this, highBand]()
		{
			if (highBypassBus2Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(highBand, 1, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(highBand, 1, (float)highBandBus2LevelSlider.getSlider().getValue());
			}
		};
	highBandBus2LevelSlider.getSlider().onValueChange = applyHighSend2;
	highBypassBus2Button.onClick = applyHighSend2;

	// Bus3 callback
	auto applyHighSend3 = [this, highBand]()
		{
			if (highBypassBus3Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(highBand, 2, 0.0f); // Adjust if you have separate send amounts per bus
			}
			else
			{
				audioProcessor.setBandSendAmount(highBand, 2, (float)highBandBus3LevelSlider.getSlider().getValue());
			}
		};
	highBandBus3LevelSlider.getSlider().onValueChange = applyHighSend3;
	highBypassBus3Button.onClick = applyHighSend3;
#pragma endregion

#pragma region BandSplitControls
	//Band Split Controls Components

	addAndMakeVisible(bandSplitKeyboard);

	bandSplitSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
	bandSplitSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	//73 Key length, E1 to E7
	bandSplitSlider.setRange(28.0, 100.0f, 1.0);
	bandSplitSlider.setMinAndMaxValues(48.0, 72.0);
	addAndMakeVisible(bandSplitSlider);

	// Ensure there’s always a minimum gap between the splits to avoid issues in processing
	const double minGapSemis = 1.0;

	auto midiToHz = [](float midiNote) -> float
		{
			return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
		};

	bandSplitSlider.onValueChange = [this, midiToHz, minGapSemis]()
	{
		auto lo = bandSplitSlider.getMinValue();
		auto hi = bandSplitSlider.getMaxValue();
		
		if (hi < lo + minGapSemis)
		{
			const auto thumb = bandSplitSlider.getThumbBeingDragged();

			if (thumb == 1) 
				lo = hi - minGapSemis;
			else
				hi = lo + minGapSemis;

			bandSplitSlider.setMinAndMaxValues(lo, hi, juce::dontSendNotification);
		}
		
		const float loHz = midiToHz((float)lo);
		const float hiHz = midiToHz((float)hi);
		
		if (auto* p = apvts.getParameter("lowMidCrossoverMidi"))
			p->setValueNotifyingHost(p->convertTo0to1((float)lo));

		if (auto* p = apvts.getParameter("midHighCrossoverMidi"))
			p->setValueNotifyingHost(p->convertTo0to1((float)hi));

	};

#pragma endregion

#pragma region PluginSlotsSetup

	// Band Plugin Slots Setup
	for (int idx = 0; idx < 9; ++idx)
	{
		const int band = idx / 3; // 0..2
		const int slot = idx % 3; // 0..2

		bandSlots[idx].setBandIndex(band);
		bandSlots[idx].setSlotIndex(slot);
		addAndMakeVisible(bandSlots[idx]);

		// Keep the menu list fresh when user opens it
		bandSlots[idx].onRequestRebuildMenuList = [this](int /*band*/, int /*slot*/)
			{
				rebuildPluginListFromHost();
			};

		// Add/Replace
		bandSlots[idx].onAddReplace = [this, idx](int band, int slot, const juce::PluginDescription& desc)
			{

				// Close plugin window for this slot first (destroys editor safely)
				pluginWindows[idx].reset();

				// If this slot already had an instance, destroy it
				if (bandInstanceId[idx] != 0)
				{
					// Clear instance id from processor first before destroying
					audioProcessor.setBandPluginInstanceId(band, slot, 0);

					audioProcessor.getHostProcessor().getPool().destroyInstance(bandInstanceId[idx]);
					bandInstanceId[idx] = 0;
				}

				// Create new instance in pool
				auto newId = audioProcessor.getHostProcessor().getPool().createInstance(desc);
				bandInstanceId[idx] = newId;


				// Route this (band, slot) to the new instance
				audioProcessor.setBandPluginInstanceId(band, slot, (uint32_t)newId);

				bandSlots[idx].setHasPlugin(newId != 0);
				bandSlots[idx].setPluginName(newId != 0 ? desc.name : juce::String("-None-"));
			};

		// Remove
		bandSlots[idx].onRemove = [this, idx](int band, int slot)
			{

				if (bandInstanceId[idx] == 0)
					return;

				// Close plugin window for this slot first (destroys editor safely)
				pluginWindows[idx].reset();

				// Clear routing + reset send/return for this (band, slot)
				audioProcessor.setBandPluginInstanceId(band, slot, 0);
				audioProcessor.setBandSendAmount(band, slot, 0.0f);
				audioProcessor.setBandReturnAmount(band, slot, 1.0f);

				// Destroy instance
				audioProcessor.getHostProcessor().getPool().destroyInstance(bandInstanceId[idx]);
				bandInstanceId[idx] = 0;

				bandSlots[idx].setHasPlugin(false);
				bandSlots[idx].setPluginName({});
				resized();
				repaint();
			};

		// Open Editor
		bandSlots[idx].onOpenEditor = [this, idx](int /*band*/, int /*slot*/)
			{
				auto id = bandInstanceId[idx];
				if (id == 0)
					return;

				// If already open, just bring it forward
				if (pluginWindows[idx])
				{
					pluginWindows[idx]->setVisible(true);
					pluginWindows[idx]->toFront(true);
					return;
				}

				auto& pool = audioProcessor.getHostProcessor().getPool();
				auto ed = pool.createEditorFor(id);
				if (!ed)
					return;

				auto title = ed->getName();
				pluginWindows[idx] = std::make_unique<HostedPluginWindow>(
					title,
					std::move(ed),
					[this, idx]()
					{
						pluginWindows[idx].reset(); // safe: runs async from closeButtonPressed
					});
			};
	}

	// Fill from cached list immediately
	rebuildPluginListFromHost();

	// Refresh once background scan finishes
	startTimerHz(2);


#pragma endregion

#pragma region MidiDisplay
	//Initialize MIDI Display Button
	addAndMakeVisible(midiDisplayButton);

	midiDisplayButton.setImages(
		false, true, true,
		midiDisplayOnImg, 1.0f, juce::Colours::transparentBlack,
		midiDisplayOnHoverImg, 1.0f, juce::Colours::transparentBlack,
		midiDisplayOnHoverImg, 1.0f, juce::Colours::transparentBlack
	);

	//Creates Midi Display Panel
	midiDisplay = std::make_unique<MidiDisplay>(audioProcessor, apvts);
	addAndMakeVisible(*midiDisplay);
	midiDisplay->setVisible(false);

	//Set MIDI Display close button images
	midiDisplay->setCloseButtonImages(
		midiDisplayOffImg, midiDisplayOffHoverImg, midiDisplayOffHoverImg
	);

	// Close Callback for MIDI Display
	midiDisplay->onRequestClose = [this]()
		{
			if (midiDisplay == nullptr)
				return;

			midiDisplay->setVisible(false);
			midiDisplay->setEnabled(false);
			midiDisplay->setAlwaysOnTop(false);
		};

	midiDisplayButton.onClick = [this]()
		{
			if (midiDisplay == nullptr)
				return;

			const bool show = !midiDisplay->isVisible();
			midiDisplay->setVisible(show);
			midiDisplay->setEnabled(show);

			if (show)
			{
				midiDisplay->setBounds(getLocalBounds());
				midiDisplay->setAlwaysOnTop(true);
				midiDisplay->toFront(true);
				midiDisplay->grabKeyboardFocus();
				midiDisplay->repaint();
			}
		
		};

	//Pedal FX Slot Callbacks
	midiDisplay->onRequestPluginList = [this](BandPluginSlot& slot)
		{
			rebuildPluginListFromHost();
			slot.setPluginList(cachedDescs);		
		};
	midiDisplay->onAddReplace = [this](int band, int /*slot*/, const juce::PluginDescription& desc)
		{

			// close existing pedal window
			pedalWindows[band].reset();

			// destroy old pedal instance
			if (pedalInstanceId[band] != 0)
			{
				audioProcessor.setPedalPluginInstanceId(band, 0);
				audioProcessor.getHostProcessor().getPool().destroyInstance(pedalInstanceId[band]);
				pedalInstanceId[band] = 0;
			}

			// create new
			auto newId = audioProcessor.getHostProcessor().getPool().createInstance(desc);

			if (newId == 0)
				return;

			pedalInstanceId[band] = newId;

			// route into the *pedal* routing arrays (separate from main slots)
			audioProcessor.setPedalPluginInstanceId(band, (uint32_t)newId);
			if (midiDisplay)
				midiDisplay->setPedalSlotLoaded(band, true, desc.name);
			
			
		};
	midiDisplay->onRemove = [this](int band, int /*slot*/)
		{
			if (pedalInstanceId[band] == 0)
				return;

			pedalWindows[band].reset();

			audioProcessor.setPedalPluginInstanceId(band, 0);
			audioProcessor.setPedalSendAmount(band, 0.0f);
			audioProcessor.setPedalReturnAmount(band, 1.0f);

			audioProcessor.getHostProcessor().getPool().destroyInstance(pedalInstanceId[band]);
			pedalInstanceId[band] = 0;

			if (midiDisplay)
				midiDisplay->setPedalSlotUnloaded(band, false);

		};
	midiDisplay->onOpenEditor = [this](int band, int /*slot*/)
		{
			auto id = pedalInstanceId[band];
			if (id == 0)
				return;

			if (pedalWindows[band])
			{
				pedalWindows[band]->setVisible(true);
				pedalWindows[band]->toFront(true);
				return;
			}

			auto& pool = audioProcessor.getHostProcessor().getPool();
			auto ed = pool.createEditorFor(id);
			if (!ed) return;

			pedalWindows[band] = std::make_unique<HostedPluginWindow>(
				ed->getName(),
				std::move(ed),
				[this, band]() { pedalWindows[band].reset(); }
			);
		};
#pragma endregion


#pragma endregion 
}

XPulseAudioProcessorEditor::~XPulseAudioProcessorEditor()
{
	setLookAndFeel(nullptr);
}

//==============================================================================
void XPulseAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
	g.fillAll(juce::Colour(245, 230, 204));
    g.setColour (juce::Colour(245, 230, 204));
    /*g.setFont (juce::FontOptions (15.0f));*/
	g.setFont(getPixelFont(12.0f));

}

//Component Layout
void XPulseAudioProcessorEditor::resized()
{
	// Layout the four band group components as three even vertical sections and one bottom section
	const int bottomHeight = 250;
	const int bandWidth = getWidth() / 3;
	const int bandHeight = getHeight() - bottomHeight;

	lowBandGroup.setBounds(0, 0, bandWidth, bandHeight);
	midBandGroup.setBounds(bandWidth, 0, bandWidth, bandHeight);
	highBandGroup.setBounds(2 * bandWidth, 0, getWidth() - 2 * bandWidth, bandHeight);
	bandSplitControlsGroup.setBounds(0, bandHeight, getWidth(), bottomHeight);

	// Position MIDI Display Button
	midiDisplayButton.setBounds(getWidth() - 60, 10, 50, 50);
	if (midiDisplay != nullptr) {
		midiDisplay->setBounds(getLocalBounds());
		if (midiDisplay->isVisible())
			midiDisplay->toFront(false);

	}
	
	// Low Band Components
	lowBypassButton.setBounds(10, 30, 100, 30);

	const int busButtonX[3] = { 25, bandWidth + 25, 2 * bandWidth + 25 };
	const int busKnobX[3]   = { 350, bandWidth + 350, 2 * bandWidth + 350 };
	const int busYStart = 57.5;
	const int busSpacing = 130;

	// Low Band Buses
	lowBypassBus1Button.setBounds(busButtonX[0], busYStart + 0 * busSpacing, 55, 55);
	lowBandBus1LevelSlider.setBounds(busKnobX[0], busYStart + 0 * busSpacing, 55, 55);
	lowBypassBus2Button.setBounds(busButtonX[0], busYStart + 1 * busSpacing, 55, 55);
	lowBandBus2LevelSlider.setBounds(busKnobX[0], busYStart + 1 * busSpacing, 55, 55);
	lowBypassBus3Button.setBounds(busButtonX[0], busYStart + 2 * busSpacing, 55, 55);
	lowBandBus3LevelSlider.setBounds(busKnobX[0], busYStart + 2 * busSpacing, 55, 55);

	// Mid Band Components
	midBypassButton.setBounds(bandWidth + 10, 30, 100, 30);
	midBypassBus1Button.setBounds(busButtonX[1], busYStart + 0 * busSpacing, 55, 55);
	midBandBus1LevelSlider.setBounds(busKnobX[1], busYStart + 0 * busSpacing, 55, 55);
	midBypassBus2Button.setBounds(busButtonX[1], busYStart + 1 * busSpacing, 55, 55);
	midBandBus2LevelSlider.setBounds(busKnobX[1], busYStart + 1 * busSpacing, 55, 55);
	midBypassBus3Button.setBounds(busButtonX[1], busYStart + 2 * busSpacing, 55, 55);
	midBandBus3LevelSlider.setBounds(busKnobX[1], busYStart + 2 * busSpacing, 55, 55);

	// High Band Components
	highBypassButton.setBounds(2 * bandWidth + 10, 30, 100, 30);
	highBypassBus1Button.setBounds(busButtonX[2], busYStart + 0 * busSpacing, 55, 55);
	highBandBus1LevelSlider.setBounds(busKnobX[2], busYStart + 0 * busSpacing, 55, 55);
	highBypassBus2Button.setBounds(busButtonX[2], busYStart + 1 * busSpacing, 55, 55);
	highBandBus2LevelSlider.setBounds(busKnobX[2], busYStart + 1 * busSpacing, 55, 55);
	highBypassBus3Button.setBounds(busButtonX[2], busYStart + 2 * busSpacing, 55, 55);
	highBandBus3LevelSlider.setBounds(busKnobX[2], busYStart + 2 * busSpacing, 55, 55);

	// Lay out plugin slots: 3 per band, stacked vertically with spacing
	const int slotHeight = 30;
	const int slotSpacing = 100;
	for (int band = 0; band < numBands; ++band)
	{
		int x = band * bandWidth + 75;
		int yStart = 70;
		int slotW = bandWidth - 150;
		for (int s = 0; s < slotsPerBand; ++s)
		{
			int slotIdx = band * slotsPerBand + s;
			int y = yStart + s * (slotHeight + slotSpacing);
			bandSlots[slotIdx].setBounds(x, y, slotW, slotHeight);
		}
	}

	// Band Split Controls Components
	//KeyBoard, Currently just for show, concept only
	

	bandSplitKeyboard.setBounds(10, bandHeight + 125, getWidth(), bottomHeight - 35);

	bandSplitSlider.setBounds(75, bandHeight + 20, getWidth() - 150, bottomHeight - 100);

	
}

// Below are custom functions for our editor class
#pragma region Custom Functions

#pragma region Host Functions
void XPulseAudioProcessorEditor::openPluginEditorWindowForBand(int band)
{
	auto id = bandInstanceId[band];
	if (id == 0)
		return;

	auto& pool = audioProcessor.getHostProcessor().getPool();
	auto ed = pool.createEditorFor(id);
	if (!ed)
		return;

	// Optional: if you only want one window per band, close any existing one first
	if (pluginWindows[band])
	{
		pluginWindows[band]->setVisible(false);
		pluginWindows[band].reset();
	}

	// Size the window to the plugin editor
	auto w = juce::jmax(200, ed->getWidth());
	auto h = juce::jmax(100, ed->getHeight());

	juce::DialogWindow::LaunchOptions opts;
	opts.dialogTitle = ed->getName();
	opts.dialogBackgroundColour = juce::Colours::darkgrey;
	opts.escapeKeyTriggersCloseButton = true;
	opts.useNativeTitleBar = true;
	opts.resizable = true;

	// IMPORTANT: the window takes ownership
	opts.content.setOwned(ed.release());

	// Centre around your plugin editor
	opts.componentToCentreAround = this;

	// Launch async returns immediately; window self-manages close.
	auto* dw = opts.launchAsync();

	// Keep a handle so you can close it when replacing/removing
	// launchAsync() returns a DialogWindow*, which is also a DocumentWindow
	pluginWindows[band].reset(dynamic_cast<juce::DocumentWindow*>(dw));

	if (pluginWindows[band])
		pluginWindows[band]->setSize(w, h);
}

void XPulseAudioProcessorEditor::rebuildPluginListFromHost()
{
	cachedDescs.clear();
	audioProcessor.getHostProcessor().getKnownPluginTypesCopy(cachedDescs);



	for (int idx = 0; idx < numBands * slotsPerBand; ++idx)
		bandSlots[idx].setPluginList(cachedDescs);
}

void XPulseAudioProcessorEditor::timerCallback()
{
	rebuildPluginListFromHost(); // TEMP: always refresh for debugging

	if (audioProcessor.getHostProcessor().isScanFinished())
	{
		//rebuildPluginListFromHost();
		stopTimer();
	}

	// Get the latest velocity from the processor
	int velocity = audioProcessor.lowBandVelocity.load();

	if (midiDisplay)
		midiDisplay->setPedalDown(audioProcessor.isSustainDown());

}

#pragma endregion

//#pragma region FX Engine Window Functions
//void XPulseAudioProcessorEditor::openPitchDependentFXWindow(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvts)
//{
//	if (pitchDependentFXWindowPtr == nullptr) // If the window is not already open
//	{
//		pitchDependentFXWindowPtr = std::make_unique<PitchDependentFXEditor>(processorRef, apvts);
//		pitchDependentFXWindowPtr->setVisible(true);
//	}
//	else
//	{
//		pitchDependentFXWindowPtr->setVisible(true);
//		pitchDependentFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
//	}
//}
//void XPulseAudioProcessorEditor::openPerformanceFXWindow(juce::AudioProcessorValueTreeState& apvts)
//{
//	if (performanceFXWindowPtr == nullptr) // If the window is not already open
//	{
//		performanceFXWindowPtr = std::make_unique<PerformanceFXEditor>(apvts);
//		performanceFXWindowPtr->setVisible(true);
//	}
//	else
//	{
//		performanceFXButton.setVisible(true);
//		performanceFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
//	}
//}
//void XPulseAudioProcessorEditor::openSpectralMorphFXWindow(juce::AudioProcessorValueTreeState& apvts)
//{
//	if (spectralMorphFXWindowPtr == nullptr) // If the window is not already open
//	{
//		spectralMorphFXWindowPtr = std::make_unique<SpectralMorphFXEditor>(apvts);
//		spectralMorphFXWindowPtr->setVisible(true);
//	}
//	else
//	{
//		spectralMorphFXButton.setVisible(true);
//		spectralMorphFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
//	}
//}
//void XPulseAudioProcessorEditor::openTextureBlendFXWindow(juce::AudioProcessorValueTreeState& apvts)
//{
//	if (textureBlendFXWindowPtr == nullptr) // If the window is not already open
//	{
//		textureBlendFXWindowPtr = std::make_unique<TextureBlendFXEditor>(apvts);
//		textureBlendFXWindowPtr->setVisible(true);
//	}
//	else
//	{
//		textureBlendFXButton.setVisible(true);
//		textureBlendFXWindowPtr->toFront(true); // Bring the window to the front if it's already open
//	}
//}
//#pragma endregion

#pragma endregion