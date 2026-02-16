
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <vector>

class PluginPool
{
public:
    using InstanceId = uint32_t;
    using PluginTypeId = juce::String;

    explicit PluginPool(juce::AudioPluginFormatManager& fm);

    // Lifecycle
    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();

    // UI thread only
    InstanceId createInstance(const juce::PluginDescription& desc);
    void destroyInstance(InstanceId id);
    void destroyAll();

    // UI thread only
    std::unique_ptr<juce::AudioProcessorEditor> createEditorFor(InstanceId id);

    // Audio thread safe (reads snapshot only)
    juce::AudioPluginInstance* getInstanceForAudio(InstanceId id) const;

    // Optional helpers
    bool hasInstance(InstanceId id) const;

    static PluginTypeId getTypeIdFor(const juce::PluginDescription& desc)
    {
        return desc.createIdentifierString();
    }

    std::vector<InstanceId> findInstancesByType(const juce::PluginDescription& desc) const;

private:
    struct Entry
    {
        juce::PluginDescription desc;
        std::unique_ptr<juce::AudioPluginInstance> instance;
    };

    // Snapshot for audio thread: (id -> raw pointer)
    struct Snapshot
    {
        std::vector<std::pair<InstanceId, juce::AudioPluginInstance*>> items;
    };

    void rebuildSnapshot(); // UI thread

    juce::AudioPluginFormatManager& formatManager;

    double sr = 44100.0;
    int bs = 512;

    InstanceId nextId = 1;

    // UI-thread-owned authoritative storage
    std::unordered_map<InstanceId, Entry> entries;

    // Audio-thread-readable snapshot
    std::shared_ptr<Snapshot> snapshot;
};

