#pragma once
#pragma once
#include <JuceHeader.h>
#include "RotaryLabelKnob.h"
#include "BinaryData.h"

class MidiDisplay : public juce::Component
{
public:
    MidiDisplay(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
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

        // Attach to APVTS
        lowVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lowVelToSend", lowVelKnob.getSlider());
        midVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "midVelToSend", midVelKnob.getSlider());
        highVelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "highVelToSend", highVelKnob.getSlider());
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
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(40);
        auto header = bounds.removeFromTop(40);

        closeButton.setBounds(getWidth() - 60, 10, 50, 50);

        auto content = bounds.reduced(20);

        // 3 knobs in a row
        auto row = content.removeFromTop(160);
        auto w = row.getWidth() / 3;

        auto col1 = row.removeFromLeft(w);
        auto col2 = row.removeFromLeft(w);
        auto col3 = row;

        lowVelKnob.setBounds(col1.removeFromTop(120).reduced(10));
        lowVelLabel.setBounds(col1.removeFromTop(30).reduced(10));

        midVelKnob.setBounds(col2.removeFromTop(120).reduced(10));
        midVelLabel.setBounds(col2.removeFromTop(30).reduced(10));

        highVelKnob.setBounds(col3.removeFromTop(120).reduced(10));
        highVelLabel.setBounds(col3.removeFromTop(30).reduced(10));
    }

    // Editor sets this
    std::function<void()> onRequestClose;

private:

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