#include "TextureBlendFXContent.h"

TextureBlendFXContent::TextureBlendFXContent(juce::AudioProcessorValueTreeState& apvts)
{
    titleLabel.setText("Texture Blend FX", juce::dontSendNotification);
    addAndMakeVisible(titleLabel);
}

TextureBlendFXContent::~TextureBlendFXContent() {}

void TextureBlendFXContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void TextureBlendFXContent::resized()
{
    titleLabel.setBounds(10, 10, 200, 30);
}
