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
	void processAudio(juce::AudioBuffer<float>& buffer);
	void processMidi(juce::MidiBuffer& midiMessages);
    
    //Custom Variables
    //MIDI Band Velocities
	int lowBandVelocity = 0;
	int midBandVelocity = 0;
	int highBandVelocity = 0;
    

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

	// Hosted Plugin Send Functions
    void setBandPluginInstanceId(int band, int slot, uint32_t id)
    {
        if ((unsigned)band < kNumBands && (unsigned)slot < kNumSlots)
            bandPluginInstanceId[band][slot].store(id, std::memory_order_relaxed);
    }

    void setBandSendAmount(int band, int slot, float v)
    {
        if ((unsigned)band < kNumBands && (unsigned)slot < kNumSlots)
            bandSendAmount[band][slot].store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }

    void setBandReturnAmount(int band, int slot, float v)
    {
        if ((unsigned)band < kNumBands && (unsigned)slot < kNumSlots)
            bandReturnAmount[band][slot].store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }
    
	// Band Splitter Functions
    void setBandSplits(float lowMidSplit, float midHighSplit);

private:
	// Default sample rate (will be updated in prepareToPlay)
    double currentSampleRate = 44100.0;

	// Function to update band filter coefficients based on current parameter values
    void updateBandFilterCutoffs();

	// Constants for band processing
	static constexpr int kNumBands = 3; // Low, Mid, High
	static constexpr int kNumSlots = 3;

    // Hosted plugin send routing
    std::atomic<uint32_t> bandPluginInstanceId[kNumBands][kNumSlots];
    std::atomic<float>    bandSendAmount[kNumBands][kNumSlots]; 
    std::atomic<float>    bandReturnAmount[kNumBands][kNumSlots];

    
    // buffers reused per block (no allocations in processBlock)
    juce::AudioBuffer<float> lowBuffer, midBuffer, highBuffer;
    juce::AudioBuffer<float> auxBuffer;

    void processHostedSends(juce::AudioBuffer<float>& low,
        juce::AudioBuffer<float>& mid,
        juce::AudioBuffer<float>& high);


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
