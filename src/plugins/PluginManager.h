#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

class PluginManager
{
public:
    PluginManager();

    void scanPlugins(std::function<void(int /*percent*/, juce::String /*name*/)> progress = nullptr);

    void loadPluginAsync(const juce::PluginDescription& desc,
                         double sampleRate,
                         int blockSize,
                         std::function<void(std::unique_ptr<juce::AudioPluginInstance>,
                                            juce::String /*error*/)> callback);

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    juce::KnownPluginList& getKnownPlugins()           { return knownPlugins; }

    void saveToProperties(juce::PropertiesFile& props);
    void loadFromProperties(juce::PropertiesFile& props);

    bool isScanning() const { return scanning.load(); }

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList          knownPlugins;
    std::atomic<bool>              scanning { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};
