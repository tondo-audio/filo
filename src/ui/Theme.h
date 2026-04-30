#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace filo::theme
{
    namespace colour
    {
        inline const juce::Colour bgBase          { 0xFF0E0E10 };
        inline const juce::Colour bgWindow        { 0xFF131316 };
        inline const juce::Colour surface         { 0xFF1A1A1D };
        inline const juce::Colour surfaceElevated { 0xFF222226 };
        inline const juce::Colour surfaceSunken   { 0xFF0A0A0C };

        inline const juce::Colour separator       { 0xFF2A2A2E };
        inline const juce::Colour outline         { 0xFF3A3A40 };
        inline const juce::Colour outlineHover    { 0xFF6E6E76 };

        inline const juce::Colour silver          { 0xFFC8C9CC };
        inline const juce::Colour silverBright    { 0xFFE8E9EC };
        inline const juce::Colour silverDim       { 0xFF7F8087 };

        inline const juce::Colour textPrimary     { 0xFFE6E6E8 };
        inline const juce::Colour textSecondary   { 0xFF9A9AA0 };
        inline const juce::Colour textDisabled    { 0xFF55555A };

        inline const juce::Colour danger          { 0xFF8E2A2A };
        inline const juce::Colour dangerSoft      { 0xFFB85454 };
    }

    namespace font
    {
        inline constexpr float xs      = 11.0f;
        inline constexpr float sm      = 12.0f;
        inline constexpr float base    = 13.0f;
        inline constexpr float lg      = 15.0f;
        inline constexpr float xl      = 18.0f;
        inline constexpr float display = 22.0f;

        inline constexpr float kUppercaseTracking = 0.12f;
    }

    namespace spacing
    {
        inline constexpr int s1 = 4;
        inline constexpr int s2 = 8;
        inline constexpr int s3 = 12;
        inline constexpr int s4 = 16;
        inline constexpr int s5 = 24;
        inline constexpr int s6 = 32;
    }

    namespace radius
    {
        inline constexpr float small  = 3.0f;
        inline constexpr float medium = 5.0f;
        inline constexpr float pill   = 999.0f;
    }

    inline juce::Font makeFont(float size,
                               int weight = juce::Font::plain,
                               bool uppercase = false)
    {
        juce::FontOptions opts;
        opts = opts.withHeight(size);

        if ((weight & juce::Font::bold) != 0)
            opts = opts.withStyle("Bold");

        juce::Font f(opts);

        if ((weight & juce::Font::italic) != 0)
            f.setItalic(true);

        if (uppercase)
            f.setExtraKerningFactor(font::kUppercaseTracking);

        return f;
    }
}
