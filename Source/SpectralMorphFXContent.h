#pragma once
#include <JuceHeader.h>

class SpectralMorphFXContent : public juce::Component
{
public:
    SpectralMorphFXContent(juce::AudioProcessorValueTreeState& apvts);
    ~SpectralMorphFXContent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::Label titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralMorphFXContent)
};
