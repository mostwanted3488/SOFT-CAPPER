#include "PluginEditor.h"
#include "UIData.h"

namespace
{
    constexpr int kW = 760, kH = 452;

    juce::String mimeFor (const juce::String& file)
    {
        if (file.endsWithIgnoreCase (".html")) return "text/html";
        if (file.endsWithIgnoreCase (".ttf"))  return "font/ttf";
        if (file.endsWithIgnoreCase (".js"))   return "text/javascript";
        if (file.endsWithIgnoreCase (".css"))  return "text/css";
        return "application/octet-stream";
    }

    juce::WebBrowserComponent::Options makeOptions (std::function<std::optional<juce::WebBrowserComponent::Resource> (const juce::String&)> provider,
                                                    std::function<void (const juce::var&)> onCap,
                                                    std::function<void (const juce::var&)> onGesture,
                                                    std::function<void (const juce::var&)> onLoud,
                                                    std::function<void (const juce::var&)> onReady)
    {
        using WBC = juce::WebBrowserComponent;
        return WBC::Options{}
            .withBackend (WBC::Options::Backend::webview2)
            .withWinWebView2Options (WBC::Options::WinWebView2{}
                                        .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                                 .getChildFile ("SoftCapperWebView"))
                                        .withBackgroundColour (juce::Colour (0xff0b0b0b)))
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider (std::move (provider))
            .withEventListener ("cap",     std::move (onCap))
            .withEventListener ("gesture", std::move (onGesture))
            .withEventListener ("loud",    std::move (onLoud))
            .withEventListener ("ready",   std::move (onReady));
    }
}

SoftCapperEditor::SoftCapperEditor (SoftCapperProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    capParam  = proc.apvts.getParameter ("cap");
    loudParam = proc.apvts.getParameter ("loud");

    web = std::make_unique<juce::WebBrowserComponent> (makeOptions (
        [this] (const juce::String& url) { return getResource (url); },
        [this] (const juce::var& o)
        {
            const float v = juce::jlimit (0.0f, 100.0f, (float) o.getProperty ("value", 0.0));
            lastCapSent = v;                                   // don't echo it back
            if (! inGesture) capParam->beginChangeGesture();
            capParam->setValueNotifyingHost (capParam->convertTo0to1 (v));
            if (! inGesture) capParam->endChangeGesture();
        },
        [this] (const juce::var& o)
        {
            const bool on = (bool) o.getProperty ("on", false);
            if (on && ! inGesture)       { inGesture = true;  capParam->beginChangeGesture(); }
            else if (! on && inGesture)  { inGesture = false; capParam->endChangeGesture(); }
        },
        [this] (const juce::var& o)
        {
            const bool on = (bool) o.getProperty ("on", false);
            lastLoudSent = on ? 1 : 0;
            loudParam->beginChangeGesture();
            loudParam->setValueNotifyingHost (on ? 1.0f : 0.0f);
            loudParam->endChangeGesture();
        },
        [this] (const juce::var&)
        {
            uiReady = true;
            pushStateToUI (true);
        }));

    addAndMakeVisible (*web);
    web->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) kW / (double) kH);
        c->setSizeLimits (kW / 2, kH / 2, kW * 2, kH * 2);
    }
    setSize (kW, kH);
    startTimerHz (30);
}

SoftCapperEditor::~SoftCapperEditor()
{
    stopTimer();
    if (inGesture)
        capParam->endChangeGesture();
}

void SoftCapperEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b0b0b));
}

void SoftCapperEditor::resized()
{
    web->setBounds (getLocalBounds());
}

void SoftCapperEditor::timerCallback()
{
    if (uiReady)
        pushStateToUI (false);
}

void SoftCapperEditor::pushStateToUI (bool force)
{
    const float cap = capParam->convertFrom0to1 (capParam->getValue());
    if (force || std::abs (cap - lastCapSent) > 0.005f)
    {
        lastCapSent = cap;
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("value", cap);
        web->emitEventIfBrowserIsVisible ("cap", juce::var (o.get()));
    }

    const int loud = loudParam->getValue() > 0.5f ? 1 : 0;
    if (force || loud != lastLoudSent)
    {
        lastLoudSent = loud;
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("on", loud == 1);
        web->emitEventIfBrowserIsVisible ("loud", juce::var (o.get()));
    }
}

std::optional<juce::WebBrowserComponent::Resource> SoftCapperEditor::getResource (const juce::String& url)
{
    auto path = url.upToFirstOccurrenceOf ("?", false, false);
    if (path.isEmpty() || path == "/")
        path = "index.html";
    const auto file = path.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < UIData::namedResourceListSize; ++i)
    {
        if (file == UIData::originalFilenames[i])
        {
            int size = 0;
            if (auto* data = UIData::getNamedResource (UIData::namedResourceList[i], size))
            {
                std::vector<std::byte> bytes ((size_t) size);
                std::memcpy (bytes.data(), data, (size_t) size);
                return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (file) };
            }
        }
    }
    return std::nullopt;
}
