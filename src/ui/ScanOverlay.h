#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ScanOverlay : public juce::Component,
                    private juce::Timer
{
public:
    ScanOverlay();
    ~ScanOverlay() override = default;

    void setCurrentItem(const juce::String& name);
    void setActive(bool shouldBeActive);

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::String currentItem;
    float spinnerAngle { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScanOverlay)
};
