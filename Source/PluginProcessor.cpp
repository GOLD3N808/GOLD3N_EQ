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

    updateFilters();

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

  //  osc.initialise([](float x) {return std::sin(x); });
 //   prepareSpec.numChannels = getTotalNumOutputChannels();
 //   osc.prepare(prepareSpec);
  //  osc.setFrequency(1000);
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

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer); // stworzenie buffora dla obu kanalow

  //  buffer.clear();
  //  juce::dsp::ProcessContextReplacing<float> stereoContext(block);
  //  osc.process(stereoContext);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);

}

//==============================================================================
bool GOLD3N_EQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GOLD3N_EQAudioProcessor::createEditor()
{
    return new GOLD3N_EQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
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

Coefficients makeLowBandFilter(const ChainSettings& lowChainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, lowChainSettings.lowBandF, lowChainSettings.lowBandQ, juce::Decibels::decibelsToGain(lowChainSettings.lowBandG));
    
}

void GOLD3N_EQAudioProcessor::updateLowBandFilter(const ChainSettings &lowChainSettings)
{
    auto lowBandCoefficients = makeLowBandFilter(lowChainSettings, getSampleRate());


    updateCoefficients(leftChain.get<ChainPositions::LowBand>().coefficients, lowBandCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::LowBand>().coefficients, lowBandCoefficients);

}

Coefficients makeMiddleBandFilter(const ChainSettings& middleChainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, middleChainSettings.middleBandF, middleChainSettings.middleBandQ, juce::Decibels::decibelsToGain(middleChainSettings.middleBandG));
}

void GOLD3N_EQAudioProcessor::updateMiddleBandFilter(const ChainSettings &middleChainSettings)
{
    auto middleBandCoefficients = makeMiddleBandFilter(middleChainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::MiddleBand>().coefficients, middleBandCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::MiddleBand>().coefficients, middleBandCoefficients);

}

Coefficients makeHighBandFilter(const ChainSettings& highChainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, highChainSettings.highBandF, highChainSettings.highBandQ, juce::Decibels::decibelsToGain(highChainSettings.highBandG));
}

void GOLD3N_EQAudioProcessor::updateHighBandFilter(const ChainSettings &highChainSettings)
{
    auto highBandCoefficients = makeHighBandFilter(highChainSettings, getSampleRate());

       updateCoefficients(leftChain.get<ChainPositions::HighBand>().coefficients, highBandCoefficients);
       updateCoefficients(rightChain.get<ChainPositions::HighBand>().coefficients, highBandCoefficients);

}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) // definicja metody update coefficients dla response curve
{
    *old = *replacements;
}

void GOLD3N_EQAudioProcessor::updateLowCutFilters(const ChainSettings &lowCutChainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(lowCutChainSettings, getSampleRate());

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    updateLowCutFilter(leftLowCut, lowCutCoefficients, lowCutChainSettings.lowCutSlope);

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateLowCutFilter(rightLowCut, lowCutCoefficients, lowCutChainSettings.lowCutSlope);
}

void GOLD3N_EQAudioProcessor::updateHighCutFilters(const ChainSettings &highCutChainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(highCutChainSettings, getSampleRate());   

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();

    updateHighCutFilter(leftHighCut, highCutCoefficients, highCutChainSettings.highCutSlope);

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>(); //ten sam switch co powyzej tylko dla prawego kanalu

    updateHighCutFilter(rightHighCut, highCutCoefficients, highCutChainSettings.highCutSlope);
}

void GOLD3N_EQAudioProcessor::updateFilters()
{
    auto lowCutChainSettings = getChainSettings(TreeState);
    auto lowChainSettings = getChainSettings(TreeState);
    auto middleChainSettings = getChainSettings(TreeState);
    auto highChainSettings = getChainSettings(TreeState);
    auto highCutChainSettings = getChainSettings(TreeState);

    updateLowCutFilters(lowCutChainSettings);
    updateLowBandFilter(lowChainSettings);
    updateMiddleBandFilter(middleChainSettings);
    updateHighBandFilter(highChainSettings);
    updateHighCutFilters(highCutChainSettings);

}

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
        "Low Band Q", "Low Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 0.23f), 1.f));


   layout.add(std::make_unique<juce::AudioParameterFloat>( // band w srednicy
        "Middle Band Frequency", "Middle Band Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Middle Band Gain", "Middle Band Gain", juce::NormalisableRange<float>(- 30.f, 30.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Middle Band Q", "Middle Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 0.23f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>( // band w gorze
        "High Band Frequency", "High Band Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.23f), 5000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "High Band Gain", "High Band Gain", juce::NormalisableRange<float>(- 30.f, 30.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "High Band Q", "High Band Q", juce::NormalisableRange<float>(0.05f, 20.f, 0.01f, 0.23f), 1.f));

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
