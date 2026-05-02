#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

class DeviceSelectorBar : public juce::Component,
                          private juce::ComboBox::Listener,
                          private juce::ChangeListener
{
public:
    explicit DeviceSelectorBar(juce::AudioDeviceManager& dm);
    ~DeviceSelectorBar() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void populateDeviceLists();
    void applySetup();
    void comboBoxChanged(juce::ComboBox* box) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    juce::AudioDeviceManager& deviceManager;

    juce::Label inputLabel      { {}, "INPUT"  };
    juce::Label outputLabel     { {}, "OUTPUT" };
    juce::Label sampleRateLabel { {}, "RATE"   };
    juce::Label bufferLabel     { {}, "BUFFER" };

    juce::ComboBox inputCombo;
    juce::ComboBox outputCombo;
    juce::ComboBox sampleRateCombo;
    juce::ComboBox bufferCombo;

    bool updating { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceSelectorBar)
};
