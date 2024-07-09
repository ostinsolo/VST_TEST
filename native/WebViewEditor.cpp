#include "PluginProcessor.h"
#include "WebViewEditor.h"

// A helper for reading numbers from a choc::Value, which seems to opportunistically parse
// JSON numbers into ints or 32-bit floats whenever it wants.
double numberFromChocValue(const choc::value::ValueView& v) {
    return (
        v.isFloat32() ? (double)v.getFloat32()
        : (v.isFloat64() ? v.getFloat64()
        : (v.isInt32() ? (double)v.getInt32()
        : (double)v.getInt64())));
}

std::string getMimeType(std::string const& ext) {
    static std::unordered_map<std::string, std::string> mimeTypes{
        { ".html",   "text/html" },
        { ".js",     "application/javascript" },
        { ".css",    "text/css" },
    };

    if (mimeTypes.count(ext) > 0)
        return mimeTypes.at(ext);

    return "application/octet-stream";
}

//==============================================================================
WebViewEditor::WebViewEditor(juce::AudioProcessor* proc, juce::File const& assetDirectory, int width, int height)
    : juce::AudioProcessorEditor(proc)
{
    setSize(width, height);
    setResizable(true, true); // Add this line to enable resizing

    choc::ui::WebView::Options opts;

#if JUCE_DEBUG
    opts.enableDebugMode = true;
#endif

#if ! ELEM_DEV_LOCALHOST
    opts.fetchResource = [=](const choc::ui::WebView::Options::Path& p) -> std::optional<choc::ui::WebView::Options::Resource> {
        auto relPath = "." + (p == "/" ? "/index.html" : p);
        auto f = assetDirectory.getChildFile(relPath);
        juce::MemoryBlock mb;

        if (!f.existsAsFile() || !f.loadFileAsData(mb))
            return {};

        return choc::ui::WebView::Options::Resource{
            std::vector<uint8_t>(mb.begin(), mb.end()),
            getMimeType(f.getFileExtension().toStdString())
        };
    };
#endif

    webView = std::make_unique<choc::ui::WebView>(opts);

#if JUCE_MAC
    viewContainer.setView(webView->getViewHandle());
#elif JUCE_WINDOWS
    viewContainer.setHWND(webView->getViewHandle());
#else
#error "We only support MacOS and Windows here yet."
#endif

    addAndMakeVisible(viewContainer);

    // Install message passing handlers
    webView->bind("__postNativeMessage__", [=](const choc::value::ValueView& args) -> choc::value::Value {
        if (args.isArray()) {
            auto eventName = args[0].getString();

            if (eventName == "ready") {
                if (auto* ptr = dynamic_cast<EffectsPluginProcessor*>(getAudioProcessor())) {
                    ptr->dispatchStateChange();
                }
            } else if (eventName == "receiveMessage") {
                if (args.size() > 1) {
                    auto messageJson = args[1].getString();
                    if (auto* ptr = dynamic_cast<EffectsPluginProcessor*>(getAudioProcessor())) {
                        ptr->handleChatMessage(messageJson);
                    }
                }
            }

#if ELEM_DEV_LOCALHOST
            if (eventName == "reload") {
                if (auto* ptr = dynamic_cast<EffectsPluginProcessor*>(getAudioProcessor())) {
                    ptr->initJavaScriptEngine();
                    ptr->dispatchStateChange();
                }
            }
#endif
        }

        return {};
    });

#if ELEM_DEV_LOCALHOST
    webView->navigate("http://localhost:5173");
#endif
}

choc::ui::WebView* WebViewEditor::getWebViewPtr()
{
    return webView.get();
}

void WebViewEditor::paint(juce::Graphics& g)
{
}

void WebViewEditor::resized()
{
    viewContainer.setBounds(getLocalBounds());
}
