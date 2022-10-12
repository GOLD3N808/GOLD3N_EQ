/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48,
    Slope_60,
    Slope_72
};

struct ChainSettings // polaczenie sygnalu z parametrami 
{
    float lowCut { 0 }, highCut { 0 };
    float lowBandF { 0 }, lowBandG { 0 }, lowBandQ { 1.f };
    float middleBandF { 0 }, middleBandG { 0 }, middleBandQ { 1.f };
    float highBandF { 0 }, highBandG { 0 }, highBandQ { 1.f };
    //int lowCutSlope { 0 }, highCutSlope { 0 };
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };

};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& TreeState);

//==============================================================================
/**
*/
class GOLD3N_EQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    GOLD3N_EQAudioProcessor();
    ~GOLD3N_EQAudioProcessor() override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); // metoda tworzenia parametrow
    juce::AudioProcessorValueTreeState TreeState { *this, nullptr, "Parameters", createParameterLayout() }; //stworzenie drzewa stanow, przypisanie parametrow

private:

    using Filter = juce::dsp::IIR::Filter<float>;

    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter, Filter, Filter>; //zmiana ilosci filtrow wzgledem ilosci slopow (6)

    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>; // sygnal mono, zdefiniowany lancuch parametrow

    MonoChain leftChain, rightChain; // rozbicie na stereo

    enum ChainPositions
    {
        LowCut,
        LowBand,
        MiddleBand,
        HighBand,
        HighCut
        
    };

    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GOLD3N_EQAudioProcessor)
};
