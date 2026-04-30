#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

class AudioEngine;
class PluginManager;

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(juce::AudioDeviceManager& dm,
               AudioEngine& engine,
               PluginManager& pluginMgr,
               juce::PropertiesFile& props);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
