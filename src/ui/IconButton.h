#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class IconButton : public juce::Button
{
public:
    explicit IconButton(const juce::String& name = {});
    ~IconButton() override = default;

    void setPath(juce::Path p);
    void setPathToggled(juce::Path p);
    void setDangerColour(bool isDanger) { danger = isDanger; repaint(); }
    void setLabel(juce::String text) { label = std::move(text); repaint(); }

protected:
    void paintButton(juce::Graphics& g, bool over, bool down) override;

private:
    juce::Path basePath;
    juce::Path toggledPath;
    juce::String label;
    bool danger { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
};
