/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GOLD3N_EQAudioProcessor::GOLD3N_EQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

GOLD3N_EQAudioProcessor::~GOLD3N_EQAudioProcessor()
{
}

//==============================================================================
const juce::String GOLD3N_EQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GOLD3N_EQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GOLD3N_EQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GOLD3N_EQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GOLD3N_EQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GOLD3N_EQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GOLD3N_EQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GOLD3N_EQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GOLD3N_EQAudioProcessor::getProgramName (int index)
{
    return {};
}

void GOLD3N_EQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GOLD3N_EQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec prepareSpec;  // zbudowanie play-back dla obu kanalow

    prepareSpec.maximumBlockSize = samplesPerBlock;

    prepareSpec.numChannels = 1;

    prepareSpec.sampleRate = sampleRate;

    leftChain.prepare(prepareSpec);
    rightChain.prepare(prepareSpec);

    auto lowChainSettings = getChainSettings(TreeState);
    auto lowBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, lowChainSettings.lowBandF, lowChainSettings.lowBandQ, juce::Decibels::decibelsToGain(lowChainSettings.lowBandG)); // dodanie wspolczynnikow
    auto middleChainSettings = getChainSettings(TreeState);
    auto middleBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, middleChainSettings.middleBandF, middleChainSettings.middleBandQ, juce::Decibels::decibelsToGain(middleChainSettings.middleBandG));
    auto highChainSettings = getChainSettings(TreeState);
    auto highBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, highChainSettings.highBandF, highChainSettings.highBandQ, juce::Decibels::decibelsToGain(highChainSettings.highBandG));

    // tu zrobimy jeszcze miejsce

    *leftChain.get<ChainPositions::LowBand>().coefficients = *lowBandCoefficients; // enum ChainPositions PP.h
    *rightChain.get<ChainPositions::LowBand>().coefficients = *lowBandCoefficients;

    *leftChain.get<ChainPositions::MiddleBand>().coefficients = *middleBandCoefficients;
    *rightChain.get<ChainPositions::MiddleBand>().coefficients = *middleBandCoefficients;

    *leftChain.get<ChainPositions::HighBand>().coefficients = *highBandCoefficients;
    *rightChain.get<ChainPositions::HighBand>().coefficients = *highBandCoefficients;

    auto lowCutChainSettings = getChainSettings(TreeState);

    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(lowCutChainSettings.lowCut, sampleRate, 2*(lowCutChainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    leftLowCut.setBypassed<4>(true);
    leftLowCut.setBypassed<5>(true);

    switch (lowCutChainSettings.lowCutSlope)  //switch okreslajacy ktory slope ma zostac uruchomiony
    {
    case LowSlope_12:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case LowSlope_24:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        break;
    }
    case LowSlope_36:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        break;
    }
    case LowSlope_48:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        break;
    }
    case LowSlope_60:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        *leftLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        leftLowCut.setBypassed<4>(false);
            break;
    }
    case LowSlope_72:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        *leftLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        leftLowCut.setBypassed<4>(false);
        *leftLowCut.get<5>().coefficients = *lowCutCoefficients[5];
        leftLowCut.setBypassed<5>(false);
            break;
    }
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>(); //ten sam switch co powyzej tylko dla prawego kanalu

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    rightLowCut.setBypassed<4>(true);
    rightLowCut.setBypassed<5>(true);

    switch (lowCutChainSettings.lowCutSlope)
    {
    case LowSlope_12:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case LowSlope_24:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        break;
    }
    case LowSlope_36:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        break;
    }
    case LowSlope_48:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        break;
    }
    case LowSlope_60:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        *rightLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        rightLowCut.setBypassed<4>(false);
        break;
    }
    case LowSlope_72:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        *rightLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        rightLowCut.setBypassed<4>(false);
        *rightLowCut.get<5>().coefficients = *lowCutCoefficients[5];
        rightLowCut.setBypassed<5>(false);
        break;
    }
    }
    //auto highCutCoefficients1 = juce::dsp::FilterDesign<float>::des

    auto highCutChainSettings = getChainSettings(TreeState);

    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(highCutChainSettings.highCut, sampleRate, 2 * (highCutChainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();

    leftHighCut.setBypassed<0>(true);
    leftHighCut.setBypassed<1>(true);
    leftHighCut.setBypassed<2>(true);
    leftHighCut.setBypassed<3>(true);
    leftHighCut.setBypassed<4>(true);
    leftHighCut.setBypassed<5>(true);

    switch (highCutChainSettings.highCutSlope)  //switch okreslajacy ktory slope ma zostac uruchomiony
    {
    case HighSlope_12:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        break;
    }
    case HighSlope_24:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        break;
    }
    case HighSlope_36:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        break;
    }
    case HighSlope_48:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        break;
    }
    case HighSlope_60:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        *leftHighCut.get<4>().coefficients = *highCutCoefficients[4];
        leftHighCut.setBypassed<4>(false);
        break;
    }
    case HighSlope_72:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        *leftHighCut.get<4>().coefficients = *highCutCoefficients[4];
        leftHighCut.setBypassed<4>(false);
        *leftHighCut.get<5>().coefficients = *highCutCoefficients[5];
        leftHighCut.setBypassed<5>(false);
        break;
    }
    }

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>(); //ten sam switch co powyzej tylko dla prawego kanalu

    rightHighCut.setBypassed<0>(true);
    rightHighCut.setBypassed<1>(true);
    rightHighCut.setBypassed<2>(true);
    rightHighCut.setBypassed<3>(true);
    rightHighCut.setBypassed<4>(true);
    rightHighCut.setBypassed<5>(true);

    switch (highCutChainSettings.highCutSlope)
    {
    case HighSlope_12:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        break;
    }
    case HighSlope_24:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        break;
    }
    case HighSlope_36:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        break;
    }
    case HighSlope_48:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        break;
    }
    case HighSlope_60:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        *rightHighCut.get<4>().coefficients = *highCutCoefficients[4];
        rightHighCut.setBypassed<4>(false);
        break;
    }
    case HighSlope_72:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        *rightHighCut.get<4>().coefficients = *highCutCoefficients[4];
        rightHighCut.setBypassed<4>(false);
        *rightHighCut.get<5>().coefficients = *highCutCoefficients[5];
        rightHighCut.setBypassed<5>(false);
        break;
    }
    }


}

