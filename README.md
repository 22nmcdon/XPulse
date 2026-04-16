Overview
XPulse is a real-time adaptive audio effects plugin developed in C++ using the JUCE framework.
The Project explores dynamic, performance-responsive audio processing through a multi-band architecture that allows for external plugin hosting and routing adaptively accross separated frequency bands.
Desgined for live musicians in mind, XPulse extends the traditional static audio effects by enabling real-time interaction between incoming audio, MIDI performance data, and hosted third-party plugins.

Features:
Multi-band Frequency Splitting
- Splits incoming audio into Low, Mid, and High frequency bands
- adjustable crossover frequencies with interactie GUI controls
- Independent processing path for each band
Third-Party Plugin Hosting
- Supports loading external VST3 plugins inside each frequency band
- Hosted plgins are processed independently per-band
- Dynamic plugin instance management using internal plugin pooling
MIDI-Driven Adaptive Processing
- MIDI velocity affects per-band send values
- Sustain pedal toggles auxiliary performance effects routing
- Designed for expressive real-time performance modulation

Technical Architecture:

XPulse follows a modular DSP processing pipeline
Input Audio -> BandSplitting -> Per-Band Plugin Processing -> MIDI Modulation -> Band Summation -> Output Audio

Core components includeL
- JUCE DSP Module for filtering and audio processing
- AudioPluginFormatManager for hosted plugin discovery/loading
- KnownPluginList for plugin management
- PluginPool System for hosted instance lifecycle management
- APVTS (AudioProcessorValueTreeState) for parameter/state management

Technologies Used:
- Language: C++
- Framework: JUCE
- IDE: Visual Studio 2022
- Testing Environment: JUCE AudioPluginHost / REAPER
- Analysis Tools: Voxengo SPAN

Installation / Build Instructions:
Requirements
- JUCE Framework
- Projucer
- Visual Studio 2022
- VST3SDK
- ASIO Driver recommended for low-latency with Audio Interface

Build Steps
- Clone Repository
- Open .jucer project in Projucer
- Export project for Visual Studio 2022
- Open solution
- Build solution

Future Improvements:
- Per-band latency compensation for hosted plugins
- Preset management
- Additional crossover filter designs

Currently have an issue with the release outside of AudioPluginHost in that it doesn't allow for the altering of band crossovers.

Author:
Noah McDonald
B.A Computer Science & Music
The College of Wooster
