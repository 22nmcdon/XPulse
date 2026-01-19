#include <juce_core/juce_core.h>
#include "HostProcessor.h"

HostProcessor::HostProcessor()
{
    formatManager.addDefaultFormats();
}

HostProcessor::~HostProcessor() = default;

void HostProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    sr = sampleRate;
    bs = blockSize;

    juce::CriticalSection::ScopedLockType lock(hostedLock);
    if (hosted)
        hosted->prepareToPlay(sr, bs);
}

void HostProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Never block the audio thread
    const juce::SpinLock::ScopedTryLockType lock(hostedSpinLock);
    if (!lock.isLocked() || !hosted)
        return;

    hosted->processBlock(buffer, midi);
}

void HostProcessor::unloadPlugin()
{
    const juce::SpinLock::ScopedLockType lock(hostedSpinLock);
    hosted.reset();
}


void HostProcessor::loadPlugin(const juce::PluginDescription& desc)
{
    juce::String error;

    std::unique_ptr<juce::AudioPluginInstance> instance
        = formatManager.createPluginInstance(desc, sr, bs, error);

    if (!instance)
    {
        // log error somewhere visible
        return;
    }

    instance->prepareToPlay(sr, bs);

    const juce::ScopedLock sl(hostedLock);
    hosted = std::move(instance);
}


void HostProcessor::replacePlugin(const juce::PluginDescription& desc) {
	releaseResources();
	unloadPlugin();
	loadPlugin(desc);
}

std::unique_ptr<juce::AudioProcessorEditor> HostProcessor::createHostedEditor()
{
    // Must be called from the UI thread
    const juce::ScopedLock sl(hostedLock);

    if (!hosted || !hosted->hasEditor())
        return {};

    // If you use createEditorIfNeeded(), JUCE may return an existing editor.
    // It returns a raw pointer that the processor owns, so DON'T wrap it in unique_ptr.
    // Prefer createEditor() which gives you ownership.

    return std::unique_ptr<juce::AudioProcessorEditor>(hosted->createEditor());
}

void HostProcessor::releaseResources()
{
    const juce::ScopedLock sl(hostedLock);

    if (hosted)
        hosted->releaseResources();
}

void HostProcessor::scanForPlugins()
{

	// Will most likly be moved to a background thread in a real app
    knownPluginList.clear();

    for (auto* f : formatManager.getFormats())
        DBG("Host format available: " + f->getName());


    for (auto* format : formatManager.getFormats())
    {
//#if defined(JUCE_WINDOWS) || defined(_WIN32) || defined(_WIN64)
        // Only scan VST3 on Windows
        if (format->getName() != "VST3")
            continue;
//#endif

        juce::FileSearchPath searchPath;

//#if defined(JUCE_WINDOWS) || defined(_WIN32) || defined(_WIN64)
        // Common VST3 install locations
        searchPath.add(juce::File("C:/Program Files/Common Files/VST3"));
        searchPath.add(juce::File("C:/Program Files/VST3"));
        searchPath.add(juce::File("C:/Program Files/Steinberg/VSTPlugins"));
        searchPath.add(juce::File("C:/Program Files/VSTPlugins"));
//#endif

#if JUCE_MAC
        // Common plugin locations on macOS (VST3 + AU components live elsewhere too)
        searchPath.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
        searchPath.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
        // AU scan is handled via AudioUnit format discovery, but including common paths doesn’t hurt
#endif

        bool dontRescanIfUpToDate = true;
        juce::PluginDirectoryScanner scanner(knownPluginList, *format, searchPath,
            dontRescanIfUpToDate,
            juce::File(),
            true);

        juce::String pluginBeingScanned;
        while (scanner.scanNextFile(true, pluginBeingScanned))
        {
            // you can log pluginBeingScanned if you want
        }

    }
}
