#pragma once
#include <JuceHeader.h>

class TextureBlendFXEditor : public juce::DocumentWindow
{
public:
    TextureBlendFXEditor(juce::AudioProcessorValueTreeState& apvts);
    ~TextureBlendFXEditor() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureBlendFXEditor)
};
