#include "PluginProcessor.h"
#include "WebViewEditor.h"
#include <choc_javascript_QuickJS.h>

//==============================================================================
// A quick helper for locating bundled asset files
juce::File getAssetsDirectory()
{
#if JUCE_MAC
    auto assetsDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile)
        .getChildFile("Contents/Resources/dist");
#elif JUCE_WINDOWS
    auto assetsDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile)
        .getParentDirectory()  // Plugin.vst3/Contents/<arch>/
        .getParentDirectory()  // Plugin.vst3/Contents/
        .getChildFile("Resources/dist");
#else
#error "We only support Mac and Windows here yet."
#endif

    return assetsDir;
}

void EffectsPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("MYPLUGINSETTINGS");

    // Store any necessary state information here

    copyXmlToBinary(xml, destData);
}

void EffectsPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName("MYPLUGINSETTINGS"))
        {
            // Retrieve any necessary state information here
        }
    }
}

//==============================================================================
// Constructor
EffectsPluginProcessor::EffectsPluginProcessor()
    : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      jsContext(choc::javascript::createQuickJSContext())
{
    // Minimal and safe operations in the constructor
}

void EffectsPluginProcessor::initialize() {
    // Initialize JavaScript engine and message fetching in a separate method
    try {
        initJavaScriptEngine();
        startFetchingMessages();
    } catch (const std::exception& e) {
        DBG("Initialization error: " << e.what());
    } catch (...) {
        DBG("Unknown exception during initialization");
    }
}

//==============================================================================
// Destructor
EffectsPluginProcessor::~EffectsPluginProcessor()
{
    stopFetchingMessages();  // Ensure all message fetching is ceased
}

//==============================================================================
// JavaScript engine initialization
void EffectsPluginProcessor::initJavaScriptEngine()
{
    try {
        jsContext = choc::javascript::createQuickJSContext();

        // Install native interop functions in our JavaScript environment
        jsContext.registerFunction("__postNativeMessage__", [this](choc::javascript::ArgumentList args) {
            try {
                if (args.size() > 1 && args[0]->isString() && args[1]->isString()) {
                    auto eventName = args[0]->getString();
                    auto eventData = args[1]->getString();

                    if (eventName == "sendMessage") {
                        auto editor = dynamic_cast<WebViewEditor*>(getActiveEditor());
                        if (editor) {
                            auto webView = editor->getWebViewPtr();
                            if (webView) {
                                auto eventDataValue = choc::value::createString(eventData);
                                webView->evaluateJavascript("globalThis.__sendMessage__(" + choc::json::toString(eventDataValue) + ")");
                            } else {
                                DBG("WebView pointer is null");
                            }
                        } else {
                            DBG("Editor is not a WebViewEditor");
                        }
                    }
                }
            } catch (const std::exception& e) {
                DBG("Exception in __postNativeMessage__: " << e.what());
            } catch (...) {
                DBG("Unknown exception in __postNativeMessage__");
            }

            return choc::value::Value();
        });

        jsContext.registerFunction("__log__", [this](choc::javascript::ArgumentList args) {
            const auto* kDispatchScript = R"script(
(function() {
  console.log(...JSON.parse(%));
  return true;
})();
)script";

            try {
                if (auto* editor = static_cast<WebViewEditor*>(getActiveEditor())) {
                    auto v = choc::value::createEmptyArray();

                    for (size_t i = 0; i < args.numArgs; ++i) {
                        v.addArrayElement(*args[i]);
                    }

                    auto expr = juce::String(kDispatchScript).replace("%", choc::json::toString(v)).toStdString();
                    editor->getWebViewPtr()->evaluateJavascript(expr);
                } else {
                    for (size_t i = 0; i < args.numArgs; ++i) {
                        DBG(choc::json::toString(*args[i]));
                    }
                }
            } catch (const std::exception& e) {
                DBG("Exception in __log__: " << e.what());
            } catch (...) {
                DBG("Unknown exception in __log__");
            }

            return choc::value::Value();
        });

        jsContext.registerFunction("__sendMessage__", [this](choc::javascript::ArgumentList args) {
            try {
                if (args.size() > 0 && args[0]->isString()) {
                    auto serializedMessage = args[0]->getString();
                    auto messageJson = juce::JSON::parse(juce::String(serializedMessage.data(), serializedMessage.length()));

                    if (messageJson.isObject()) {
                        auto message = messageJson.getProperty("message", "").toString().toStdString();
                        auto username = messageJson.getProperty("username", "").toString().toStdString();

                        sendMessageToAPI(username, message);
                    }
                }
            } catch (const std::exception& e) {
                DBG("Exception in __sendMessage__: " << e.what());
            } catch (...) {
                DBG("Unknown exception in __sendMessage__");
            }

            return choc::value::Value();
        });

        // A simple shim to write various console operations to our native __log__ handler
        jsContext.evaluate(R"shim(
(function() {
  if (typeof globalThis.console === 'undefined') {
    globalThis.console = {
      log(...args) {
        __log__('[embedded:log]', ...args);
      },
      warn(...args) {
          __log__('[embedded:warn]', ...args);
      },
      error(...args) {
          __log__('[embedded:error]', ...args);
      }
    };
  }
})();
        )shim");

        // Load and evaluate the main.js script
        auto chatScriptFile = getAssetsDirectory().getChildFile("main.js");
        if (chatScriptFile.existsAsFile()) {
            auto chatScriptContents = chatScriptFile.loadFileAsString().toStdString();
            jsContext.evaluate(chatScriptContents);
        } else {
            DBG("main.js file does not exist");
        }
    } catch (const std::exception& e) {
        DBG("Exception in initJavaScriptEngine: " << e.what());
    } catch (...) {
        DBG("Unknown exception in initJavaScriptEngine");
    }
}

