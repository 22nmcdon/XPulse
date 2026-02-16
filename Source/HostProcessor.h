#pragma once

#include <vector>
#include <atomic>
#include <memory>
#include <string>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginPool.h"

class HostProcessor
{
public:
    HostProcessor();
    ~HostProcessor();

    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();

    PluginPool& getPool() { return pool; }

    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    PluginPool::InstanceId getActiveInstanceId() const { return hostedInstanceId; }
    void setActiveInstanceId(PluginPool::InstanceId id) { hostedInstanceId = id; }


    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);


    //UI
    std::unique_ptr<juce::AudioProcessorEditor> createHostedEditor();


    // Scanning / loading Plugins
    void scanForPlugins();
    void startBackgroundScan();
    bool isScanFinished() const { return scanFinished.load(); }


    // Loading (now uses the pool)
    void loadPlugin(const juce::PluginDescription& desc);
    void unloadPlugin();
	void replacePlugin(const juce::PluginDescription& desc);

    void getKnownPluginTypesCopy(juce::Array<juce::PluginDescription>& out) const;



    // State
    void getState(juce::MemoryBlock& dest);
    void setState(const void* data, int size);


private:
    juce::ApplicationProperties appProps;

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

	PluginPool pool;
    PluginPool::InstanceId hostedInstanceId = 0;

    double sr = 44100.0;
    int bs = 512;

    #pragma region PluginScannerThread
    //This is a Helper Thread that is owned by HostProcessor

    class ScannerThread : public juce::Thread
    {
    public:
        explicit ScannerThread(HostProcessor& hp)
            : juce::Thread("Plugin Scanner Thread"), host(hp) {}
        void run() override;

    private:
        HostProcessor& host;
    };
    

    #pragma endregion

    juce::CriticalSection pluginListLock;

    std::unique_ptr<ScannerThread> scannerThread;
    std::atomic<bool> scanFinished{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostProcessor)
};
