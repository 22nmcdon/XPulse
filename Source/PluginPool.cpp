#include "PluginPool.h"
#include "atomic"

// Important Note: This class is designed to be mostly used from the UI thread.
// The audio thread only reads from the snapeshot atomic pointer, while the UI
// thread can mutate the entries map and rebuild the snapshot as needed.

PluginPool::PluginPool(juce::AudioPluginFormatManager& fm)
    : formatManager(fm)
{
    std::atomic_store(&snapshot, std::make_shared<Snapshot>());
}

void PluginPool::prepareToPlay(double sampleRate, int blockSize)
{
    sr = sampleRate;
    bs = blockSize;

    // UI thread call is safest. If host calls prepare on audio thread, it's still okay-ish here,
    // because we don't mutate snapshot structure, only instances. Keep it simple for now.
    for (auto& [id, e] : entries)
        if (e.instance) e.instance->prepareToPlay(sr, bs);
}

void PluginPool::releaseResources()
{
    for (auto& [id, e] : entries)
        if (e.instance) e.instance->releaseResources();
}

PluginPool::InstanceId PluginPool::createInstance(const juce::PluginDescription& desc)
{
    juce::String error;

    auto inst = formatManager.createPluginInstance(desc, sr, bs, error);
    if (!inst)
    {
        DBG("PluginPool createInstance failed: " + error);
        return 0;
    }

    inst->prepareToPlay(sr, bs);

    const auto id = nextId++;
    Entry entry;
    entry.desc = desc;
    entry.instance = std::move(inst);
    entries.emplace(id, std::move(entry));

    rebuildSnapshot();
    return id;
}

void PluginPool::destroyInstance(InstanceId id)
{
    auto it = entries.find(id);
    if (it == entries.end())
        return;

    // Editor must be destroyed by whoever owns it before this call
    entries.erase(it);

    rebuildSnapshot();
}

void PluginPool::destroyAll()
{
    entries.clear();
    rebuildSnapshot();
}

std::unique_ptr<juce::AudioProcessorEditor> PluginPool::createEditorFor(InstanceId id)
{
    auto it = entries.find(id);
    if (it == entries.end() || !it->second.instance)
        return {};

    if (!it->second.instance->hasEditor())
        return {};

    return std::unique_ptr<juce::AudioProcessorEditor>(it->second.instance->createEditor());
}

juce::AudioPluginInstance* PluginPool::getInstanceForAudio(InstanceId id) const
{
    auto snap = std::atomic_load(&snapshot);
    if (!snap)
        return nullptr;

    for (auto& [sid, ptr] : snap->items)
        if (sid == id)
            return ptr;

    return nullptr;
}

bool PluginPool::hasInstance(InstanceId id) const
{
    return entries.find(id) != entries.end();
}

void PluginPool::rebuildSnapshot()
{
    auto newSnap = std::make_shared<Snapshot>();
    newSnap->items.reserve(entries.size());

    for (auto& [id, e] : entries)
        newSnap->items.emplace_back(id, e.instance.get());

    std::atomic_store(&snapshot, newSnap);
}

std::vector<PluginPool::InstanceId> PluginPool::findInstancesByType(const juce::PluginDescription& desc) const
{
    std::vector<InstanceId> result;
    auto wanted = desc.createIdentifierString();

    for (const auto& [id, e] : entries)
    {
        if (e.desc.createIdentifierString() == wanted)
            result.push_back(id);
    }

    return result;
}