//==============================================================================
// Message sending and fetching
void EffectsPluginProcessor::sendMessageToAPI(const std::string& nickname, const std::string& message) {
    try {
        juce::URL url(apiSendEndpoint);

        // Create postData object
        auto postData = std::make_unique<juce::DynamicObject>();
        postData->setProperty("nickname", juce::String(nickname));
        postData->setProperty("message", juce::String(message));

        // Convert postData to JSON string
        juce::var jsonVar(postData.get());
        juce::String jsonString = juce::JSON::toString(jsonVar);

        // Prepare the InputStreamOptions
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                       .withExtraHeaders("Content-Type: application/json")
                       .withConnectionTimeoutMs(10000)
                       .withHttpRequestCmd("POST");

        // Create the input stream with post data
        auto stream = url.createInputStream(options);

        if (stream != nullptr) {
            // Process the response synchronously
            auto responseBody = stream->readEntireStreamAsString();
            auto responseJson = juce::JSON::parse(responseBody);

            if (responseJson.isObject()) {
                auto messagesArray = responseJson.getProperty("messages", juce::var()).getArray();
                if (messagesArray != nullptr && messagesArray->size() > 0) {
                    lastMessageTimestamp = messagesArray->getReference(messagesArray->size() - 1).getProperty("createdAt", 0);
                    for (auto& messageVar : *messagesArray) {
                        auto messageJson = juce::JSON::toString(messageVar);
                        handleChatMessage(messageJson.toStdString());
                    }
                }
            }
        } else {
            DBG("Failed to create input stream for sendMessageToAPI");
        }
    } catch (const std::exception& e) {
        DBG("Exception in sendMessageToAPI: " << e.what());
    } catch (...) {
        DBG("Unknown exception in sendMessageToAPI");
    }
}

void EffectsPluginProcessor::fetchNewMessages() {
    try {
        DBG("Entering fetchNewMessages");

        juce::URL url(apiGetEndpoint);

        // Create postData object
        auto postData = std::make_unique<juce::DynamicObject>();
        postData->setProperty("fromTimestamp", lastMessageTimestamp);

        // Convert postData to JSON string
        juce::var jsonVar(postData.get());
        juce::String jsonString = juce::JSON::toString(jsonVar);

        // Prepare the InputStreamOptions
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                       .withExtraHeaders("Content-Type: application/json")
                       .withConnectionTimeoutMs(10000)
                       .withHttpRequestCmd("POST");

        // Create the input stream with post data
        auto stream = url.createInputStream(options);

        if (stream != nullptr) {
            // Process the response synchronously
            auto responseBody = stream->readEntireStreamAsString();
            auto responseJson = juce::JSON::parse(responseBody);

            if (responseJson.isObject()) {
                auto messagesArray = responseJson.getProperty("messages", juce::var()).getArray();
                if (messagesArray != nullptr && messagesArray->size() > 0) {
                    lastMessageTimestamp = messagesArray->getReference(messagesArray->size() - 1).getProperty("createdAt", 0);
                    for (auto& messageVar : *messagesArray) {
                        auto messageJson = juce::JSON::toString(messageVar);
                        handleChatMessage(messageJson.toStdString());
                    }
                }
            } else {
                DBG("Response JSON is not an object");
            }
        } else {
            DBG("Failed to create input stream for fetchNewMessages");
        }
    } catch (const std::exception& e) {
        DBG("Exception in fetchNewMessages: " << e.what());
    } catch (...) {
        DBG("Unknown exception in fetchNewMessages");
    }

    DBG("Exiting fetchNewMessages");
}

