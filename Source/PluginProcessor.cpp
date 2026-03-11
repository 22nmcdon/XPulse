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

    for (int b = 0; b < 3; ++b)
    {
        for (int s = 0; s < 3; ++s)
        {
            sendSmooth[b][s].reset(sampleRate, 0.02); // 20ms smoothing applies below as well
            sendSmooth[b][s].setCurrentAndTargetValue(0.0f);
        }
    }

    for (int b = 0; b < 3; ++b)
    {
        velSmooth[b].reset(sampleRate, 0.02);
        velSmooth[b].setCurrentAndTargetValue(0.5f);
    }
    for (int b = 0; b < 3; ++b)
    {
        pedalSendSmooth[b].reset(sampleRate, 0.02);
        pedalSendSmooth[b].setCurrentAndTargetValue(0.0f);
    }
	
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
    hzRange.setSkewForCentre(1000.0f); 

    params.push_back(std::make_unique<juce::AudioParameterFloat>("lowMidCrossover", "Low-Mid Crossover", hzRange, 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("midHighCrossover", "Mid-High Crossover", hzRange, 4000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lowMidCrossoverMidi","Low-Mid Crossover MIDI",juce::NormalisableRange<float>(28.0f, 100.0f, 1.0f),48.0f)); 
    params.push_back(std::make_unique<juce::AudioParameterFloat>("midHighCrossoverMidi","Mid-High Crossover MIDI",juce::NormalisableRange<float>(28.0f, 100.0f, 1.0f),72.0f));

    // MIDI send paramters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowVelToSend", "Low Band Vel To Send", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "midVelToSend", "Mid Band Vel To Send", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highVelToSend", "High Band Vel To Send", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

	//Return the parameter layout
	return { params.begin(), params.end() };
}

//Audio Processing Function
void XPulseAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer)
{
    //Dirty Flag
    if (crossoverDirty.exchange(false, std::memory_order_acq_rel))
    {
        updateBandFilterCutoffs();
    }

	pitchDependent(buffer);
}

//MIDI Processing Function
void XPulseAudioProcessor::processMidi(juce::MidiBuffer& midiMessages) 
{
	pitchDependent(midiMessages);
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

    //Dirty Flag
    //crossoverDirty.store(true, std::memory_order_release);

	updateBandFilterCutoffs();
}



// Overloaded Pitch Dependent Processing for MIDI

void XPulseAudioProcessor::pitchDependent(juce::MidiBuffer& midiMessages)
{
	juce::MidiBuffer lowMidi, midMidi, highMidi;

	//Gets the crossover frequencies from the parameters to determine how to split the MIDI messages into bands
	float lowMidMidi = *parameters.getRawParameterValue("lowMidCrossoverMidi");
	float midHighMidi = *parameters.getRawParameterValue("midHighCrossoverMidi");

    if (midiMessages.isEmpty()) {
		return; // No MIDI messages to process
    }


    for (const auto metadata : midiMessages) 
    {
        const auto msg = metadata.getMessage();

        //Pedal messages & others
        if (msg.isController() && msg.getControllerNumber() == 64)
            sustainDown.store(msg.getControllerValue() >= 64, std::memory_order_relaxed);

        if (msg.isNoteOnOrOff()) {
            int note = msg.getNoteNumber();
			DBG("Note: " << note);

            if (note < lowMidMidi)
                lowMidi.addEvent(msg, metadata.samplePosition);
            else if (note > midHighMidi)
                highMidi.addEvent(msg, metadata.samplePosition);
            else 
                midMidi.addEvent(msg, metadata.samplePosition);
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
void XPulseAudioProcessor::processLowBand(juce::MidiBuffer& midiMessages)
{
    int highVelocity = 0;

    juce::MidiBuffer filtered;
    filtered.ensureSize(midiMessages.getNumEvents());

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            const int vel127 = juce::jlimit(0, 127, (int)std::lround(msg.getVelocity() * 127.0f));
            highVelocity = juce::jmax(highVelocity, vel127);

            filtered.addEvent(msg, metadata.samplePosition);
        }
        else
        {
            filtered.addEvent(msg, metadata.samplePosition);
        }
    }

    midiMessages.swapWith(filtered);

    lowBandVelocity.store(highVelocity, std::memory_order_relaxed);
}
void XPulseAudioProcessor::processMidBand(juce::MidiBuffer& midiMessages) 
{
    int highVelocity = 0;

    juce::MidiBuffer filtered;
    filtered.ensureSize(midiMessages.getNumEvents());

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            const int vel127 = juce::jlimit(0, 127, (int)std::lround(msg.getVelocity() * 127.0f));
            highVelocity = juce::jmax(highVelocity, vel127);

            filtered.addEvent(msg, metadata.samplePosition);
        }
        else
        {
            filtered.addEvent(msg, metadata.samplePosition);
        }
    }

    midiMessages.swapWith(filtered);

    midBandVelocity.store(highVelocity, std::memory_order_relaxed);
}
void XPulseAudioProcessor::processHighBand(juce::MidiBuffer& midiMessages)
{
    int highVelocity = 0;

    juce::MidiBuffer filtered;
    filtered.ensureSize(midiMessages.getNumEvents());

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            const int vel127 = juce::jlimit(0, 127, (int)std::lround(msg.getVelocity() * 127.0f));
            highVelocity = juce::jmax(highVelocity, vel127);

            filtered.addEvent(msg, metadata.samplePosition);
        }
        else
        {
            filtered.addEvent(msg, metadata.samplePosition);
        }
    }

    midiMessages.swapWith(filtered);

    highBandVelocity.store(highVelocity, std::memory_order_relaxed);
}

