#pragma once
#include <JuceHeader.h>

class PerformanceFXContent : public juce::Component
{
public:
    PerformanceFXContent(juce::AudioProcessorValueTreeState& apvts);
    ~PerformanceFXContent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::Label titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceFXContent)
};
