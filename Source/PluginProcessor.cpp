/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
XPulseAudioProcessor::XPulseAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
	// Initialise band plugin instance IDs and send/return amounts to default values
    for (int b = 0; b < kNumBands; ++b)
    {
        for (int s = 0; s < kNumSlots; ++s)
        {
            bandPluginInstanceId[b][s].store(0, std::memory_order_relaxed);
            bandSendAmount[b][s].store(0.0f, std::memory_order_relaxed);
            bandReturnAmount[b][s].store(1.0f, std::memory_order_relaxed);
        }
    }
}

XPulseAudioProcessor::~XPulseAudioProcessor()
{
}

//==============================================================================
const juce::String XPulseAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XPulseAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool XPulseAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool XPulseAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double XPulseAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int XPulseAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int XPulseAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XPulseAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String XPulseAudioProcessor::getProgramName (int index)
{
    return {};
}

void XPulseAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================

//Important for all pre-playback initialisation ie Gain processors, Filters, Delays etc
void XPulseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
	spec.sampleRate = getSampleRate();
	spec.maximumBlockSize = getBlockSize();
    spec.numChannels = getTotalNumOutputChannels();

    
    // Gain processor
	prepareGainProcessor(spec);

	// Band filters
	prepareBandFilters(spec);
	updateBandFilterCutoffs();

    hostProcessor_.prepareToPlay(sampleRate, samplesPerBlock); // (important for hosted plugins too)


	// Removes per-block heap allocations by pre-sizing buffers
    auto numCh = getTotalNumOutputChannels();
    lowBuffer.setSize(numCh, samplesPerBlock);
    midBuffer.setSize(numCh, samplesPerBlock);
    highBuffer.setSize(numCh, samplesPerBlock);
    auxBuffer.setSize(numCh, samplesPerBlock);
	
}

void XPulseAudioProcessor::releaseResources()
{
	hostProcessor_.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool XPulseAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

// This is the Main plug-in processing block!!==============================
//==========================================================================
void XPulseAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	// ScopedNoDenormals is used to avoid denormalised numbers which can cause performance issues
	// Clears any output channels that don't have input data
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	//Process incoming MIDI messages
	processMidi(midiMessages);

	//Process audio
	processAudio(buffer);
    
}

//==============================================================================
bool XPulseAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* XPulseAudioProcessor::createEditor()
{
    return new XPulseAudioProcessorEditor (*this);
}

//==============================================================================
void XPulseAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void XPulseAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XPulseAudioProcessor();
}

//==============================================================================
#pragma region Custom Functions
//APVTS Parameter Layout Creation
juce::AudioProcessorValueTreeState::ParameterLayout XPulseAudioProcessor::createParameterLayout()
{
    //Creates paramter layout
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	//Parameter Creation (ID, Name, Min, Max, Default)

	//Gain Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("midGain","MidGain", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("lowGain", "LowGain", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("highGain", "HighGain", 0.0f, 1.0f, 0.5f));

	//Band Filters Cutoff Frequencies
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff", "Cutoff", 20.0f, 20000.0f, 1000.0f));

	//Band Split Frequencies
    auto hzRange = juce::NormalisableRange<float>(20.0f, 20000.0f);
    hzRange.setSkewForCentre(1000.0f); //Centre

    params.push_back(std::make_unique<juce::AudioParameterFloat>("lowMidCrossover", "Low-Mid Crossover", hzRange, 250.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("midHighCrossover", "Mid-High Crossover", hzRange, 4000.0f));


	//Return the parameter layout
	return { params.begin(), params.end() };
}

//Audio Processing Function
void XPulseAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer) {
	pitchDependent(buffer);
}

//MIDI Processing Function
void XPulseAudioProcessor::processMidi(juce::MidiBuffer& midiMessages) {
    
}

#pragma region PitchDependentProcessing
//Pitch-Dependent Processing Function Audio

void XPulseAudioProcessor::pitchDependent(juce::AudioBuffer<float>& buffer) {
	//Create copies of the main buffer for each band and ensures buffers are 
    // the correct size causing no  need to reallocate memory each block
    lowBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    midBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    highBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        lowBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        midBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        highBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

	//Process each band
	processLowBand(lowBuffer);
	processMidBand(midBuffer);
	processHighBand(highBuffer);

    //Runs sends into Hosted Plugins
    processHostedSends(lowBuffer, midBuffer, highBuffer);

	buffer.clear();

	//Mix the processed bands back into the main buffer
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.addFrom(channel, 0, lowBuffer, channel, 0, lowBuffer.getNumSamples());
        buffer.addFrom(channel, 0, midBuffer, channel, 0, midBuffer.getNumSamples());
        buffer.addFrom(channel, 0, highBuffer, channel, 0, highBuffer.getNumSamples());
	}
}

