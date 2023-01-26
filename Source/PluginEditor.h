/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] /= (float) numBins;
        }
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }

    int getFFTSize() const { return 1 << order; }

    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }

    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom), top);
        };

        auto y = map(renderData[0]);

                jassert( !std::isnan(y) && !std::isinf(y) );

                p.startNewSubPath(0, y);

        const int pathResolution = 2;

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

                        jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct MySlidersLabels : juce::Slider

{
    MySlidersLabels() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {

    }

};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<GOLD3N_EQAudioProcessor::BlockType>& scsf) :
        leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath;  }
private:
    SingleChannelSampleFifo<GOLD3N_EQAudioProcessor::BlockType>* leftChannelFifo;

    juce::AudioBuffer<float> monoBuffer;

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathProducer;

    juce::Path leftChannelFFTPath;
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

    void updateChain();

    juce::Image background;

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;
    
};

//==============================================================================

class GOLD3N_EQAudioProcessorEditor : public juce::AudioProcessorEditor
     
{
public:
    GOLD3N_EQAudioProcessorEditor (GOLD3N_EQAudioProcessor&);
    ~GOLD3N_EQAudioProcessorEditor() override;

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
