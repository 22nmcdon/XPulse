#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchDependentFXEditor.h"
#include "PerformanceFXEditor.h"
#include "SpectralMorphFXEditor.h"
#include "TextureBlendFXEditor.h"
#include "BandPluginSlot.h"
#include "BinaryData.h"
#include "MidiDisplay.h"
#include "RotaryLabelKnob.h"   

// This is the header file for the plugin editor, which defines the XPulseAudioProcessorEditor class 
// and is responsible for the GUI componentes of the plugin
class HostProcessor;

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
	// Fonts
	static juce::Font getPixelFont(float height)
	{
		static juce::Typeface::Ptr tf =
			juce::Typeface::createSystemTypefaceFor(BinaryData::ARCADECLASSIC_TTF,
				BinaryData::ARCADECLASSIC_TTFSize);

		return juce::Font(tf).withHeight(height);
	}

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


		}
		void paint(juce::Graphics& g) override
		{
			// Draw the keyboard image
			g.drawImageWithin(keyboardImage, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);


		}
		void mouseDown(const juce::MouseEvent& event) override
		{
			// Convert mouse position to MIDI key and update split points
			// (Implement logic based on image layout)

			//This is not Implemented, for future works
		}
	private:
		juce::Image keyboardImage;

	};

	#pragma endregion

    struct PluginLookAndFeel : public juce::LookAndFeel_V4
    {
        PluginLookAndFeel()
        {
            //  Images 
            knobImg = juce::ImageCache::getFromMemory(BinaryData::Knob_png, BinaryData::Knob_pngSize);

            rightThumbOn = juce::ImageCache::getFromMemory(BinaryData::SliderRightOn_png, BinaryData::SliderRightOn_pngSize);
            rightThumbOff = juce::ImageCache::getFromMemory(BinaryData::SliderRightOff_png, BinaryData::SliderRightOff_pngSize);
            leftThumbOn = juce::ImageCache::getFromMemory(BinaryData::SliderLeftOn_png, BinaryData::SliderLeftOn_pngSize);
            leftThumbOff = juce::ImageCache::getFromMemory(BinaryData::SliderLeftOff_png, BinaryData::SliderLeftOff_pngSize);
            sliderBar = juce::ImageCache::getFromMemory(BinaryData::SliderBar_png, BinaryData::SliderBar_pngSize);

            // Font 
            typeface = juce::Typeface::createSystemTypefaceFor(
                BinaryData::ARCADECLASSIC_TTF,
                BinaryData::ARCADECLASSIC_TTFSize);
        }

		// Font Helper Function  
        juce::Font getPixelFont(float height) const
        {
            if (typeface != nullptr)
                return juce::Font(typeface).withHeight(height);

            return juce::Font(height);
        }

		// Custom Colors
        juce::Colour uiTextColour = juce::Colour::fromRGB(138, 0, 0);

        juce::Label* createSliderTextBox(juce::Slider& slider) override
        {
            auto* l = juce::LookAndFeel_V4::createSliderTextBox(slider);

            // Font + text colour
            l->setFont(getPixelFont(14.0f));
            l->setColour(juce::Label::textColourId, uiTextColour);

            // Background + outline (tweak as you like)
            l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

            // Center the text
            l->setJustificationType(juce::Justification::centred);

            // If you want display-only (no typing), keep this:
            l->setEditable(false, false, false);

            // If you want the user to type values, comment out the line above.

            return l;
        }


        void drawGroupComponentOutline(juce::Graphics& g,
            int width, int height,
            const juce::String& text,
            const juce::Justification& position,
            juce::GroupComponent& group) override
        {
            // Border
            g.setColour(juce::Colours::white.withAlpha(0.7f)); // adjust to taste
            const int textH = 18;                                // space reserved for title
            g.drawRect(0, textH / 2, width, height - textH / 2);

            // Title text
            g.setColour(uiTextColour);

            // Use your pixel font helper
            g.setFont(getPixelFont(14.0f));

            // Give the title a little padding so it doesn't sit on the border
            auto titleArea = juce::Rectangle<int>(8, 0, width - 16, textH);
            g.drawText(text, titleArea, position, true);
        }
        // Knob 
        void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
            juce::Slider& slider) override
        {
            // If this isn't one of your image knobs, fallback to default
            if (!knobImg.isValid())
            {
                juce::LookAndFeel_V4::drawRotarySlider(g, x, y, w, h, sliderPos, rotaryStartAngle, rotaryEndAngle, slider);
                return;
            }

            const int size = juce::jmin(w, h);
            auto dest = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h)
                .withSizeKeepingCentre((float)size, (float)size);

            const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            juce::AffineTransform t =
                juce::AffineTransform::translation(-knobImg.getWidth() * 0.5f,
                    -knobImg.getHeight() * 0.5f)
                .rotated(angle)
                .scaled(dest.getWidth() / (float)knobImg.getWidth(),
                    dest.getHeight() / (float)knobImg.getHeight())
                .translated(dest.getCentreX(), dest.getCentreY());

            g.drawImageTransformed(knobImg, t);
        }

        // Band split slider 
        static float midiToHz(float midiNote)
        {
            return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
        }

        static juce::String midiToNoteName(int midiNote)
        {
            static const char* flatNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };

            midiNote = juce::jlimit(0, 127, midiNote);

            const int noteIndex = midiNote % 12;
            const int octave = (midiNote / 12) - 1; // 60 -> C4 (common convention)

            return juce::String(flatNames[noteIndex]) + juce::String(octave);
        }

        void drawLinearSlider(juce::Graphics& g,
            int x, int y, int width, int height,
            float sliderPos, float minSliderPos, float maxSliderPos,
            const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            // Only custom-draw your TwoValueHorizontal range slider. Everything else falls back.
            if (style != juce::Slider::TwoValueHorizontal)
            {
                juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
                    sliderPos, minSliderPos, maxSliderPos,
                    style, slider);
                return;
            }

            const float thumbW = 32.0f;
            const float thumbH = 32.0f;
            const float thumbHalf = thumbW * 0.5f;
            const float trackH = 8.0f;

            const float trackLeft = (float)x + thumbHalf;
            const float trackRight = (float)x + (float)width - thumbHalf;
            const float trackW = trackRight - trackLeft;

            const float trackY = (float)y + ((float)height - trackH) * 0.5f;

            const float minX = juce::jlimit(trackLeft, trackRight, minSliderPos);
            const float maxX = juce::jlimit(trackLeft, trackRight, maxSliderPos);

            // bar
            if (sliderBar.isValid())
                g.drawImage(sliderBar, trackLeft, trackY, trackW, trackH, 0, 0, sliderBar.getWidth(), sliderBar.getHeight());
            else
                g.fillRoundedRectangle(trackLeft, trackY, trackW, trackH, trackH * 0.5f);

            // thumbs
            const float thumbY = (float)y + ((float)height - thumbH) * 0.5f;

            const int thumb = slider.getThumbBeingDragged();
            const bool draggingMin = (thumb == 1);
            const bool draggingMax = (thumb == 2);

            const auto& leftImg = draggingMin ? leftThumbOn : leftThumbOff;
            const auto& rightImg = draggingMax ? rightThumbOn : rightThumbOff;

            auto drawThumb = [&](const juce::Image& img, float centerX)
                {
                    const float drawX = centerX - thumbHalf;

                    if (img.isValid())
                        g.drawImage(img, drawX, thumbY, thumbW, thumbH, 0, 0, img.getWidth(), img.getHeight());
                    else
                        g.fillEllipse(drawX, thumbY, thumbW, thumbH);
                };

            drawThumb(leftImg, minX);
            drawThumb(rightImg, maxX);

            // labels
            const float loMidi = (float)slider.getMinValue();
            const float hiMidi = (float)slider.getMaxValue();

            const juce::String loText = midiToNoteName((int)loMidi);
            const juce::String hiText = midiToNoteName((int)hiMidi);

            g.setColour(uiTextColour);
            g.setFont(getPixelFont(12.0f));

            const float labelW = 70.0f;
            const float labelH = 14.0f;
            const float labelGap = 2.0f;
            const float labelY = thumbY - labelH - labelGap;

            auto drawLabelCenteredAt = [&](float centerX, const juce::String& text)
                {
                    float lx = centerX - (labelW * 0.5f);
                    lx = juce::jlimit((float)x, (float)x + (float)width - labelW, lx);

                    g.drawFittedText(text, (int)lx, (int)labelY, (int)labelW, (int)labelH,
                        juce::Justification::centred, 1);
                };

            drawLabelCenteredAt(minX, loText);
            drawLabelCenteredAt(maxX, hiText);
        }

        // assets 
        juce::Image knobImg;

        juce::Image rightThumbOn, rightThumbOff;
        juce::Image leftThumbOn, leftThumbOff;
        juce::Image sliderBar;

        juce::Typeface::Ptr typeface;
    };

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

