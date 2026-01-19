/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "HostProcessor.h"

//==============================================================================
/**
*/
class XPulseAudioProcessor  : public juce::AudioProcessor
{
public:
	//Custom Prepare Functions
	void prepareGainProcessor(const juce::dsp::ProcessSpec& spec);
	void prepareBandFilters(const juce::dsp::ProcessSpec& spec);
	void prepareReverbProcessor(const juce::dsp::ProcessSpec& spec);

	//Host Processor Functions
	HostProcessor& getHostProcessor() { return hostProcessor_; }
	const HostProcessor& getHostProcessor() const { return hostProcessor_; }

    // PitchDependent Functions for Audio
    void pitchDependent(juce::AudioBuffer<float>& buffer);
	void processLowBand(juce::AudioBuffer<float>& buffer);
	void processMidBand(juce::AudioBuffer<float>& buffer);
	void processHighBand(juce::AudioBuffer<float>& buffer);

	void pitchDependent(juce::MidiBuffer& midiMessages);
	void processLowBand(juce::MidiBuffer& midiMessages);
	void processMidBand(juce::MidiBuffer& midiMessages);
	void processHighBand(juce::MidiBuffer& midiMessages);

	//Custom Processing Functions
	void loadReverbPreset(int presetIndex, char band);
	void processAudio(juce::AudioBuffer<float>& buffer);
	void processMidi(juce::MidiBuffer& midiMessages);
    
    //Custom Variables
    //MIDI Band Velocities
	int lowBandVelocity = 0;
	int midBandVelocity = 0;
	int highBandVelocity = 0;
    
    //Base Reverb Parameters: 0.8f, 0.5f, 0.4f, 0.8f, 1.0f, 0.0f
    float lowBaseRoomSize = 0.8f, lowBaseDamping = 0.5f, lowBaseWetLevel = 0.4f, lowBaseDryLevel = 0.8f, lowBaseWidth = 1.0f, lowBaseFreezeMode = 0.0f;
    float midBaseRoomSize = 0.8f, midBaseDamping = 0.5f, midBaseWetLevel = 0.4f, midBaseDryLevel = 0.8f, midBaseWidth = 1.0f, midBaseFreezeMode = 0.0f;
    float highBaseRoomSize = 0.8f, highBaseDamping = 0.5f, highBaseWetLevel = 0.4f, highBaseDryLevel = 0.8f, highBaseWidth = 1.0f, highBaseFreezeMode = 0.0f;

    //Reverb processor
    juce::dsp::Reverb highBandReverbProcessor;
    juce::dsp::Reverb::Parameters highBandReverbParameters;
    juce::dsp::Reverb midBandReverbProcessor;
    juce::dsp::Reverb::Parameters midBandReverbParameters;
    juce::dsp::Reverb lowBandReverbProcessor;
    juce::dsp::Reverb::Parameters lowBandReverbParameters;


    //==============================================================================
    XPulseAudioProcessor();
    ~XPulseAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	// Create an instance of the Audio Processor Value Tree State(APVTS)
    juce::AudioProcessorValueTreeState parameters;


private:
	// This is a custom function to create the parameter layout for the APVTS
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	//Host Processor Instance
    HostProcessor hostProcessor_; 

	// ======== DSP processors ========
	//Gain processor
	juce::dsp::Gain<float> highGainProcessor;
    juce::dsp::Gain<float> midGainProcessor;
	juce::dsp::Gain<float> lowGainProcessor;

	//Band filters
    juce::dsp::IIR::Coefficients<float>::Ptr lowBandCoefficients;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowBand;

	juce::dsp::IIR::Coefficients<float>::Ptr midBandCoefficients;
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
    > midBand;

	juce::dsp::IIR::Coefficients<float>::Ptr highBandCoefficients;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highBand;
    //Custom Variables
	

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XPulseAudioProcessor)
};
