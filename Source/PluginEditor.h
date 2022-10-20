/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//struct LookAndFeel : juce::LookAndFeel_V4
//{
  //  void drawRotarySlider(juce::Graphics&,
  //      int x, int y, int width, int height,
  //      float sliderPosProportional,
  //      float rotaryStartAngle,
  //      float rotaryEndAngle,
  //      juce::Slider&) override;
//};

struct MySlidersLabels : juce::Slider

{
    MySlidersLabels() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {
        //setTextBoxStyle(Slider::TextBoxBelow, true, 100, 25);
       // setTextBoxStyle()
    }
 //   MySlidersLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
  //      param(&rap),
   //     suffix(unitSuffix)
 //   {
 //       setLookAndFeel(&lnf);
 //   }
 //   ~MySlidersLabels()
 //   {
 //       setLookAndFeel(nullptr);
 //   }

 //   void paint(juce::Graphics& g) override;
  //  juce::Rectangle<int> getSliderBounds() const;
 //   int getTextHeight() const { return 14; }
 //   juce::String getDisplayString() const;

//private:
  //  LookAndFeel lnf;
  //  juce::RangedAudioParameter* param;
 //   juce::String suffix;
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
    void resized() override;

private:
    GOLD3N_EQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged { false };

    MonoChain monoChain;

    juce::Image background;

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();
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
    void paint (juce::Graphics& gg) override;
    void resized() override;

    

    

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GOLD3N_EQAudioProcessor& audioProcessor;

    

    MySlidersLabels lowBandFreqSlider,
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