float XPulseAudioProcessor::getVelocitySendMultiplier(int band) const
{
    // Read velToSend parameter for the band (0..1, where 0.5 is neutral)
    const float velToSend =
        (band == 0 ? *parameters.getRawParameterValue("lowVelToSend") :
            band == 1 ? *parameters.getRawParameterValue("midVelToSend") :
            *parameters.getRawParameterValue("highVelToSend"));

    // Read last velocity for the band (0..127)
    const int vel127 =
        (band == 0 ? (int)lowBandVelocity.load(std::memory_order_relaxed) :
            band == 1 ? (int)midBandVelocity.load(std::memory_order_relaxed) :
            (int)highBandVelocity.load(std::memory_order_relaxed));

    float velNorm = juce::jlimit(0.0f, 1.0f, vel127 / 127.0f);

    // gamma > 1 emphasizes extremities
    const float gamma = 2.2f;
    velNorm = std::pow(velNorm, gamma);

    // a lower pivot increases the amount of high range vs low range
    const float pivot = 0.35f;
    float centered = (velNorm - pivot) / (1.0f - pivot); 
    centered = centered * 2.0f - 1.0f;
    const float depth = (velToSend - 0.5f) * 2.0f;

    

    float mult = 1.0f + centered * depth;                              
    mult = juce::jlimit(0.0f, 2.0f, mult);

    return mult;
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

    auto* pLo = parameters.getRawParameterValue("lowMidCrossover");
    auto* pHi = parameters.getRawParameterValue("midHighCrossover");

    
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

    // IMPORTANT: overwrite existing coefficients, don’t swap pointers
    *lowBand.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, lo);
    *midHP.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, lo);
    *midLP.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, hi);
    *highBand.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, hi);

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
	// Gather unique instance IDs across all bandSlot routes
    for (int band = 0; band < kNumBands; ++band)
        for (int slot = 0; slot < kNumSlots; ++slot)
            pushUnique((PluginPool::InstanceId)bandPluginInstanceId[band][slot].load(std::memory_order_relaxed));
	// Also include any instance IDs from pedal sends (if sustain is down)
    for (int band = 0; band < 3; ++band)
        pushUnique((PluginPool::InstanceId)pedalPluginInstanceId[band].load(std::memory_order_relaxed));
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

            const float baseSend =
                bandSendAmount[bandIndex][slotIndex].load(std::memory_order_relaxed);

            if (baseSend <= 0.0001f)
                return;

            const float velMult = getVelocitySendMultiplier(bandIndex);
            const float floor = 0.1f;
            const float targetSend = juce::jlimit(baseSend * floor, 1.0f, baseSend * velMult);

            auto& sm = sendSmooth[bandIndex][slotIndex];
            sm.setTargetValue(targetSend);

            //const float send = sm.getNextValue(); // one step per block

            //if (send <= 0.0001f)
            //    return;

            for (int n = 0; n < numSamp; ++n)
            {
                const float g = sm.getNextValue();

                for (int ch = 0; ch < numCh; ++ch)
                    auxBuffer.setSample(ch, n,
                        auxBuffer.getSample(ch, n) + bandBuf.getSample(ch, n) * g);
            }
        };

        auto sumPedalFrom = [&](int bandIndex, juce::AudioBuffer<float>& bandBuf)
            {
                const auto routedId =
                    (PluginPool::InstanceId)pedalPluginInstanceId[bandIndex].load(std::memory_order_relaxed);

                if (routedId != id)
                    return;

                float target = pedalSendAmount[bandIndex].load(std::memory_order_relaxed);
                auto& sm = pedalSendSmooth[bandIndex];
                sm.setTargetValue(target);

                float send = sm.getNextValue(); // per-block smoothing

                // Gate by sustain pedal
                if (!sustainDown.load(std::memory_order_relaxed))
                    send = 0.0f;

                if (send <= 0.0001f)
                    return;

                for (int ch = 0; ch < numCh; ++ch)
                    auxBuffer.addFrom(ch, 0, bandBuf, ch, 0, numSamp, send);
            };

        // add pedal sends
        sumPedalFrom(0, low);
        sumPedalFrom(1, mid);
        sumPedalFrom(2, high);

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
       
        auto returnPedalTo = [&](int bandIndex, juce::AudioBuffer<float>& bandBuf)
            {
                const auto routedId =
                    (PluginPool::InstanceId)pedalPluginInstanceId[bandIndex].load(std::memory_order_relaxed);

                if (routedId != id)
                    return;

                const float ret = pedalReturnAmount[bandIndex].load(std::memory_order_relaxed);
                if (ret <= 0.0001f)
                    return;

                for (int ch = 0; ch < numCh; ++ch)
                    bandBuf.addFrom(ch, 0, auxBuffer, ch, 0, numSamp, ret);
            };

        returnPedalTo(0, low);
        returnPedalTo(1, mid);
        returnPedalTo(2, high);
        // Return Sends for each band and its corresponding slots
        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(0, slot, low);
        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(1, slot, mid);
        for (int slot = 0; slot < kNumSlots; ++slot) returnTo(2, slot, high);
    }
}



#pragma endregion

#pragma endregion