//==============================================================================
// Remaining plugin methods

void EffectsPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    // Implement your audio processing logic here
}

void EffectsPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Add any necessary preparation code here
}

void EffectsPluginProcessor::releaseResources() {
    // Add any necessary resource cleanup code here
}

//==============================================================================
// Dispatchers for state changes and errors
void EffectsPluginProcessor::dispatchStateChange() {
    const auto* kDispatchScript = R"script(
(function() {
  if (typeof globalThis.__receiveStateChange__ !== 'function')
    return false;

  globalThis.__receiveStateChange__();
  return true;
})();
)script";

    try {
        if (auto* editor = static_cast<WebViewEditor*>(getActiveEditor())) {
            editor->getWebViewPtr()->evaluateJavascript(kDispatchScript);
        }

        jsContext.evaluate(kDispatchScript);
    } catch (const std::exception& e) {
        DBG("Exception in dispatchStateChange: " << e.what());
    } catch (...) {
        DBG("Unknown exception in dispatchStateChange");
    }
}

void EffectsPluginProcessor::dispatchError(std::string const& name, std::string const& message) {
    const auto* kDispatchScript = R"script(
(function() {
  if (typeof globalThis.__receiveError__ !== 'function')
    return false;

  let e = new Error(%);
  e.name = @;

  globalThis.__receiveError__(e);
  return true;
})();
)script";

    try {
        auto nameStr = juce::String(name);
        auto messageStr = juce::String(message);
        auto expr = juce::String(kDispatchScript).replace("@", juce::JSON::toString(nameStr)).replace("%", juce::JSON::toString(messageStr)).toStdString();

        if (auto* editor = static_cast<WebViewEditor*>(getActiveEditor())) {
            editor->getWebViewPtr()->evaluateJavascript(expr);
        }

        jsContext.evaluate(expr);
    } catch (const std::exception& e) {
        DBG("Exception in dispatchError: " << e.what());
    } catch (...) {
        DBG("Unknown exception in dispatchError");
    }
}

void EffectsPluginProcessor::handleChatMessage(std::string_view message) {
    try {
        auto messageJson = juce::JSON::parse(juce::String(message.data(), message.length()));

        if (messageJson.isObject()) {
            auto sender = messageJson.getProperty("nickname", "").toString();
            auto text = messageJson.getProperty("message", "").toString();
            auto timestamp = messageJson.getProperty("createdAt", 0).toString();

            auto messageObject = std::make_unique<juce::DynamicObject>();
            messageObject->setProperty("sender", sender);
            messageObject->setProperty("text", text);
            messageObject->setProperty("timestamp", timestamp);

            auto messageString = juce::JSON::toString(juce::var(messageObject.release()));

            if (auto* editor = static_cast<WebViewEditor*>(getActiveEditor())) {
                auto webView = editor->getWebViewPtr();
                if (webView) {
                    std::string jsCall = "globalThis.__receiveMessage__(" + messageString.toStdString() + ")";
                    webView->evaluateJavascript(jsCall);
                } else {
                    DBG("WebView pointer is null in handleChatMessage");
                }
            }
        }
    } catch (const std::exception& e) {
        DBG("Exception in handleChatMessage: " << e.what());
    } catch (...) {
        DBG("Unknown exception in handleChatMessage");
    }
}

//==============================================================================
// Editor creation
juce::AudioProcessorEditor* EffectsPluginProcessor::createEditor() {
    auto* editor = new WebViewEditor(this, getAssetsDirectory(), 2000, 500);
    if (!editor->getWebViewPtr()) {
        DBG("Failed to initialize WebViewEditor");
    }
    return editor;
}

bool EffectsPluginProcessor::hasEditor() const {
    return true;
}

//==============================================================================
// Plugin properties
const juce::String EffectsPluginProcessor::getName() const {
    return JucePlugin_Name;
}

bool EffectsPluginProcessor::acceptsMidi() const {
    return false;
}

bool EffectsPluginProcessor::producesMidi() const {
    return false;
}

bool EffectsPluginProcessor::isMidiEffect() const {
    return false;
}

double EffectsPluginProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int EffectsPluginProcessor::getNumPrograms() {
    return 1;
}

int EffectsPluginProcessor::getCurrentProgram() {
    return 0;
}

void EffectsPluginProcessor::setCurrentProgram(int /* index */) {}
const juce::String EffectsPluginProcessor::getProgramName(int /* index */) { return {}; }
void EffectsPluginProcessor::changeProgramName(int /* index */, const juce::String& /* newName */) {}

//==============================================================================
// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    auto* processor = new EffectsPluginProcessor();
    processor->initialize();  // Ensure initialize is called after construction
    return processor;
}
