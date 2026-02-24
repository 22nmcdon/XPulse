#pragma once
#include <JuceHeader.h>

class RotaryLabelKnob : public juce::Component, private juce::Timer
{
public:
    RotaryLabelKnob()
    {
        // Slider setup stays minimal here; you can keep your existing style
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);

        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(valueLabel);

        //Start Timer
		startTimerHz(30); 
    }

    juce::Slider& getSlider() { return slider; }

    void resized() override
    {
        auto r = getLocalBounds();

        auto labelArea = r.removeFromBottom(18);
        valueLabel.setBounds(labelArea);

        slider.setBounds(r);
    }

    void setLabelStyle(const juce::Font& font, juce::Colour textColour)
    {
        valueLabel.setFont(font);
        valueLabel.setColour(juce::Label::textColourId, textColour);
	}

    void setAsPercentKnob()
    {
        slider.setRange(0.0, 1.0, 0.001);

        updateValueLabel();

        slider.onValueChange = [this]()
        {
                updateValueLabel();
        };
    }

    void updateValueLabel()
    {
        const int pct = juce::jlimit(0, 100,
            (int)std::lround(slider.getValue() * 100.0));

        valueLabel.setText(juce::String(pct) + "%", juce::dontSendNotification);
    }

    void timerCallback() override
    {
        // This is just a safety measure to ensure the label updates if the slider value changes
        // from outside (e.g., via an attachment).
        updateValueLabel();
	}

private:
    juce::Slider slider;
    juce::Label  valueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryLabelKnob)
};