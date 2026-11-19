#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kKnee      = 0.55f;   // linear below ~ -5.2 dBFS
    constexpr float kCeiling   = 0.989f;  // ~ -0.1 dBFS
    constexpr float kMaxDrive  = 24.0f;   // dB at 100 %
    constexpr float kLoudBoost = 9.0f;    // dB at 100 % in LOUD mode
    constexpr float kSilence   = 1.0e-8f; // mean-square gate (~ -80 dB)
}

float SoftCapperProcessor::softClip (float x) noexcept
{
    const float a = std::abs (x);
    if (a <= kKnee)
        return x * kCeiling;
    const float y = kKnee + (1.0f - kKnee) * std::tanh ((a - kKnee) / (1.0f - kKnee));
    return std::copysign (y * kCeiling, x);
}

juce::AudioProcessorValueTreeState::ParameterLayout SoftCapperProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cap", 1 }, "Cap",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "loud", 1 }, "Loud", false));
    return layout;
}

SoftCapperProcessor::SoftCapperProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SOFTCAPPER", createLayout())
{
    capParam  = apvts.getRawParameterValue ("cap");
    loudParam = apvts.getRawParameterValue ("loud");
}

bool SoftCapperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void SoftCapperProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) juce::jmax (1, getTotalNumOutputChannels()), 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    oversampling->reset();
    setLatencySamples ((int) std::round (oversampling->getLatencyInSamples()));
    inMs.assign ((size_t) samplesPerBlock, 0.0f);

    driveSmoothed.reset (sampleRate * 4.0, 0.05);   // per oversampled sample
    driveSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (capParam->load() * 0.01f * kMaxDrive));

    envCoef  = 1.0f - std::exp (-1.0f / (0.30f * (float) sampleRate)); // ~300 ms RMS window
    gainCoef = 1.0f - std::exp (-1.0f / (0.08f * (float) sampleRate)); // gain glide
    envIn = envOut = 0.0f;
    autoGain = 1.0f;
}

void SoftCapperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = getTotalNumOutputChannels();
    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    if (n == 0 || numCh == 0 || oversampling == nullptr)
        return;
    if ((size_t) n > inMs.size())          // host sent a bigger block than announced
    {
        inMs.resize ((size_t) n);
        oversampling->initProcessing ((size_t) n);
    }

    const float t    = juce::jlimit (0.0f, 1.0f, capParam->load() * 0.01f);
    const bool  loud = loudParam->load() > 0.5f;
    driveSmoothed.setTargetValue (juce::Decibels::decibelsToGain (t * kMaxDrive));

    // 1) input loudness (before anything), per base-rate sample
    std::fill (inMs.begin(), inMs.begin() + n, 0.0f);
    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* d = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            inMs[(size_t) i] += d[i] * d[i];
    }

    // 2) drive + clip at 4x
    juce::dsp::AudioBlock<float> block (buffer);
    auto up = oversampling->processSamplesUp (block);
    const size_t upN = up.getNumSamples();
    for (size_t i = 0; i < upN; ++i)
    {
        const float g = driveSmoothed.getNextValue();
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            float* d = up.getChannelPointer (ch);
            d[i] = softClip (d[i] * g);
        }
    }
    oversampling->processSamplesDown (block);

    // 3) automatic gain: match RMS of the clipped signal to the input (+ LOUD boost)
    const float boost = loud ? juce::Decibels::decibelsToGain (t * kLoudBoost) : 1.0f;
    const float inv = 1.0f / (float) numCh;
    for (int i = 0; i < n; ++i)
    {
        float outMs = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float v = buffer.getSample (ch, i);
            outMs += v * v;
        }
        envIn  += envCoef * (inMs[(size_t) i] * inv - envIn);
        envOut += envCoef * (outMs * inv - envOut);

        if (envIn > kSilence && envOut > kSilence)
        {
            float target = std::sqrt (envIn / envOut) * boost;
            target = loud ? juce::jlimit (0.01f, 1.0f, target)   // LOUD: never above the clipper ceiling
                          : juce::jlimit (0.01f, 2.0f, target);
            autoGain += gainCoef * (target - autoGain);
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            d[i] = juce::jlimit (-1.0f, 1.0f, d[i] * autoGain);  // final safety
        }
    }
}

juce::AudioProcessorEditor* SoftCapperProcessor::createEditor()
{
    return new SoftCapperEditor (*this);
}

void SoftCapperProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SoftCapperProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoftCapperProcessor();
}
