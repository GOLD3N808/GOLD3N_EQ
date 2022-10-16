/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GOLD3N_EQAudioProcessorEditor::GOLD3N_EQAudioProcessorEditor(GOLD3N_EQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    lowCutFreqSliderAttachment(audioProcessor.TreeState, "LowCut Frequency", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.TreeState, "HighCut Frequency", highCutFreqSlider),
    lowBandFreqSliderAttachment(audioProcessor.TreeState, "Low Band Frequency", lowBandFreqSlider),
    lowBandGainSliderAttachment(audioProcessor.TreeState, "Low Band Gain", lowBandGainSlider),
    lowBandQSliderAttachment(audioProcessor.TreeState, "Low Band Q", lowBandQSlider),
    middleBandFreqSliderAttachment(audioProcessor.TreeState, "Middle Band Frequency", middleBandFreqSlider),
    middleBandGainSliderAttachment(audioProcessor.TreeState, "Middle Band Gain", middleBandGainSlider),
    middleBandQSliderAttachment(audioProcessor.TreeState, "Middle Band Q", middleBandQSlider),
    highBandFreqSliderAttachment(audioProcessor.TreeState, "High Band Frequency", highBandFreqSlider),
    highBandGainSliderAttachment(audioProcessor.TreeState, "High Band Gain", highBandGainSlider),
    highBandQSliderAttachment(audioProcessor.TreeState, "High Band Q", highBandQSlider),
    lowCutSlopeSliderAttachment(audioProcessor.TreeState, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.TreeState, "HighCut Slope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60); // dodany timer dla dzialania responsee curve

    setSize (900, 600);
}

GOLD3N_EQAudioProcessorEditor::~GOLD3N_EQAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

//==============================================================================
void GOLD3N_EQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
  //  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll(Colours::black);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.5);

    auto RC = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& lowband = monoChain.get<ChainPositions::LowBand>();
    auto& middleband = monoChain.get<ChainPositions::MiddleBand>();
    auto& highband = monoChain.get<ChainPositions::HighBand>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> magazines;

    magazines.resize(RC);

    for (int i = 0; i < RC; i++) // petla dla krzywej odpowiedzi
    {
        double magazine = 1.f;
        auto freq = mapToLog10(double(i) / double(RC), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::LowBand>())
            magazine *= lowband.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::MiddleBand>())
            magazine *= middleband.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::HighBand>())
            magazine *= highband.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            magazine *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            magazine *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            magazine *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            magazine *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<4>())
            magazine *= lowcut.get<4>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<5>())
            magazine *= lowcut.get<5>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            magazine *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            magazine *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            magazine *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            magazine *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<4>())
            magazine *= highcut.get<4>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<5>())
            magazine *= highcut.get<5>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        magazines[i] = Decibels::gainToDecibels(magazine);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(magazines.front()));

    for (size_t i = 1; i < magazines.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(magazines[i]));
    }

    g.setColour(Colours::brown);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::yellow);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void GOLD3N_EQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds(); // dodane slidery
    

    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.66));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.66));
    highCutSlopeSlider.setBounds(highCutArea);

    auto lowBandArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highBandArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    auto middleBandArea = bounds;

    lowBandFreqSlider.setBounds(lowBandArea.removeFromTop(lowBandArea.getHeight() * 0.33));
    lowBandGainSlider.setBounds(lowBandArea.removeFromTop(lowBandArea.getHeight() * 0.5));
    lowBandQSlider.setBounds(lowBandArea);

    highBandFreqSlider.setBounds(highBandArea.removeFromTop(highBandArea.getHeight() * 0.33));
    highBandGainSlider.setBounds(highBandArea.removeFromTop(highBandArea.getHeight() * 0.5));
    highBandQSlider.setBounds(highBandArea);

    middleBandFreqSlider.setBounds(middleBandArea.removeFromTop(middleBandArea.getHeight() * 0.33));
    middleBandGainSlider.setBounds(middleBandArea.removeFromTop(middleBandArea.getHeight() * 0.5));
    middleBandQSlider.setBounds(middleBandArea);

}

void GOLD3N_EQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}
void GOLD3N_EQAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //up mono
        auto lowChainSettings = getChainSettings(audioProcessor.TreeState); //dodane response curve dla lowband
        auto lowBandCoefficients = makeLowBandFilter(lowChainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::LowBand>().coefficients, lowBandCoefficients);

        auto middleChainSettings = getChainSettings(audioProcessor.TreeState); //dodane response curve dla lowband
        auto middleBandCoefficients = makeMiddleBandFilter(middleChainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::MiddleBand>().coefficients, middleBandCoefficients);

        auto highChainSettings = getChainSettings(audioProcessor.TreeState); //dodane response curve dla lowband
        auto highBandCoefficients = makeHighBandFilter(highChainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::HighBand>().coefficients, highBandCoefficients);

        auto lowCutChainSettings = getChainSettings(audioProcessor.TreeState);
        auto lowCutCoefficients = makeLowCutFilter(lowCutChainSettings, audioProcessor.getSampleRate());
        auto highCutChainSettings = getChainSettings(audioProcessor.TreeState);
        auto highCutCoefficients = makeHighCutFilter(highCutChainSettings, audioProcessor.getSampleRate());
        
        updateLowCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, lowCutChainSettings.lowCutSlope);
        updateHighCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, highCutChainSettings.highCutSlope);
        
        //signal repaint
        repaint();
    }
}
    std::vector<juce::Component*> GOLD3N_EQAudioProcessorEditor::getComps()
    {
        return
        {
            &lowBandFreqSlider,
            &lowBandGainSlider,
            &lowBandQSlider,
            &middleBandFreqSlider,
            &middleBandGainSlider,
            &middleBandQSlider,
            &highBandFreqSlider,
            &highBandGainSlider,
            &highBandQSlider,
            &lowCutFreqSlider,
            &highCutFreqSlider,
            &lowCutSlopeSlider,
            &highCutSlopeSlider
        };
    }

