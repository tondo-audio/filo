#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LevelMeter : public juce::Component
{
public:
    LevelMeter();

    void setLevel(float rmsLinear);
    void paint(juce::Graphics& g) override;

private:
    float level      { 0.0f };
    float peakLevel  { 0.0f };
    int   peakHoldFrames { 0 };

    static constexpr int kPeakHoldDuration = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
