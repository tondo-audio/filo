#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LevelMeter : public juce::Component
{
public:
    LevelMeter();

    void setLevel(float peakLinear, float rmsLinear);
    void paint(juce::Graphics& g) override;

private:
    static float dbToNorm(float dB) noexcept;
    static float linearToDb(float linear) noexcept;

    static constexpr float kFloorDb              = -60.0f;
    static constexpr float kBarDecayDbPerFrame   = 0.4f;   // 12 dB/s @ 30 Hz (RMS, "VU-like")
    static constexpr float kPeakDecayDbPerFrame  = 0.27f;  // 8 dB/s @ 30 Hz
    static constexpr int   kPeakHoldDuration     = 60;     // 2 s @ 30 Hz
    static constexpr int   kClipHoldFrames       = 30;     // 1 s @ 30 Hz
    static constexpr float kClipThresholdDb      = -0.3f;

    float levelDb       { kFloorDb };
    float peakDb        { kFloorDb };
    int   peakHoldFrames { 0 };
    int   clipFrames     { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
