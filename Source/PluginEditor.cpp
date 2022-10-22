/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(GOLD3N_EQAudioProcessor& p) : audioProcessor(p),
// leftChannelFifo(&audioProcessor.leftChannelFifo)
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

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

    startTimerHz(60); // dodany timer dla dzialania responsee curve
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}
void ResponseCurveComponent::timerCallback()
{

     const auto fftBounds = getAnalysisArea().toFloat();
     auto sampleRate = audioProcessor.getSampleRate();

     leftPathProducer.process(fftBounds, sampleRate);
     rightPathProducer.process(fftBounds, sampleRate);

    if (parametersChanged.compareAndSetBool(false, true))
    {

        DBG("params changed");
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
       // repaint();
    }

    repaint();
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    auto bounds = getAnalysisArea();
    auto responseArea = (bounds);

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

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(Colours::green); // spektrum dla lewego kanalu
    g.strokePath(leftChannelFFTPath, PathStrokeType(2.f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(Colours::blue); // spektrum dla prawego kanalu
    g.strokePath(rightChannelFFTPath, PathStrokeType(2.f));

    g.setColour(Colours::black);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::violet);
    g.strokePath(responseCurve, PathStrokeType(4.f));

}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true); // metoda resized dla narysowania siatki

        Graphics g(background);

        Array<float> freqs
        {
                20, 30, 40, 50, 100,
                200, 300, 500, 1000,
                2000, 3000, 4000, 5000, 10000, 20000
        };

        auto renderArea = getAnalysisArea();
        auto left = renderArea.getX();
        auto right = renderArea.getRight();
        auto top = renderArea.getY();
        auto bottom = renderArea.getBottom();
        auto width = renderArea.getWidth();

        Array<float> xs;
        for (auto f : freqs)
        {
            auto normX = mapFromLog10(f, 20.f, 20000.f);
            xs.add(left + width * normX);
        }

        g.setColour(Colours::dimgrey);
        for (auto x : xs)
        {
            g.drawVerticalLine(x, top, bottom);
        }
        /*for (auto f : freqs)
        {
            auto normX = mapFromLog10(f, 20.f, 20000.f);

            g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
        }*/

        Array<float> gain
        {
            -24, -12, 0, 12, 24
        };

        for (auto gDb : gain)
        {
            auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

            //g.drawHorizontalLine(y, 0, getWidth());
            g.setColour(gDb == 0.f ? Colour(250u, 249u, 247u)  : Colours::darkgrey );
            g.drawHorizontalLine(y, left, right);
        }

   //     g.drawRect(getAnalysisArea());

        g.setColour(Colours:: lightgrey);
        const int fontHeight = 12;
        g.setFont(fontHeight);

        for (int i = 0; i < freqs.size(); i++)
        {
            auto f = freqs[i];
            auto x = xs[i];

            bool addK = false;
            String str;
            if (f > 999.f)
            {
                addK = true;
                f /= 1000.f;
            }

            str << f;
            if (addK)
                str << "k";
            str << "Hz";

            auto textWidth = g.getCurrentFont().getStringWidth(str);

            Rectangle<int> r;

            r.setSize(textWidth, fontHeight);
            r.setCentre(x, 0);
            r.setY(getHeight() - fontHeight);

            g.drawFittedText(str, r, juce::Justification::bottom, 1);

            r.setSize(textWidth, fontHeight);
            r.setCentre(x, 0);
            r.setY(1);
            g.drawFittedText(str, r, juce::Justification::bottom, 1);
        }

        for (auto gDb : gain)
        {
            auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

            String str;
            if (gDb > 0)
                str << "+";
            str << gDb;

            auto textWidth = g.getCurrentFont().getStringWidth(str);

            Rectangle<int> r;
            r.setSize(textWidth, fontHeight);
            r.setX(getWidth() - textWidth);
            r.setCentre(r.getCentreX(), y);

            g.setColour(gDb == 0.f ? Colour(250u, 249u, 247u) : Colours::darkgrey);
            g.drawFittedText(str, r, juce::Justification::centred, 1);

            str.clear();
            if (gDb > 0)
                str << "+";
            str << gDb;

            r.setX(0);
            textWidth = g.getCurrentFont().getStringWidth(str);
            r.setSize(textWidth, fontHeight);
            g.setColour(Colours::lightgrey);
            g.setColour(gDb == 0.f ? Colour(250u, 249u, 247u) : Colours::darkgrey);
            g.drawFittedText(str, r, juce::Justification::centred, 1);
        }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
  //  bounds.reduce(20, 16);
    bounds.removeFromTop(12);
     bounds.removeFromBottom(12);
     bounds.removeFromLeft(20);
     bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(8);
    bounds.removeFromBottom(8);

    return bounds;
}

//==============================================================================
GOLD3N_EQAudioProcessorEditor::GOLD3N_EQAudioProcessorEditor(GOLD3N_EQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),

    responseCurveComponent(audioProcessor),
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
    using namespace juce;

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    lowCutFreqSlider.setTextValueSuffix("Hz");
    highCutFreqSlider.setTextValueSuffix("Hz");

    lowBandFreqSlider.setTextValueSuffix("Hz");
    lowBandGainSlider.setTextValueSuffix("dB");
    lowBandQSlider.setTextValueSuffix("");

    middleBandFreqSlider.setTextValueSuffix("Hz");
    middleBandGainSlider.setTextValueSuffix("dB");
    middleBandQSlider.setTextValueSuffix("");

    highBandFreqSlider.setTextValueSuffix("Hz");
    highBandGainSlider.setTextValueSuffix("dB");
    highBandQSlider.setTextValueSuffix("");


    getLookAndFeel().setColour(Slider::thumbColourId, Colours::white);
    getLookAndFeel().setColour(Slider::rotarySliderOutlineColourId, Colours::darkmagenta);
    getLookAndFeel().setColour(Slider::rotarySliderFillColourId, Colours::violet);

    setSize (900, 600);
}

GOLD3N_EQAudioProcessorEditor::~GOLD3N_EQAudioProcessorEditor()
{

}

//==============================================================================
void GOLD3N_EQAudioProcessorEditor::paint (juce::Graphics& gg)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
  gg.setGradientFill(ColourGradient{ Colours::black.brighter(0.2f), getLocalBounds().toFloat().getCentre(), Colours::black.darker(0.8f), {}, true });
  gg.fillRect(getLocalBounds());

}

void GOLD3N_EQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds(); // dodane slidery
    

    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.5);

    responseCurveComponent.setBounds(responseArea);

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
            &highCutSlopeSlider,
            &responseCurveComponent
        };
    }