void XPulseAudioProcessor::processLowBand(juce::AudioBuffer<float>& buffer) {
    //Process Low Band

    //Apply Low-Pass Filter 
    //Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowBand.process(context);

    //Apply Gain 
    float lowGainValue = *parameters.getRawParameterValue("lowGain");    
    lowGainProcessor.setGainLinear(lowGainValue);
    lowGainProcessor.process(context);

}

void XPulseAudioProcessor::processMidBand(juce::AudioBuffer<float>& buffer) {
    //Process Mid Band

    //Apply Band-Pass Filter 
    //Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    auto& midHP = midBand.get<0>();
    auto& midLP = midBand.get<1>();
    midBand.process(context);

    //Apply Gain 
    float midGainValue = *parameters.getRawParameterValue("midGain");
    midGainProcessor.setGainLinear(midGainValue);
    midGainProcessor.process(context);
}

void XPulseAudioProcessor::processHighBand(juce::AudioBuffer<float>& buffer) {
    //Process High Band

    //Apply High-Pass Filter
    //Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    highBand.process(context);

    //Apply Gain 
    float highGainValue = *parameters.getRawParameterValue("highGain");
    highGainProcessor.setGainLinear(highGainValue);
    highGainProcessor.process(context);


}

void XPulseAudioProcessor::setBandSplits(float lowMidHz, float midHighHz)
{
    lowMidHz = juce::jlimit(20.0f, 20000.0f, lowMidHz);
    midHighHz = juce::jlimit(20.0f, 20000.0f, midHighHz);

    const float minGapHz = 10.0f;
    if (midHighHz < lowMidHz + minGapHz)
        midHighHz = lowMidHz + minGapHz;

    if (auto* p1 = parameters.getParameter("lowMidCrossover"))
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p1);
        jassert(ranged);

        p1->beginChangeGesture();
        p1->setValueNotifyingHost(ranged->convertTo0to1(lowMidHz));
        p1->endChangeGesture();
    }

    if (auto* p2 = parameters.getParameter("midHighCrossover"))
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p2);
        jassert(ranged);

        p2->beginChangeGesture();
        p2->setValueNotifyingHost(ranged->convertTo0to1(midHighHz));
        p2->endChangeGesture();
    }
	updateBandFilterCutoffs();
}



