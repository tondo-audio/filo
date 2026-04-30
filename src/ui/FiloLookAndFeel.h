#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FiloLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FiloLookAndFeel();
    ~FiloLookAndFeel() override = default;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool isHighlighted, bool isDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override;

    void drawScrollbar(juce::Graphics&, juce::ScrollBar&,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;
    int getDefaultScrollbarWidth() override { return 6; }

private:
    juce::Typeface::Ptr interTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiloLookAndFeel)
};
