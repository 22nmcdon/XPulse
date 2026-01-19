#pragma once
#include <JuceHeader.h>

class TextureBlendFXContent : public juce::Component
{
public:
    TextureBlendFXContent(juce::AudioProcessorValueTreeState& apvts);
    ~TextureBlendFXContent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::Label titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureBlendFXContent)
};
