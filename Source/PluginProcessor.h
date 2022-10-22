/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0 );
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0 )
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0 )
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
    Right,
    Left
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }
    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); i++)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }
    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1, bufferSize, false, true, true);
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }

    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepaerd() const { return prepared.get(); }
    int getSize() const { return size.get(); }

    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }

private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        fifoIndex++;
    }
};

enum LowSlope
{
    LowSlope_12,
    LowSlope_24,
    LowSlope_36,
    LowSlope_48,
    LowSlope_60,
    LowSlope_72
};

enum HighSlope
{
    HighSlope_12,
    HighSlope_24,
    HighSlope_36,
    HighSlope_48,
    HighSlope_60,
    HighSlope_72
};

struct ChainSettings // polaczenie sygnalu z parametrami 
{
    float lowCut { 0 }, highCut { 0 };
    float lowBandF { 0 }, lowBandG { 0 }, lowBandQ { 1.f };
    float middleBandF { 0 }, middleBandG { 0 }, middleBandQ { 1.f };
    float highBandF { 0 }, highBandG { 0 }, highBandQ { 1.f };
    LowSlope lowCutSlope{ LowSlope::LowSlope_12 }; 
    HighSlope highCutSlope{ HighSlope::HighSlope_12 }; 

};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& TreeState);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter, Filter, Filter>; //zmiana ilosci filtrow wzgledem ilosci slopow (6)

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>; // sygnal mono, zdefiniowany lancuch parametrow

enum ChainPositions
{
    LowCut,
    LowBand,
    MiddleBand,
    HighBand,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr; //dodana metoda updateCoefficients dla response curve

void updateCoefficients(Coefficients &old, const Coefficients &replacements);

Coefficients makeLowBandFilter(const ChainSettings& lowChainSettings, double sampleRate);
Coefficients makeMiddleBandFilter(const ChainSettings& middleChainSettings, double sampleRate);
Coefficients makeHighBandFilter(const ChainSettings& highChainSettings, double sampleRate);

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateLowCutFilter(ChainType& chain,
    const CoefficientType& lowCutCoefficients,
    //  const ChainSettings& lowCutChainSettings)
    const LowSlope& lowCutSlope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);
    chain.template setBypassed<4>(true);
    chain.template setBypassed<5>(true);

     //switch okreslajacy ktory slope ma zostac uruchomiony
    switch (lowCutSlope)
    {
    case LowSlope_72:
    {
        update<5>(chain, lowCutCoefficients);
    }
    case LowSlope_60:
    {
        update<4>(chain, lowCutCoefficients);
    }
    case LowSlope_48:
    {
        update<3>(chain, lowCutCoefficients);
    }
    case LowSlope_36:
    {
        update<2>(chain, lowCutCoefficients);
    }
    case LowSlope_24:
    {
        update<1>(chain, lowCutCoefficients);
    }
    case LowSlope_12:
    {
        update<0>(chain, lowCutCoefficients);
    }
    }

}

template<typename ChainType, typename CoefficientType>
void updateHighCutFilter(ChainType& chain,
    const CoefficientType& highCutCoefficients,
    const HighSlope& highCutSlope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);
    chain.template setBypassed<4>(true);
    chain.template setBypassed<5>(true);

  //switch okreslajacy ktory slope ma zostac uruchomiony
    switch (highCutSlope)
    {
    case HighSlope_72:
    {
        update<5>(chain, highCutCoefficients);
    }
    case HighSlope_60:
    {
        update<4>(chain, highCutCoefficients);
    }
    case HighSlope_48:
    {
        update<3>(chain, highCutCoefficients);
    }
    case HighSlope_36:
    {
        update<2>(chain, highCutCoefficients);
    }
    case HighSlope_24:
    {
        update<1>(chain, highCutCoefficients);
    }
    case HighSlope_12:
    {
        update<0>(chain, highCutCoefficients);
    }

    }
}

inline auto makeLowCutFilter(const ChainSettings& lowCutChainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(lowCutChainSettings.lowCut, sampleRate, 2 * (lowCutChainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& highCutChainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(highCutChainSettings.highCut, sampleRate, 2 * (highCutChainSettings.highCutSlope + 1));
}

//==============================================================================
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

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right };
    
private:
    MonoChain leftChain, rightChain; // rozbicie na stereo

    void updateLowBandFilter(const ChainSettings& lowChainSettings);
    void updateMiddleBandFilter(const ChainSettings& middleChainSettings);
    void updateHighBandFilter(const ChainSettings& highChainSettings);

    void updateLowCutFilters(const ChainSettings& lowCutChainSettings);
    void updateHighCutFilters(const ChainSettings& highCutChainSettings);

    void updateFilters();
    
    //juce::dsp::Oscillator<float> osc;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GOLD3N_EQAudioProcessor)
};