#pragma region PedalSlots
    PluginPool::InstanceId pedalInstanceId[3]{ 0, 0, 0 };
    std::unique_ptr<juce::DocumentWindow> pedalWindows[3];
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
    RotaryLabelKnob lowBandBus1LevelSlider;
	TwoStateHoverButton lowBypassBus2Button{};
    RotaryLabelKnob lowBandBus2LevelSlider;
	TwoStateHoverButton lowBypassBus3Button{};
    RotaryLabelKnob lowBandBus3LevelSlider;

    // MIDI Controls Panel
    std::unique_ptr<MidiDisplay> midiDisplay;
	juce::ImageButton midiDisplayButton;
    
	// MidBand components
	TwoStateHoverButton midBypassButton{};
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midBandGainAttachment;

	TwoStateHoverButton midBypassBus1Button{};
    RotaryLabelKnob midBandBus1LevelSlider;
	TwoStateHoverButton midBypassBus2Button{};
    RotaryLabelKnob midBandBus2LevelSlider;
	TwoStateHoverButton midBypassBus3Button{};
    RotaryLabelKnob midBandBus3LevelSlider;

	// HighBand components
	TwoStateHoverButton highBypassButton{};
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> highBandGainAttachment;

	TwoStateHoverButton highBypassBus1Button{};
    RotaryLabelKnob highBandBus1LevelSlider;
	TwoStateHoverButton highBypassBus2Button{};
    RotaryLabelKnob highBandBus2LevelSlider;
	TwoStateHoverButton highBypassBus3Button{};
    RotaryLabelKnob highBandBus3LevelSlider;


	// Band Split Keyboard
	BandSplitKeyboard bandSplitKeyboard;//Not Implmented Yet
	
	juce::Slider bandSplitSlider{};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowMidCrossoverAttachmentMidi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midHighCrossoverAttachmentMidi;


	//LookAndFeel
	PluginLookAndFeel pluginLookAndFeel;

	//Audio Processor Reference
	juce::AudioProcessorValueTreeState& apvts;
    XPulseAudioProcessor& audioProcessor;


	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XPulseAudioProcessorEditor)
};
