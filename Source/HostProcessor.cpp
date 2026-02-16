#include <juce_core/juce_core.h>
#include "HostProcessor.h"

HostProcessor::HostProcessor()
	: pool(formatManager)
{
    formatManager.addDefaultFormats();

    juce::PropertiesFile::Options opts;
    opts.applicationName = "XPulse";
    opts.filenameSuffix = "settings";
    opts.osxLibrarySubFolder = "Application Support";
    opts.folderName = "XPulse"; // optional on Windows

    appProps.setStorageParameters(opts);

    // Load cached plugin list immediately
    if (auto* pf = appProps.getUserSettings())
    {
        auto xmlText = pf->getValue("KnownPluginList");
        if (xmlText.isNotEmpty())
        {
            if (auto xml = juce::parseXML(xmlText))
                knownPluginList.recreateFromXml(*xml);
        }
    }


    // Start background scan (below)
    startBackgroundScan();
}

HostProcessor::~HostProcessor() 
{
    if (scannerThread)
    {
        //Waits for run() to exit
        scannerThread->stopThread(2000);
        scannerThread.reset();
    }
}


void HostProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    sr = sampleRate;
    bs = blockSize;

	pool.prepareToPlay(sr, bs);
   
}

void HostProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (hostedInstanceId == 0)
        return;

    if (auto* inst = pool.getInstanceForAudio(hostedInstanceId))
        inst->processBlock(buffer, midi);
}

void HostProcessor::unloadPlugin()
{
    if (hostedInstanceId == 0)
        return;

    pool.destroyInstance(hostedInstanceId);
    hostedInstanceId = 0;
}

void HostProcessor::loadPlugin(const juce::PluginDescription& desc)
{
    auto id = pool.createInstance(desc);
    if (id != 0)
        hostedInstanceId = id;
}


void HostProcessor::replacePlugin(const juce::PluginDescription& desc)
{
	unloadPlugin();
	loadPlugin(desc);
}

std::unique_ptr<juce::AudioProcessorEditor> HostProcessor::createHostedEditor()
{
    // UI thread only
    if (hostedInstanceId == 0)
        return {};

    return pool.createEditorFor(hostedInstanceId);
}

void HostProcessor::releaseResources()
{
	pool.releaseResources();
}

void HostProcessor::scanForPlugins()
{
    juce::KnownPluginList tempList;

    for (auto* f : formatManager.getFormats())
        DBG("Host format available: " + f->getName());

    for (auto* format : formatManager.getFormats())
    {
        if (!format->getName().containsIgnoreCase("vst3"))
            continue;

        juce::FileSearchPath searchPath;
        searchPath.add(juce::File("C:/Program Files/Common Files/VST3"));
        searchPath.add(juce::File("C:/Program Files/VST3"));
        searchPath.add(juce::File("C:/Program Files/Steinberg/VSTPlugins"));
        searchPath.add(juce::File("C:/Program Files/VSTPlugins"));

#if JUCE_MAC
        searchPath.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
        searchPath.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
#endif

        bool dontRescanIfUpToDate = true;
        juce::PluginDirectoryScanner scanner(tempList, *format, searchPath,
            dontRescanIfUpToDate,
            juce::File(),
            true);

        juce::String pluginBeingScanned;
        while (scanner.scanNextFile(true, pluginBeingScanned))
        {
        }
    }

    auto xml = tempList.createXml();

    {
        const juce::ScopedLock sl(pluginListLock);
        knownPluginList.clear();

        if (xml)
            knownPluginList.recreateFromXml(*xml);
    }
}

void HostProcessor::getKnownPluginTypesCopy(juce::Array<juce::PluginDescription>& out) const
{
    const juce::ScopedLock sl(pluginListLock);
    out = knownPluginList.getTypes();
}

void HostProcessor::startBackgroundScan()
{
    scanFinished.store(false);

    if (scannerThread && scannerThread->isThreadRunning())
        return;

    scannerThread = std::make_unique<ScannerThread>(*this);
    scannerThread->startThread();
}

void HostProcessor::ScannerThread::run()
{
    {
        host.scanForPlugins(); 
    }

    if (auto* pf = host.appProps.getUserSettings())
    {
        juce::String xmlText;

        {
            const juce::ScopedLock sl(host.pluginListLock);
            if (auto xml = host.knownPluginList.createXml())
                xmlText = xml->toString();
        }

        pf->setValue("KnownPluginList", xmlText);
        pf->saveIfNeeded();
    }


    host.scanFinished.store(true);
}


