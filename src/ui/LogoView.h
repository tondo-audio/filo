#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LogoView : public juce::Component
{
public:
    LogoView();
    ~LogoView() override = default;

    void paint(juce::Graphics& g) override;

private:
    std::unique_ptr<juce::Drawable> svgDrawable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogoView)
};
