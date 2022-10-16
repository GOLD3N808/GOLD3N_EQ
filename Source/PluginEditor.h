/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct MySliders : juce::Slider

{
    MySliders() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

struct ResponseCurveComponent: juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(GOLD3N_EQAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    GOLD3N_EQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged { false };

    MonoChain monoChain;
};


//==============================================================================
/**
*/
class GOLD3N_EQAudioProcessorEditor : public juce::AudioProcessorEditor


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



    MySliders lowBandFreqSlider, 
        lowBandGainSlider, 
        lowBandQSlider, 
        middleBandFreqSlider, 
        middleBandGainSlider, 
        middleBandQSlider, 
        highBandFreqSlider, 
        highBandGainSlider, 
        highBandQSlider, 
        lowCutFreqSlider, 
        highCutFreqSlider, 
        lowCutSlopeSlider, 
        highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using TreeStateConnect = juce::AudioProcessorValueTreeState; // connect parametrow ze sliderami
    using Attachment = TreeStateConnect::SliderAttachment;

    Attachment lowBandFreqSliderAttachment,
        lowBandGainSliderAttachment,
        lowBandQSliderAttachment,
        middleBandFreqSliderAttachment,
        middleBandGainSliderAttachment,
        middleBandQSliderAttachment,
        highBandFreqSliderAttachment,
        highBandGainSliderAttachment,
        highBandQSliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;



    std::vector<juce::Component*> getComps();



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GOLD3N_EQAudioProcessorEditor)
};
