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

//Hosted Plugin Window Class
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


//==============================================================================
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

	// Custom Image Assignments
	//Button Images
	auto offImg = juce::ImageCache::getFromMemory(BinaryData::bypassOff_png, BinaryData::bypassOff_pngSize);
	auto offHoverImg = juce::ImageCache::getFromMemory(BinaryData::bypassOffHover_png, BinaryData::bypassOffHover_pngSize);
	auto onImg = juce::ImageCache::getFromMemory(BinaryData::bypassOn_png, BinaryData::bypassOn_pngSize);
	auto onHoverImg = juce::ImageCache::getFromMemory(BinaryData::bypassOnHover_png, BinaryData::bypassOnHover_pngSize);

	//Knob Image
	knob.knobImg = juce::ImageCache::getFromMemory(BinaryData::Knob_png, BinaryData::Knob_pngSize);
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
	lowBandBus1LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	lowBandBus1LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	lowBandBus1LevelSlider.setRange(0.0, 1.0, 0.001);
	lowBandBus1LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);

	lowBandBus1LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(lowBandBus1LevelSlider);

	// Bus2 Logic
	lowBypassBus2Button.setClickingTogglesState(true);
	lowBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(lowBypassBus2Button);
	lowBandBus2LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	lowBandBus2LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	lowBandBus2LevelSlider.setRange(0.0, 1.0, 0.001);
	lowBandBus2LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	lowBandBus2LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(lowBandBus2LevelSlider);

	// Bus3 Logic
	lowBypassBus3Button.setClickingTogglesState(true);
	lowBypassBus3Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(lowBypassBus3Button);
	lowBandBus3LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	lowBandBus3LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	lowBandBus3LevelSlider.setRange(0.0, 1.0, 0.001);
	lowBandBus3LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	lowBandBus3LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(lowBandBus3LevelSlider);


	//Apply Send Amount Based on Bypass State
	auto applyLowSend = [this, lowBand]()
		{
			if (lowBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(lowBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(lowBand, 0, (float)lowBandBus1LevelSlider.getValue());

			}

		};

	lowBandBus1LevelSlider.onValueChange = applyLowSend;
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
				audioProcessor.setBandSendAmount(lowBand, 1, (float)lowBandBus2LevelSlider.getValue());
			}
		};
	lowBandBus2LevelSlider.onValueChange = applyLowSend2;
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
				audioProcessor.setBandSendAmount(lowBand, 2, (float)lowBandBus3LevelSlider.getValue());
			}
		};
	lowBandBus3LevelSlider.onValueChange = applyLowSend3;
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
	midBandBus1LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	midBandBus1LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	midBandBus1LevelSlider.setRange(0.0, 1.0, 0.001);
	midBandBus1LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	midBandBus1LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(midBandBus1LevelSlider);

	// Bus2 Logic
	midBypassBus2Button.setClickingTogglesState(true);
	midBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(midBypassBus2Button);
	midBandBus2LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	midBandBus2LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	midBandBus2LevelSlider.setRange(0.0, 1.0, 0.001);
	midBandBus2LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	midBandBus2LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(midBandBus2LevelSlider);

	// Bus3 Logic
	midBypassBus3Button.setClickingTogglesState(true);
	midBypassBus3Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(midBypassBus3Button);
	midBandBus3LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	midBandBus3LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	midBandBus3LevelSlider.setRange(0.0, 1.0, 0.001);
	midBandBus3LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	midBandBus3LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(midBandBus3LevelSlider);

	//Apply Send Amount Based on Bypass State
	auto applyMidSend = [this, midBand]()
		{
			if (midBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(midBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(midBand, 0, (float)midBandBus1LevelSlider.getValue());
			}
		};

	midBandBus1LevelSlider.onValueChange = applyMidSend;
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
				audioProcessor.setBandSendAmount(midBand, 1, (float)midBandBus2LevelSlider.getValue());
			}
		};
	midBandBus2LevelSlider.onValueChange = applyMidSend2;
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
				audioProcessor.setBandSendAmount(midBand, 2, (float)midBandBus3LevelSlider.getValue());
			}
		};
	midBandBus3LevelSlider.onValueChange = applyMidSend3;
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
	highBandBus1LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	highBandBus1LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	highBandBus1LevelSlider.setRange(0.0, 1.0, 0.001);
	highBandBus1LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	highBandBus1LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(highBandBus1LevelSlider);

	// Bus2 Logic
	highBypassBus2Button.setClickingTogglesState(true);
	highBypassBus2Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(highBypassBus2Button);
	highBandBus2LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	highBandBus2LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	highBandBus2LevelSlider.setRange(0.0, 1.0, 0.001);
	highBandBus2LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	highBandBus2LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(highBandBus2LevelSlider);

	// Bus3 Logic
	highBypassBus3Button.setClickingTogglesState(true);
	highBypassBus3Button.setImages(offImg, offHoverImg, onImg, onHoverImg);
	addAndMakeVisible(highBypassBus3Button);
	highBandBus3LevelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	highBandBus3LevelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	highBandBus3LevelSlider.setRange(0.0, 1.0, 0.001);
	highBandBus3LevelSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
		juce::MathConstants<float>::pi * 3.25f,
		true);
	highBandBus3LevelSlider.setLookAndFeel(&knob);
	addAndMakeVisible(highBandBus3LevelSlider);

	//Apply Send Amount Based on Bypass State
	auto applyHighSend = [this, highBand]()
		{
			if (highBypassBus1Button.getToggleState())
			{
				audioProcessor.setBandSendAmount(highBand, 0, 0.0f);
			}
			else
			{
				audioProcessor.setBandSendAmount(highBand, 0, (float)highBandBus1LevelSlider.getValue());
			}
		};

	highBandBus1LevelSlider.onValueChange = applyHighSend;
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
				audioProcessor.setBandSendAmount(highBand, 1, (float)highBandBus2LevelSlider.getValue());
			}
		};
	highBandBus2LevelSlider.onValueChange = applyHighSend2;
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
				audioProcessor.setBandSendAmount(highBand, 2, (float)highBandBus3LevelSlider.getValue());
			}
		};
	highBandBus3LevelSlider.onValueChange = applyHighSend3;
	highBypassBus3Button.onClick = applyHighSend3;
