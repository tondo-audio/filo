#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "ui/DeviceSelectorBar.h"
#include "ui/PluginChainView.h"
#include "ui/LevelMeter.h"
#include "ui/LogoView.h"
#include "ui/IconButton.h"

class AudioEngine;
class PluginManager;

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      private juce::Timer
{
public:
    MainComponent(juce::AudioDeviceManager& dm,
                  AudioEngine& engine,
                  PluginManager& pluginMgr,
                  juce::PropertiesFile& props);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void showAddPluginMenu();
    void startScan();

    juce::AudioDeviceManager& deviceManager;
    AudioEngine&  engine;
    PluginManager& pluginMgr;
    juce::PropertiesFile& props;

    LogoView logo;

    DeviceSelectorBar deviceBar;

    juce::Viewport       chainViewport;
    PluginChainView      chainView;

    LevelMeter inputMeter;
    LevelMeter outputMeter;

    IconButton addPluginButton;
    IconButton rescanButton;

    juce::Label inputMeterLabel  { {}, "INPUT" };
    juce::Label outputMeterLabel { {}, "OUTPUT" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