void GOLD3N_EQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GOLD3N_EQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void GOLD3N_EQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto lowChainSettings = getChainSettings(TreeState);
    auto lowBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(), lowChainSettings.lowBandF, lowChainSettings.lowBandQ, juce::Decibels::decibelsToGain(lowChainSettings.lowBandG)); // dodanie wspolczynnikow
    auto middleChainSettings = getChainSettings(TreeState);
    auto middleBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
       getSampleRate(), middleChainSettings.middleBandF, middleChainSettings.middleBandQ, juce::Decibels::decibelsToGain(middleChainSettings.middleBandG));
    auto highChainSettings = getChainSettings(TreeState);
    auto highBandCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(), highChainSettings.highBandF, highChainSettings.highBandQ, juce::Decibels::decibelsToGain(highChainSettings.highBandG));

    // tu zrobimy jeszcze miejsce

    *leftChain.get<ChainPositions::LowBand>().coefficients = *lowBandCoefficients;
    *rightChain.get<ChainPositions::LowBand>().coefficients = *lowBandCoefficients;

    *leftChain.get<ChainPositions::MiddleBand>().coefficients = *middleBandCoefficients;
    *rightChain.get<ChainPositions::MiddleBand>().coefficients = *middleBandCoefficients;

    *leftChain.get<ChainPositions::HighBand>().coefficients = *highBandCoefficients;
    *rightChain.get<ChainPositions::HighBand>().coefficients = *highBandCoefficients;

    auto lowCutChainSettings = getChainSettings(TreeState); // zdefiniowanie ustawien lancucha dla lowcut'a

    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(lowCutChainSettings.lowCut, getSampleRate(), 2 * (lowCutChainSettings.lowCutSlope + 1)); //wspolczynniki dla lowcut

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);  //lowcut dla 6 slopow
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    leftLowCut.setBypassed<4>(true);
    leftLowCut.setBypassed<5>(true);

    switch (lowCutChainSettings.lowCutSlope) //switch okreslajacy ktory slope ma zostac uruchomiony
    {
    case LowSlope_12:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case LowSlope_24:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        break;
    }
    case LowSlope_36:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        break;
    }
    case LowSlope_48:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        break;
    }
    case LowSlope_60:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        *leftLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        leftLowCut.setBypassed<4>(false);
        break;
    }
    case LowSlope_72:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        *leftLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        leftLowCut.setBypassed<4>(false);
        *leftLowCut.get<5>().coefficients = *lowCutCoefficients[5];
        leftLowCut.setBypassed<5>(false);
        break;
    }
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>(); 

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    rightLowCut.setBypassed<4>(true);
    rightLowCut.setBypassed<5>(true);

    switch (lowCutChainSettings.lowCutSlope) //ten sam switch co powyzej tylko dla prawego kanalu
    {
    case LowSlope_12:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case LowSlope_24:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        break;
    }
    case LowSlope_36:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        break;
    }
    case LowSlope_48:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        break;
    }
    case LowSlope_60:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        *rightLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        rightLowCut.setBypassed<4>(false);
        break;
    }
    case LowSlope_72:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        *rightLowCut.get<4>().coefficients = *lowCutCoefficients[4];
        rightLowCut.setBypassed<4>(false);
        *rightLowCut.get<5>().coefficients = *lowCutCoefficients[5];
        rightLowCut.setBypassed<5>(false);
        break;
    }
    }

    auto highCutChainSettings = getChainSettings(TreeState);

    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(highCutChainSettings.highCut, getSampleRate(), 2 * (highCutChainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();

    leftHighCut.setBypassed<0>(true);
    leftHighCut.setBypassed<1>(true);
    leftHighCut.setBypassed<2>(true);
    leftHighCut.setBypassed<3>(true);
    leftHighCut.setBypassed<4>(true);
    leftHighCut.setBypassed<5>(true);

    switch (highCutChainSettings.highCutSlope)  //switch okreslajacy ktory slope ma zostac uruchomiony
    {
    case HighSlope_12:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        break;
    }
    case HighSlope_24:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        break;
    }
    case HighSlope_36:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        break;
    }
    case HighSlope_48:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        break;
    }
    case HighSlope_60:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        *leftHighCut.get<4>().coefficients = *highCutCoefficients[4];
        leftHighCut.setBypassed<4>(false);
        break;
    }
    case HighSlope_72:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        *leftHighCut.get<4>().coefficients = *highCutCoefficients[4];
        leftHighCut.setBypassed<4>(false);
        *leftHighCut.get<5>().coefficients = *highCutCoefficients[5];
        leftHighCut.setBypassed<5>(false);
        break;
    }
    }

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>(); //ten sam switch co powyzej tylko dla prawego kanalu

    rightHighCut.setBypassed<0>(true);
    rightHighCut.setBypassed<1>(true);
    rightHighCut.setBypassed<2>(true);
    rightHighCut.setBypassed<3>(true);
    rightHighCut.setBypassed<4>(true);
    rightHighCut.setBypassed<5>(true);

    switch (highCutChainSettings.highCutSlope)
    {
    case HighSlope_12:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        break;
    }
    case HighSlope_24:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        break;
    }
    case HighSlope_36:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        break;
    }
    case HighSlope_48:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        break;
    }
    case HighSlope_60:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        *rightHighCut.get<4>().coefficients = *highCutCoefficients[4];
        rightHighCut.setBypassed<4>(false);
        break;
    }
    case HighSlope_72:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        *rightHighCut.get<4>().coefficients = *highCutCoefficients[4];
        rightHighCut.setBypassed<4>(false);
        *rightHighCut.get<5>().coefficients = *highCutCoefficients[5];
        rightHighCut.setBypassed<5>(false);
        break;
    }
    }

    juce::dsp::AudioBlock<float> block(buffer); // stworzenie buffora dla obu kanalow

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool GOLD3N_EQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GOLD3N_EQAudioProcessor::createEditor()
{
    //return new GOLD3N_EQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void GOLD3N_EQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GOLD3N_EQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& TreeState) // zdefiniowanie metody getChainSettings
{
    ChainSettings paramSettings;

    paramSettings.lowCut = TreeState.getRawParameterValue("LowCut Frequency")->load();
    paramSettings.highCut = TreeState.getRawParameterValue("HighCut Frequency")->load();
    paramSettings.lowBandF = TreeState.getRawParameterValue("Low Band Frequency")->load();
    paramSettings.lowBandG = TreeState.getRawParameterValue("Low Band Gain")->load();
    paramSettings.lowBandQ = TreeState.getRawParameterValue("Low Band Q")->load();
    paramSettings.middleBandF = TreeState.getRawParameterValue("Middle Band Frequency")->load();
    paramSettings.middleBandG = TreeState.getRawParameterValue("Middle Band Gain")->load();
    paramSettings.middleBandQ = TreeState.getRawParameterValue("Middle Band Q")->load();
    paramSettings.highBandF = TreeState.getRawParameterValue("High Band Frequency")->load();
    paramSettings.highBandG = TreeState.getRawParameterValue("High Band Gain")->load();
    paramSettings.highBandQ = TreeState.getRawParameterValue("High Band Q")->load();
    paramSettings.lowCutSlope = static_cast<LowSlope>(TreeState.getRawParameterValue("LowCut Slope")->load());
    paramSettings.highCutSlope = static_cast<HighSlope>(TreeState.getRawParameterValue("HighCut Slope")->load());


    return paramSettings;
}

//void GOLD3N_EQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)


juce::AudioProcessorValueTreeState::ParameterLayout
GOLD3N_EQAudioProcessor::createParameterLayout() // definicja metody tworzenia parametrow
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "LowCut Frequency", "LowCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f)); // ID parametru, Nazwa, wartosc min, wartosc max, skok, skewFactor, wartosc default

    layout.add(std::make_unique<juce::AudioParameterFloat>( // filtr dolno przepustowy
        "HighCut Frequency", "HighCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>( // band w dole
        "Low Band Frequency", "Low Band Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 100.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Low Band Gain", "Low Band Gain", juce::NormalisableRange<float>(- 30.f, 30.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Low Band Q", "Low Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 1.f), 1.f));


   layout.add(std::make_unique<juce::AudioParameterFloat>( // band w srednicy
        "Middle Band Frequency", "Middle Band Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Middle Band Gain", "Middle Band Gain", juce::NormalisableRange<float>(- 30.f, 30.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Middle Band Q", "Middle Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>( // band w gorze
        "High Band Frequency", "High Band Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 5000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "High Band Gain", "High Band Gain", juce::NormalisableRange<float>(- 30.f, 30.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "High Band Q", "High Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 1.f), 1.f));

    juce::StringArray slopeArray; //lista rozwijana z mozliwoscia wyboru nachylenia zbocza filtrow (db/Oct)
    for (int i = 0; i < 6; i++)
    {
        juce::String str;
        str << (12 + i*12);
        str << "db/Oct";
        slopeArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", slopeArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", slopeArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GOLD3N_EQAudioProcessor();
}
