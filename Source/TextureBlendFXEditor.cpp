#include "TextureBlendFXEditor.h"

TextureBlendFXEditor::TextureBlendFXEditor(juce::AudioProcessorValueTreeState& apvts)
    : juce::DocumentWindow("Texture Blend FX Editor", juce::Colours::lightgrey, juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    setSize(900, 300);
    setVisible(true);
}

TextureBlendFXEditor::~TextureBlendFXEditor() {}

void TextureBlendFXEditor::closeButtonPressed()
{
    setVisible(false); // Hide the window when the close button is pressed
}