//Pitch Dependent Processing MIDI
void XPulseAudioProcessor::pitchDependent(juce::MidiBuffer& midiMessages) {
	juce::MidiBuffer lowMidi, midMidi, highMidi;
    
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOnOrOff()) {
            int note = msg.getNoteNumber();
            if (note < 48) // Example: C2 and below = Low band
                lowMidi.addEvent(msg, metadata.samplePosition);
            else if (note < 78) // Example: C2-E5 = Mid band
                midMidi.addEvent(msg, metadata.samplePosition);
            else // F5 and above = High band
                highMidi.addEvent(msg, metadata.samplePosition);
        }
        else {
            // Non-note messages go to all bands, or handle as needed
            lowMidi.addEvent(msg, metadata.samplePosition);
            midMidi.addEvent(msg, metadata.samplePosition);
            highMidi.addEvent(msg, metadata.samplePosition);
        }
    }

	//Process each band
	processLowBand(lowMidi);
    processMidBand(midMidi);
    processHighBand(highMidi);

	midiMessages.clear();
    midiMessages.addEvents(lowMidi, 0, -1, 0);
    midiMessages.addEvents(midMidi, 0, -1, 0);
    midiMessages.addEvents(highMidi, 0, -1, 0);
}
void XPulseAudioProcessor::processLowBand(juce::MidiBuffer& midiMessages) {
	float totalVelocity = 0.0f;
    int length = 0;
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
			//Multipied by 127 to convert from 0.0-1.0 to 0-127 MIDI velocity range
			float velocity = msg.getVelocity() * 127;
			totalVelocity += velocity;
            length += 1;
        }
        else {
            // Non-note and Non-note on messages remain unchanged
            midiMessages.addEvent(msg, metadata.samplePosition);
        }
	}
    if (length > 0) {
        lowBandVelocity = int(totalVelocity / length);
    }

	//Here I will check Parameters For Velocity Based FX Modulation
	//This will be based on User Parameters set in the GUI
    
    //Reverb:
	//This will work by modyfing th Gain value for the wet signal based on the average velocity of the notes in the band
	//The Dry signal will remain unchanged
}
void XPulseAudioProcessor::processMidBand(juce::MidiBuffer& midiMessages) {
    float totalVelocity = 0.0f;
    int length = 0;
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        // Example: Transpose down an octave for low band
        if (msg.isNoteOn()) {
            //Multipied by 127 to convert from 0.0-1.0 to 0-127 MIDI velocity range
            float velocity = msg.getVelocity() * 127;
            totalVelocity += velocity;
            length += 1;
        }
        else {
            // Non-note messages and Non-note onremain unchanged
            midiMessages.addEvent(msg, metadata.samplePosition);
        }
    }
    if(length > 0) {
        midBandVelocity = int(totalVelocity / length);
    }
    //Here I will check Parameters For Velocity Based FX Modulation
    //This will be based on User Parameters set in the GUI
       
    //Reverb:
    //This will work by modyfing th Gain value for the wet signal based on the average velocity of the notes in the band
    //The Dry signal will remain unchanged
}
void XPulseAudioProcessor::processHighBand(juce::MidiBuffer& midiMessages) {
    float totalVelocity = 0.0f;
    int length = 0;
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        // Example: Transpose down an octave for low band
        if (msg.isNoteOn()) {
            //Multipied by 127 to convert from 0.0-1.0 to 0-127 MIDI velocity range
            float velocity = msg.getVelocity() * 127;
            totalVelocity += velocity;
            length += 1;
        }
        else {
            // Non-note and Non=note on messages remain unchanged
            midiMessages.addEvent(msg, metadata.samplePosition);
        }
    }
    if (length > 0) {
        highBandVelocity = int(totalVelocity / length);
    }
    //Here I will check Parameters For Velocity Based FX Modulation
    //This will be based on User Parameters set in the GUI

    //Reverb:
    //This will work by modyfing th Gain value for the wet signal based on the average velocity of the notes in the band
    //The Dry signal will remain unchanged
}
#pragma endregion

#pragma region PrepareToPlayFuncions
void XPulseAudioProcessor::prepareGainProcessor(const juce::dsp::ProcessSpec& spec) {

    midGainProcessor.prepare(spec);
    lowGainProcessor.prepare(spec);
    highGainProcessor.prepare(spec);
    midGainProcessor.reset();
    lowGainProcessor.reset();
    highGainProcessor.reset();
}

void XPulseAudioProcessor::prepareBandFilters(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    // 1) set initial states FIRST (non-null)
    lowBand.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 250.0f);

    auto& midHP = midBand.get<0>();
    auto& midLP = midBand.get<1>();
    midHP.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 250.0f);
    midLP.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 4000.0f);

    highBand.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 4000.0f);

    // 2) now prepare
    lowBand.prepare(spec);
    midBand.prepare(spec);
    highBand.prepare(spec);

    // 3) now reset
    lowBand.reset();
    midBand.reset();
    highBand.reset();
}


void XPulseAudioProcessor::updateBandFilterCutoffs()
{
    if (currentSampleRate <= 0.0)
    {
        DBG("[XPulse] currentSampleRate is invalid: " + juce::String(currentSampleRate));
        return;
    }

    auto* pLo = parameters.getRawParameterValue("lowMidCrossover");
    auto* pHi = parameters.getRawParameterValue("midHighCrossover");


    if (pLo == nullptr || pHi == nullptr)
    {
        DBG("[XPulse] Crossover parameter pointers are null");
        return;
    }

    float lo = *pLo;
    float hi = *pHi;

   
    // Clamp to safe range AND nyquist-safe range
    const float nyquistSafe = (float)(0.49 * currentSampleRate);
    lo = juce::jlimit(20.0f, nyquistSafe, lo);
    hi = juce::jlimit(20.0f, nyquistSafe, hi);

    // enforce ordering + gap
    const float minGapHz = 10.0f;
    if (hi < lo + minGapHz) hi = juce::jmin(nyquistSafe, lo + minGapHz);

    // state pointers must exist
    auto& midHP = midBand.get<0>();
    auto& midLP = midBand.get<1>();

    if (lowBand.state == nullptr || midHP.state == nullptr || midLP.state == nullptr || highBand.state == nullptr)
    {
        DBG("[XPulse] One or more filter state pointers are null!");
        DBG("  lowBand.state: " + juce::String::toHexString((juce::uint64)(uintptr_t)lowBand.state.get()));
        DBG("  midHP.state: " + juce::String::toHexString((juce::uint64)(uintptr_t)midHP.state.get()));
        DBG("  midLP.state: " + juce::String::toHexString((juce::uint64)(uintptr_t)midLP.state.get()));
        DBG("  highBand.state: " + juce::String::toHexString((juce::uint64)(uintptr_t)highBand.state.get()));
        return;
    }

    DBG("[XPulse] updateBandFilterCutoffs: sampleRate=" + juce::String(currentSampleRate) + ", lo=" + juce::String(lo) + ", hi=" + juce::String(hi));

    // IMPORTANT: swap pointer, don't mutate *state in place
    lowBand.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, lo);
    midHP.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, lo);
    midLP.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, hi);
    highBand.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, hi);
}



