#pragma once
#pragma once
#include <JuceHeader.h>
#include "RotaryLabelKnob.h"
#include "BinaryData.h"
#include "PluginProcessor.h"
#include "BandPluginSlot.h"

// This component is responsible for displaying MIDI-related controls and information, such as velocity knobs and pedal plugin slots.
// It also provides callbacks for interactions with the pedal plugin slots, allowing the main editor to handle plugin management actions.
class MidiDisplay : public juce::Component
{
public:
	// Helper Functions for pluginSlots callbacks
    std::function<void(BandPluginSlot&)> onRequestPluginList;
    std::function<void(int band, int slot, const juce::PluginDescription&)> onAddReplace;
    std::function<void(int band, int slot)> onRemove;
    std::function<void(int band, int slot)> onOpenEditor;
    void setPedalSlotLoaded(int band, bool hasPlugin, const juce::String& name)
    {
        if (band < 0 || band >= 3) return;
        pedalSlots[band].setHasPlugin(hasPlugin);
        pedalSlots[band].setPluginName(name);
    }
    void setPedalSlotUnloaded(int band, bool hasPlugin)
    {
        if (band < 0 || band >= 3) return;
        pedalSlots[band].setHasPlugin(false);
        pedalSlots[band].setPluginName({});
	}


    MidiDisplay(XPulseAudioProcessor& processorRef, juce::AudioProcessorValueTreeState& apvtsRef)
        : audioProcessor(processorRef), apvts(apvtsRef)
    {
        setInterceptsMouseClicks(true, true); 
        
        addAndMakeVisible(closeButton);

        closeButton.onClick = [this]
        {
            if (onRequestClose)
                onRequestClose();
        };

        auto setupVel = [this](RotaryLabelKnob& knob, juce::Label& title, const juce::String& text)
        {
            addAndMakeVisible(knob);

            auto& s = knob.getSlider();
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setRange(0.0, 1.0, 0.001);
            s.setRotaryParameters(juce::MathConstants<float>::pi * 1.75f,
                juce::MathConstants<float>::pi * 3.25f,
                true);

            // wrapper label settings
            knob.setLabelStyle(getPixelFont(14.0f), juce::Colour::fromRGB(138, 0, 0));
            knob.setAsPercentKnob();

            // title label (above knob)
            title.setText(text, juce::dontSendNotification);
            title.setFont(getPixelFont(14.0f));
            title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(138, 0, 0));
            title.setJustificationType(juce::Justification::centred);
            title.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(title);
        };

        setupVel(lowVelKnob, lowVelLabel, "Low Velocity To Send");
        setupVel(midVelKnob, midVelLabel, "Mid Velocity To Send");
        setupVel(highVelKnob, highVelLabel, "High Velocity To Send");

		// Pedal Plugin Slot
        for (int b = 0; b < 3; ++b)
        {
            pedalSlots[b].setBandIndex(b);
			pedalSlots[b].setSlotIndex(99); // Dummy slot index since these are single-slot "bands"
            addAndMakeVisible(pedalSlots[b]);

            pedalSlotLabels[b].setInterceptsMouseClicks(false, false);
            pedalSlotLabels[b].setJustificationType(juce::Justification::centred);
            pedalSlotLabels[b].setFont(getPixelFont(14.0f));
            pedalSlotLabels[b].setColour(juce::Label::textColourId, uiTextColour);
            pedalSlotLabels[b].setText(b == 0 ? "Low Pedal FX" : b == 1 ? "Mid Pedal FX" : "High Pedal FX",
                juce::dontSendNotification);
            addAndMakeVisible(pedalSlotLabels[b]);
        };
        for (int b = 0; b < 3; ++b)
        {
            pedalSlots[b].onRequestRebuildMenuList = [this, b](int, int)
                {
                    if (onRequestPluginList) onRequestPluginList(pedalSlots[b]);
                };

            pedalSlots[b].onAddReplace = [this](int band, int slot, const juce::PluginDescription& desc)
                {
                    if (onAddReplace) onAddReplace(band, slot, desc);
                };

            pedalSlots[b].onRemove = [this](int band, int slot)
                {
                    if (onRemove) onRemove(band, slot);
                };

            pedalSlots[b].onOpenEditor = [this](int band, int slot)
                {
                    if (onOpenEditor) onOpenEditor(band, slot);
                };
        }



