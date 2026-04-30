#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class LevelMeasurer;

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    void start(juce::AudioDeviceManager& dm);
    void stop();

    void addPlugin(std::unique_ptr<juce::AudioPluginInstance> instance);
    void removePlugin(int index);
    void movePlugin(int fromIndex, int toIndex);
    void setPluginBypassed(int index, bool bypassed);

    int getNumPlugins() const;
    juce::AudioPluginInstance* getPlugin(int index) const;
    bool isPluginBypassed(int index) const;

    float getInputLevel()  const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    void rebuildConnections();

    struct ChainEntry
    {
        juce::AudioProcessorGraph::NodeID nodeID;
        bool bypassed { false };
    };

    juce::AudioProcessorGraph graph;
    juce::AudioProcessorPlayer player;
    juce::AudioDeviceManager* deviceManager { nullptr };

    juce::AudioProcessorGraph::NodeID inputNodeID;
    juce::AudioProcessorGraph::NodeID outputNodeID;
    juce::AudioProcessorGraph::NodeID inputMeasurerNodeID;
    juce::AudioProcessorGraph::NodeID outputMeasurerNodeID;

    std::vector<ChainEntry> chain;

    std::atomic<float> inputLevel  { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
