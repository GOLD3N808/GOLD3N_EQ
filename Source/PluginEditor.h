/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class GOLD3N_EQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    GOLD3N_EQAudioProcessorEditor (GOLD3N_EQAudioProcessor&);
    ~GOLD3N_EQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GOLD3N_EQAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GOLD3N_EQAudioProcessorEditor)
};
