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
#include "BandPluginSlot.h"

class HostProcessor;
//==============================================================================
/**
*/
class XPulseAudioProcessorEditor  : public juce::AudioProcessorEditor,
									private juce::Timer        
{
public:
	
    XPulseAudioProcessorEditor (XPulseAudioProcessor&);
    ~XPulseAudioProcessorEditor() override;


    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

	//Plugin Host Helper Functions
	void rebuildPluginListFromHost();

private:
	void timerCallback() override;

	void openPluginEditorWindowForBand(int band);
	#pragma region Custom Components

	// Two State Hover Button
	class TwoStateHoverButton : public juce::Button
		{
		public:
			TwoStateHoverButton() : juce::Button("TwoStateHoverButton") {}

			void setImages(juce::Image offN, juce::Image offH,
				juce::Image onN, juce::Image onH)
			{
				offNormal = offN; offHover = offH;
				onNormal = onN;  onHover = onH;
				repaint();
			}

			void paintButton(juce::Graphics& g, bool isHovered, bool isDown) override
			{
				const bool isOn = getToggleState();
				const juce::Image& img =
					isOn ? ((isHovered || isDown) ? onHover : onNormal)
					: ((isHovered || isDown) ? offHover : offNormal);

				if (img.isValid())
					g.drawImageWithin(img, 0, 0, getWidth(), getHeight(),
						juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
			}

		private:
			juce::Image offNormal, offHover, onNormal, onHover;
		};

	// Rotating Knob
	struct RotatingImageKnob : public juce::LookAndFeel_V4
	{
		juce::Image knobImg;

		void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
			float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
			juce::Slider&) override
		{
			if (!knobImg.isValid())
				return;

			// Make a square destination centered in the slider bounds
			const int size = juce::jmin(w, h);
			auto dest = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h)
				.withSizeKeepingCentre((float)size, (float)size);

			const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

			// Rotate around image center, then scale to dest, then move to dest center
			juce::AffineTransform t =
				juce::AffineTransform::translation(-knobImg.getWidth() * 0.5f,
					-knobImg.getHeight() * 0.5f)
				.rotated(angle)
				.scaled(dest.getWidth() / (float)knobImg.getWidth(),
					dest.getHeight() / (float)knobImg.getHeight())
				.translated(dest.getCentreX(), dest.getCentreY());

			g.drawImageTransformed(knobImg, t);
		}
	};
	RotatingImageKnob knob;

	// Keyboard Component: Band-Separation UI
	// Spans from E1 to E7 as a 73-key keyboard using MIDI 28-100
	// Involves Two Split-Points to create Three Bands
	//Not Fully Implemented Yet, but the idea is to have a visual representation 
	// of the keyboard with two draggable split points that determine the low/mid 
	// and mid/high band boundaries. The user can click on the keyboard to set these 
	// split points, and the editor will update accordingly.
	class BandSplitKeyboard : public juce::Component
	{
	public:
		BandSplitKeyboard()
		{
			// Load the keyboard image
			keyboardImage = juce::ImageFileFormat::loadFrom(BinaryData::KeyBoard_png, BinaryData::KeyBoard_pngSize);
			
			// Default splitPoints
			lowBandSplit = 52;  // E3
			highBandSplit = 76; // E5
		}
		void paint(juce::Graphics& g) override
		{
			// Draw the keyboard image
			g.drawImageWithin(keyboardImage, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);

			// Draw overlays for split points
			drawSplitOverlay(g, lowBandSplit);
			drawSplitOverlay(g, highBandSplit);

			// Optionally, highlight bands
			// drawBandHighlight(g, ...);
		}
		void mouseDown(const juce::MouseEvent& event) override
		{
			// Convert mouse position to MIDI key and update split points
			// (Implement logic based on image layout)
		}
	private:
		juce::Image keyboardImage;
		int lowBandSplit;   // MIDI key for low-mid split
		int highBandSplit;  // MIDI key for mid-high split

		void drawSplitOverlay(juce::Graphics& g, int midiKey)
		{
			// Calculate x position for the split key
			float keyWidth = getWidth() / 73.0f;
			int keyIndex = midiKey - 21;
			float x = keyIndex * keyWidth;
			g.setColour(juce::Colours::red);
			g.drawLine(x, 0, x, getHeight(), 2.0f);
		}
	};

	#pragma endregion

	#pragma region BandPluginSlots

	static constexpr int numBands = 3;
	static constexpr int slotsPerBand = 3;
	static constexpr int numSlots = numBands * slotsPerBand;

	BandPluginSlot bandSlots[numSlots];
	PluginPool::InstanceId bandInstanceId[numSlots]{ 0 };
	std::unique_ptr<juce::DocumentWindow> pluginWindows[numSlots];
	juce::Array<juce::PluginDescription> cachedDescs;

	// Helper to get band index from slot index
	static int getBandForSlot(int slot) { return slot / slotsPerBand; }
	static int getSlotInBand(int slot) { return slot % slotsPerBand; }

	#pragma endregion

	// Band group components
	juce::GroupComponent lowBandGroup{ "lowBandGroup", "Low Band" };
	juce::GroupComponent midBandGroup{ "midBandGroup", "Mid Band" };
	juce::GroupComponent highBandGroup{ "highBandGroup", "High Band" };
	juce::GroupComponent bandSplitControlsGroup{ "bandSplitControlsGroup", "Band Split Controls" };

	// LowBand components
	TwoStateHoverButton lowBypassButton{};
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lowBandGainAttachment;

	// Bypass buttons and rotary knobs for 3 slots per band
	TwoStateHoverButton lowBypassBus1Button{};
	juce::Slider lowBandBus1LevelSlider;
	TwoStateHoverButton lowBypassBus2Button{};
	juce::Slider lowBandBus2LevelSlider;
	TwoStateHoverButton lowBypassBus3Button{};
	juce::Slider lowBandBus3LevelSlider;

	// MidBand components
	TwoStateHoverButton midBypassButton{};
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midBandGainAttachment;

	TwoStateHoverButton midBypassBus1Button{};
	juce::Slider midBandBus1LevelSlider;
	TwoStateHoverButton midBypassBus2Button{};
	juce::Slider midBandBus2LevelSlider;
	TwoStateHoverButton midBypassBus3Button{};
	juce::Slider midBandBus3LevelSlider;

	// HighBand components
	TwoStateHoverButton highBypassButton{};
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> highBandGainAttachment;

	TwoStateHoverButton highBypassBus1Button{};
	juce::Slider highBandBus1LevelSlider;
	TwoStateHoverButton highBypassBus2Button{};
	juce::Slider highBandBus2LevelSlider;
	TwoStateHoverButton highBypassBus3Button{};
	juce::Slider highBandBus3LevelSlider;


	// Band Split Keyboard
	BandSplitKeyboard bandSplitKeyboard;//Not Implmented Yet
	
	juce::Slider bandSplitSlider{};


	//Audio Processor Reference
	juce::AudioProcessorValueTreeState& apvts;
    XPulseAudioProcessor& audioProcessor;

	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XPulseAudioProcessorEditor)
};
