#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// The interface is the same HTML/SVG scene as the browser preview, shown in a
// native WebView (WebView2 on Windows, WKWebView on macOS) and bound to the
// plugin parameters through JUCE's native integration.
class SoftCapperEditor final : public juce::AudioProcessorEditor,
                               private juce::Timer
{
public:
    explicit SoftCapperEditor (SoftCapperProcessor&);
    ~SoftCapperEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void pushStateToUI (bool force);
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    SoftCapperProcessor& proc;
    juce::RangedAudioParameter* capParam  = nullptr;
    juce::RangedAudioParameter* loudParam = nullptr;

    std::unique_ptr<juce::WebBrowserComponent> web;

    float lastCapSent  = -1.0f;
    int   lastLoudSent = -1;
    bool  uiReady = false;
    bool  inGesture = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftCapperEditor)
};
