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

    setSize (900, 600);
}

GOLD3N_EQAudioProcessorEditor::~GOLD3N_EQAudioProcessorEditor()
{
}

//==============================================================================
void GOLD3N_EQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
  //  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);
  //  g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
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