#pragma endregion

#pragma region HostedPluginSends
void XPulseAudioProcessor::processHostedSends(juce::AudioBuffer<float>& low,
    juce::AudioBuffer<float>& mid,
    juce::AudioBuffer<float>& high)
{
    juce::MidiBuffer emptyMidi;

    const auto numCh = low.getNumChannels();
    const auto numSamp = low.getNumSamples();

    // Each slot can route to an arbitrary hosted instance.
    // bandPluginInstanceId[3][3], bandSendAmount[3][3], bandReturnAmount[3][3]
    static constexpr int kNumBands = 3;
    static constexpr int kNumSlots = 3;

    // Gather unique instance IDs across all band/slot routes
    PluginPool::InstanceId usedIds[kNumBands * kNumSlots] = {};
    int numUsed = 0;

    auto pushUnique = [&](PluginPool::InstanceId id)
        {
            if (id == 0) return;

            for (int i = 0; i < numUsed; ++i)
                if (usedIds[i] == id)
                    return;

            usedIds[numUsed++] = id;
        };

    for (int band = 0; band < kNumBands; ++band)
        for (int slot = 0; slot < kNumSlots; ++slot)
            pushUnique((PluginPool::InstanceId)bandPluginInstanceId[band][slot].load(std::memory_order_relaxed));

    // For each unique hosted instance, sum all sends targeting it, process once, then return to all targets.
    for (int u = 0; u < numUsed; ++u)
    {
        const auto id = usedIds[u];

        auto* plugin = hostProcessor_.getPool().getInstanceForAudio(id);
        if (!plugin)
            continue;

        auxBuffer.setSize(numCh, numSamp, false, false, true);
        auxBuffer.clear();

        // Sum sends from any band/slot that routes to this instance id
        auto sumSendFrom = [&](int bandIndex, int slotIndex, juce::AudioBuffer<float>& bandBuf)
            {
                const auto routedId =
                    (PluginPool::InstanceId)bandPluginInstanceId[bandIndex][slotIndex].load(std::memory_order_relaxed);

                if (routedId != id)
                    return;

                const float send =
                    bandSendAmount[bandIndex][slotIndex].load(std::memory_order_relaxed);

                if (send <= 0.0001f)
                    return;

                for (int ch = 0; ch < numCh; ++ch)
                    auxBuffer.addFrom(ch, 0, bandBuf, ch, 0, numSamp, send);
            };

        // Low band
        for (int slot = 0; slot < kNumSlots; ++slot)
            sumSendFrom(0, slot, low);

        // Mid band
        for (int slot = 0; slot < kNumSlots; ++slot)
            sumSendFrom(1, slot, mid);

        // High band
        for (int slot = 0; slot < kNumSlots; ++slot)
            sumSendFrom(2, slot, high);

        // Process hosted plugin once for this instance id
        plugin->processBlock(auxBuffer, emptyMidi);

        // Return wet back to any band/slot that routes to this instance id
        auto returnTo = [&](int bandIndex, int slotIndex, juce::AudioBuffer<float>& bandBuf)
            {
                const auto routedId =
                    (PluginPool::InstanceId)bandPluginInstanceId[bandIndex][slotIndex].load(std::memory_order_relaxed);

                if (routedId != id)
                    return;

                const float ret =
                    bandReturnAmount[bandIndex][slotIndex].load(std::memory_order_relaxed);

               
                if (ret <= 0.0001f)
                    return;

                for (int ch = 0; ch < numCh; ++ch)
                    bandBuf.addFrom(ch, 0, auxBuffer, ch, 0, numSamp, ret);
            };

        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(0, slot, low);
        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(1, slot, mid);
        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(2, slot, high);
    }
}



#pragma endregion

#pragma endregion
