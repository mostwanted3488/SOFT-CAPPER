#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// SOFT CAPPER — one knob soft clipper with automatic gain.
//  CAP  (0..100 %) : drive 0 → +24 dB into a smooth-knee clipper (4x oversampled)
//  LOUD (on/off)   : off = output loudness matched to the input (pure tone change)
//                    on  = up to +9 dB louder than the matched level, never above -0.1 dBFS
class SoftCapperProcessor final : public juce::AudioProcessor
{
public:
    SoftCapperProcessor();
    ~SoftCapperProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SOFT CAPPER"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // the clip curve, shared with tests
    static float softClip (float x) noexcept;

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;   // 4x, built in prepareToPlay
    std::vector<float> inMs;                                         // per-sample input power

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveSmoothed;
    std::atomic<float>* capParam  = nullptr;
    std::atomic<float>* loudParam = nullptr;

    double sr = 44100.0;
    float envIn = 0.0f, envOut = 0.0f, autoGain = 1.0f;
    float envCoef = 0.0f, gainCoef = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftCapperProcessor)
};
