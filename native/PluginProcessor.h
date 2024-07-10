#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <choc_javascript.h>

//==============================================================================
class EffectsPluginProcessor
    : public juce::AudioProcessor, public juce::Timer
{
public:
    //==============================================================================
    EffectsPluginProcessor();
    ~EffectsPluginProcessor() override;

    //==============================================================================
    void initialize(); // Declare the initialize method here

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void initJavaScriptEngine();
    void dispatchStateChange();
    void dispatchError(std::string const& name, std::string const& message);
    void handleChatMessage(std::string_view message);
    void sendMessageToAPI(const std::string& nickname, const std::string& message);
    void fetchNewMessages();
    void startFetchingMessages();
    void stopFetchingMessages();
    void timerCallback() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

private:
    choc::javascript::Context jsContext;
    std::string apiBaseUrl = "http://ableton-chat-01-72c15f63599a.herokuapp.com";
    std::string apiSendEndpoint = apiBaseUrl + "/messages/send";
    std::string apiGetEndpoint = apiBaseUrl + "/messages/get";
    int64_t lastMessageTimestamp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPluginProcessor)
};
