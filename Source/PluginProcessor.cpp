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

//Important for all pre-playback initialisation ie Gain processors, Filters, Delays, Reverbs etc
void XPulseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    juce::dsp::ProcessSpec spec;
	spec.sampleRate = getSampleRate();
	spec.maximumBlockSize = getBlockSize();
    spec.numChannels = getTotalNumOutputChannels();
    
	// Host Processor
    hostProcessor_.prepareToPlay(sampleRate, samplesPerBlock);


    // Gain processor
	prepareGainProcessor(spec);

	// Band filters
	prepareBandFilters(spec);

	// Reverb
	prepareReverbProcessor(spec);
	
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


	//Important Host Processing
	hostProcessor_.processBlock(buffer, midiMessages);

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

	//Reverb Parameters: roomSize, damping, wetLevel, dryLevel, width, freezeMode, Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>("roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("damping", "Damping", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetLevel", "Wet Level", 0.0f, 1.0f, 0.33f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryLevel", "Dry Level", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("width", "Width", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("freezeMode", "Freeze Mode", 0.0f, 1.0f, 0.0f));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("highReverbMaster", "Reverb Master", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("midReverbMaster", "Reverb Master", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("lowReverbMaster", "Reverb Master", 0.0f, 1.0f, 0.5f));

	//Reverb Preset Selection
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lowReverbPreset", "Low Reverb Preset",
		juce::StringArray{ "Small Room", "Concert Hall", "Dark Room", "Bright Room", "Ambient Room" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("midReverbPreset", "Mid Reverb Preset",
        juce::StringArray{ "Small Room", "Concert Hall", "Dark Room", "Bright Room", "Ambient Room" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("highReverbPreset", "High Reverb Preset",
        juce::StringArray{ "Small Room", "Concert Hall", "Dark Room", "Bright Room", "Ambient Room" }, 0));


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

//Load Reverb Preset Function
void XPulseAudioProcessor::loadReverbPreset(int presetIndex, char band)
{
    DBG("loadReverbPreset called: presetIndex=" << presetIndex << ", band='" << band << "'");
    // Preset values (0.0–1.0)
    struct Preset { float room, damp, wet, dry, width, freeze; };
    static const Preset presets[] = {
        { 0.2f, 0.3f, 0.2f, 0.9f, 0.5f, 0.0f }, // Small Room
        { 0.8f, 0.5f, 0.4f, 0.8f, 1.0f, 0.0f }, // Concert Hall
        { 0.4f, 0.9f, 0.3f, 0.85f, 0.6f, 0.0f }, // Dark Room
        { 0.6f, 0.1f, 0.35f, 0.9f, 1.0f, 0.0f }, // Bright Room
        { 0.9f, 0.5f, 0.6f, 0.5f, 1.0f, 1.0f }, // Ambient Room
    };
    if (presetIndex >= 0 && presetIndex < 5)
    {
        // Set APVTS parameters (normalized 0.0–1.0)
        if (band == 'l') {
            lowBaseRoomSize = presets[presetIndex].room;
            lowBaseDamping = presets[presetIndex].damp;
            lowBaseWetLevel = presets[presetIndex].wet;
            lowBaseDryLevel = presets[presetIndex].dry;
            lowBaseWidth = presets[presetIndex].width;
            lowBaseFreezeMode = presets[presetIndex].freeze;
        }
        else if (band == 'm') {
            midBaseRoomSize = presets[presetIndex].room;
            midBaseDamping = presets[presetIndex].damp;
            midBaseWetLevel = presets[presetIndex].wet;
            midBaseDryLevel = presets[presetIndex].dry;
            midBaseWidth = presets[presetIndex].width;
            midBaseFreezeMode = presets[presetIndex].freeze;
		}
		else if (band == 'h') {
		    highBaseRoomSize = presets[presetIndex].room;
		    highBaseDamping = presets[presetIndex].damp;
		    highBaseWetLevel = presets[presetIndex].wet;
		    highBaseDryLevel = presets[presetIndex].dry;
		    highBaseWidth = presets[presetIndex].width;
		    highBaseFreezeMode = presets[presetIndex].freeze;
        }
    }
}

#pragma region PitchDependentProcessing
//Pitch-Dependent Processing Function Audio

void XPulseAudioProcessor::pitchDependent(juce::AudioBuffer<float>& buffer) {
	juce::AudioBuffer<float> lowBuffer, midBuffer, highBuffer;
	lowBuffer.makeCopyOf(buffer);
	midBuffer.makeCopyOf(buffer);
	highBuffer.makeCopyOf(buffer);

	//Process each band
	processLowBand(lowBuffer);
	processMidBand(midBuffer);
	processHighBand(highBuffer);

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

	//Apply Low-Pass Filter ----
    //Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
	lowBand.process(context);

	//Apply Gain ----
	float lowGainValue = *parameters.getRawParameterValue("lowGain");
	lowGainProcessor.setGainLinear(lowGainValue);
    lowGainProcessor.process(context);


	//Apply Reverb ----
	/*float master = *parameters.getRawParameterValue("lowReverbMaster");
    lowBandReverbParameters.roomSize = lowBaseRoomSize;
    lowBandReverbParameters.damping = lowBaseDamping * master;
    lowBandReverbParameters.wetLevel = lowBaseWetLevel * master;
    lowBandReverbParameters.dryLevel = lowBaseDryLevel;
    lowBandReverbParameters.width = lowBaseWidth * master;
    lowBandReverbParameters.freezeMode = lowBaseFreezeMode;
    lowBandReverbProcessor.setParameters(lowBandReverbParameters);*/
	lowBandReverbProcessor.process(context);
}

void XPulseAudioProcessor::processMidBand(juce::AudioBuffer<float>& buffer) {
    //Process Mid Band

    //Apply Band-Pass Filter ----
	//Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
	midBand.process(context);

	//Apply Gain ----
	float midGainValue = *parameters.getRawParameterValue("midGain");
	midGainProcessor.setGainLinear(midGainValue);
    midGainProcessor.process(context);

	//Apply Reverb ----
    /*float master = *parameters.getRawParameterValue("midReverbMaster");
    midBandReverbParameters.roomSize = midBaseRoomSize;
    midBandReverbParameters.damping = midBaseDamping * master;
    midBandReverbParameters.wetLevel = midBaseWetLevel * master;
    midBandReverbParameters.dryLevel = midBaseDryLevel;
    midBandReverbParameters.width = midBaseWidth * master;
    midBandReverbParameters.freezeMode = midBaseFreezeMode;
	midBandReverbProcessor.setParameters(midBandReverbParameters);*/
    midBandReverbProcessor.process(context);
}

void XPulseAudioProcessor::processHighBand(juce::AudioBuffer<float>& buffer) {
    //Process High Band

	//Apply High-Pass Filter ----
    //Build an AudioBlock and process it with the DSP processors
    juce::dsp::AudioBlock<float> block(buffer);
	juce::dsp::ProcessContextReplacing<float> context(block);
	highBand.process(context);

	//Apply Reverb ----
	float highGainValue = *parameters.getRawParameterValue("highGain");
	highGainProcessor.setGainLinear(highGainValue);
	highGainProcessor.process(context);

	//Apply Reverb ----
    /*float master = *parameters.getRawParameterValue("highReverbMaster");
    highBandReverbParameters.roomSize = highBaseRoomSize;
    highBandReverbParameters.damping = highBaseDamping * master;
    highBandReverbParameters.wetLevel = highBaseWetLevel * master;
    highBandReverbParameters.dryLevel = highBaseDryLevel;
    highBandReverbParameters.width = highBaseWidth * master;
    highBandReverbParameters.freezeMode = highBaseFreezeMode;
    highBandReverbProcessor.setParameters(highBandReverbParameters);*/
	highBandReverbProcessor.process(context);
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

void XPulseAudioProcessor::prepareBandFilters(const juce::dsp::ProcessSpec& spec) {
    // Band filters Setup
    if (lowBandCoefficients == nullptr)
		lowBandCoefficients = new juce::dsp::IIR::Coefficients<float>();
    *lowBandCoefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 200.0f); //Above 200Hz is cutoff
    lowBand.state = lowBandCoefficients;

    auto& midHighPass = midBand.get<0>();
    auto& midLowPass = midBand.get<1>();

	if (midHighPass.state == nullptr)
        midHighPass.state = new juce::dsp::IIR::Coefficients<float>();
	*midHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 200.0f); //Below 200Hz is cutoff
	midHighPass.state = midHighPass.state;

	if (midLowPass.state == nullptr)
		midLowPass.state = new juce::dsp::IIR::Coefficients<float>();
    *midLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 2000.0f); //Above 2000Hz is cutoff
	midLowPass.state = midLowPass.state;

	if (highBandCoefficients == nullptr)
        highBandCoefficients = new juce::dsp::IIR::Coefficients<float>();
	*highBandCoefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 2000.0f); //Below 2000Hz is cutoff
	highBand.state = highBandCoefficients;

	// Prepare/Reset filters
    lowBand.prepare(spec);
    midBand.prepare(spec);
    highBand.prepare(spec);
    lowBand.reset();
    midBand.reset();
	highBand.reset();
}