#pragma endregion

#pragma region BandSplitControls
	//Band Split Controls Components

	/*addAndMakeVisible(bandSplitKeyboard);*/

	bandSplitSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
	bandSplitSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);

	// frequency range (pick what makes sense)

	//Later add an option to switch to full range 0-127 MIDI control, which isn't as applicable in most cases
	bandSplitSlider.setRange(24.0, 108.0, 1.0);
	bandSplitSlider.setMinAndMaxValues(48.0, 72.0);
	addAndMakeVisible(bandSplitSlider);

	// Ensure there’s always a minimum gap between the splits to avoid issues in processing
	const double minGapSemis = 1.0;

	auto midiToHz = [](float midiNote) -> float
		{
			return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
		};

	auto pushSplitsToProcessor = [this, &midiToHz]()
		{
			const double lo = bandSplitSlider.getMinValue();
			const double hi = bandSplitSlider.getMaxValue();
			audioProcessor.setBandSplits(midiToHz(lo), midiToHz(hi));
		};

	bandSplitSlider.onValueChange = [this, &midiToHz, pushSplitsToProcessor, minGapSemis]()
		{
			auto lo = bandSplitSlider.getMinValue();
			auto hi = bandSplitSlider.getMaxValue();

			if (hi < lo + minGapSemis)
    {
        const auto thumb = bandSplitSlider.getThumbBeingDragged();

        if (thumb == bandSplitSlider.getMinimum())
            lo = hi - minGapSemis;
        else
            hi = lo + minGapSemis;

        // write back without triggering recursion
        bandSplitSlider.setMinAndMaxValues(lo, hi, juce::dontSendNotification);
    }

    pushSplitsToProcessor();
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

#pragma endregion 
}

XPulseAudioProcessorEditor::~XPulseAudioProcessorEditor()
{
}

//==============================================================================
void XPulseAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
	g.fillAll(juce::Colour(245, 230, 204));
    g.setColour (juce::Colour(245, 230, 204));
    g.setFont (juce::FontOptions (15.0f));

    
}

//Component Layout
void XPulseAudioProcessorEditor::resized()
{
	// Layout the four band group components as three even vertical sections and one bottom section
	const int bottomHeight = 150;
	const int bandWidth = getWidth() / 3;
	const int bandHeight = getHeight() - bottomHeight;

	lowBandGroup.setBounds(0, 0, bandWidth, bandHeight);
	midBandGroup.setBounds(bandWidth, 0, bandWidth, bandHeight);
	highBandGroup.setBounds(2 * bandWidth, 0, getWidth() - 2 * bandWidth, bandHeight);
	bandSplitControlsGroup.setBounds(0, bandHeight, getWidth(), bottomHeight);

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
	//bandSplitKeyboard.setBounds(10, bandHeight + 20, getWidth(), bottomHeight);

	bandSplitSlider.setBounds(10, bandHeight + 20, getWidth(), bottomHeight);

	
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