        // Attach to APVTS
        lowVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lowVelToSend", lowVelKnob.getSlider());
        midVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "midVelToSend", midVelKnob.getSlider());
        highVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "highVelToSend", highVelKnob.getSlider());
    }

    void setPedalDown(bool down)
    {
        pedalDown = down;
        repaint();
    }

    void setCloseButtonImages(const juce::Image& normal,
        const juce::Image& hover,
        const juce::Image& down = juce::Image())
    {
        closeButton.setImages(
            false, true, true,
            normal, 1.0f, juce::Colours::transparentBlack,
            hover, 1.0f, juce::Colours::transparentBlack,
            down.isValid() ? down : hover, 1.0f, juce::Colours::transparentBlack
        );
    }

    ~MidiDisplay() override = default;
   

    void paint(juce::Graphics& g) override
    {
        // Dim the whole plugin behind the overlay
        g.fillAll(juce::Colours::black.withAlpha(0.65f));

        // Panel card
        auto bounds = getLocalBounds();

        g.setColour(juce::Colour(172, 147, 98).withAlpha(0.75f));
        g.fillRoundedRectangle(bounds.toFloat(), 12.0f);

        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(bounds.toFloat(), 12.0f, 1.5f);

        g.setColour(uiTextColour);
        g.setFont(getPixelFont(24.0f));
        g.drawText("MIDI Controls", bounds.removeFromTop(40), juce::Justification::centred);

		//Pedal down indicator
        g.setFont(getPixelFont(14.0f));
        g.setColour(pedalDown ? juce::Colours::lime : juce::Colours::red);
        g.drawText(pedalDown ? "SUSTAIN: ON" : "SUSTAIN: OFF",
            getLocalBounds().removeFromTop(20),
            juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(40);
        auto header = bounds.removeFromTop(40);

        closeButton.setBounds(getWidth() - 60, 10, 50, 50);

        auto content = bounds.reduced(20);

        // 3 knobs in a row
        auto velRow = content.removeFromTop(160);
        auto velWidth = velRow.getWidth() / 3;

        auto col1 = velRow.removeFromLeft(velWidth);
        auto col2 = velRow.removeFromLeft(velWidth);
        auto col3 = velRow;

        lowVelKnob.setBounds(col1.removeFromTop(120).reduced(10));
        lowVelLabel.setBounds(col1.removeFromTop(30).reduced(10));

        midVelKnob.setBounds(col2.removeFromTop(120).reduced(10));
        midVelLabel.setBounds(col2.removeFromTop(30).reduced(10));

        highVelKnob.setBounds(col3.removeFromTop(120).reduced(10));
        highVelLabel.setBounds(col3.removeFromTop(30).reduced(10));

		//Pedal slots in a row
        auto pedalRow = content.removeFromTop(120);
        auto pedalWidth = pedalRow.getWidth() / 3;

        for (int b = 0; b < 3; ++b)
        {
            auto col = pedalRow.removeFromLeft(pedalWidth).reduced(10);

            pedalSlotLabels[b].setBounds(col.removeFromTop(20));
            pedalSlots[b].setBounds(col.removeFromTop(30));
        }
    }

    // Editor sets this
    std::function<void()> onRequestClose;

private:
    // Reference to main processor
	XPulseAudioProcessor& audioProcessor;

    // Pedal Boolean
    bool pedalDown = false;

	// Pedal Plugin Slots
	BandPluginSlot pedalSlots[3]; 
    juce::Label     pedalSlotLabels[3];
    
	// Helper to load our pixel font from BinaryData
    static juce::Font getPixelFont(float height)
    {
        static juce::Typeface::Ptr tf =
            juce::Typeface::createSystemTypefaceFor(BinaryData::ARCADECLASSIC_TTF,
                BinaryData::ARCADECLASSIC_TTFSize);

        return juce::Font(tf).withHeight(height);
    }

    juce::Colour uiTextColour = juce::Colour::fromRGB(138, 0, 0);

    juce::AudioProcessorValueTreeState& apvts;

	juce::ImageButton closeButton;

    RotaryLabelKnob lowVelKnob, midVelKnob, highVelKnob;
    juce::Label  lowVelLabel, midVelLabel, highVelLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowVelAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midVelAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highVelAttach;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiDisplay)
};