void XPulseAudioProcessor::prepareReverbProcessor(const juce::dsp::ProcessSpec& spec) {
    // Reverb Bands Setup
    highBandReverbProcessor.prepare(spec);
    midBandReverbProcessor.prepare(spec);
    lowBandReverbProcessor.prepare(spec);


    // Set some default reverb parameters for the bands
    //High Band Reverb
    highBandReverbParameters.roomSize = 0.5f; // 0.0 to 1.0
    highBandReverbParameters.damping = 0.5f; // 0.0 to 1.0
    highBandReverbParameters.wetLevel = 0.33f; // 0.0 to 1.0
    highBandReverbParameters.dryLevel = 0.4f; // 0.0 to 1.0
    highBandReverbParameters.width = 1.0f; // 0.0 to 1.0
    highBandReverbParameters.freezeMode = 0.0f; // 0.0 (off) to 1.0 (infinite reverb)
    highBandReverbProcessor.setParameters(highBandReverbParameters);

    //Mid Band Reverb
    midBandReverbParameters.roomSize = 0.5f; // 0.0 to 1.0
    midBandReverbParameters.damping = 0.5f; // 0.0 to 1.0
    midBandReverbParameters.wetLevel = 0.33f; // 0.0 to 1.0
    midBandReverbParameters.dryLevel = 0.4f; // 0.0 to 1.0
    midBandReverbParameters.width = 1.0f; // 0.0 to 1.0
    midBandReverbParameters.freezeMode = 0.0f; // 0.0 (off) to 1.0 (infinite reverb)
    midBandReverbProcessor.setParameters(midBandReverbParameters);

    //Low Band Reverb
    lowBandReverbParameters.roomSize = 0.5f; // 0.0 to 1.0
    lowBandReverbParameters.damping = 0.5f; // 0.0 to 1.0
    lowBandReverbParameters.wetLevel = 0.33f; // 0.0 to 1.0
    lowBandReverbParameters.dryLevel = 0.4f; // 0.0 to 1.0
    lowBandReverbParameters.width = 1.0f; // 0.0 to 1.0
    lowBandReverbParameters.freezeMode = 0.0f; // 0.0 (off) to 1.0 (infinite reverb)
    lowBandReverbProcessor.setParameters(lowBandReverbParameters);

}
#pragma endregion

#pragma endregion
