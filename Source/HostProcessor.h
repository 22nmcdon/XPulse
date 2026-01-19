#pragma once

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <juce_audio_processors/juce_audio_processors.h>

class HostProcessor
{
public:
    HostProcessor();
    ~HostProcessor();

    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);

    //UI
    std::unique_ptr<juce::AudioProcessorEditor> createHostedEditor();


    // Scanning / loading Plugins
    void scanForPlugins();

    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }


    void loadPlugin(const juce::PluginDescription& desc);
    void unloadPlugin();
	void replacePlugin(const juce::PluginDescription& desc);


    // State
    void getState(juce::MemoryBlock& dest);
    void setState(const void* data, int size);

private:

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    juce::SpinLock hostedSpinLock;
    std::unique_ptr<juce::AudioPluginInstance> hosted;

    double sr = 44100.0;
    int bs = 512;

    juce::CriticalSection hostedLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostProcessor)
};
