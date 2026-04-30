#include "IconPaths.h"

namespace filo::icons
{
    using juce::Path;
    using juce::MathConstants;

    Path dragHandle()
    {
        Path p;
        for (int i = 0; i < 3; ++i)
        {
            const float y = 8.0f + (float) i * 4.0f;
            p.startNewSubPath(5.0f, y);
            p.lineTo(19.0f, y);
        }
        return p;
    }

    Path power(bool filled)
    {
        Path p;
        constexpr float cx = 12.0f, cy = 13.0f, r = 7.0f;

        const float openHalf = MathConstants<float>::pi * 0.18f;
        p.addCentredArc(cx, cy, r, r, 0.0f,
                        openHalf,
                        MathConstants<float>::twoPi - openHalf,
                        true);

        p.startNewSubPath(cx, 3.0f);
        p.lineTo(cx, 11.0f);

        if (filled)
        {
            p.addEllipse(cx - 3.0f, cy - 2.0f, 6.0f, 6.0f);
        }

        return p;
    }

    Path cross()
    {
        Path p;
        p.startNewSubPath(6.0f, 6.0f);
        p.lineTo(18.0f, 18.0f);
        p.startNewSubPath(18.0f, 6.0f);
        p.lineTo(6.0f, 18.0f);
        return p;
    }

    Path plus()
    {
        Path p;
        p.startNewSubPath(12.0f, 5.0f);
        p.lineTo(12.0f, 19.0f);
        p.startNewSubPath(5.0f, 12.0f);
        p.lineTo(19.0f, 12.0f);
        return p;
    }

    Path refresh()
    {
        Path p;
        constexpr float cx = 12.0f, cy = 12.0f, r = 7.0f;

        const float startAngle = MathConstants<float>::pi * 0.25f;
        const float endAngle   = MathConstants<float>::twoPi - MathConstants<float>::pi * 0.05f;

        p.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);

        const float ax = cx + r * std::sin(endAngle);
        const float ay = cy - r * std::cos(endAngle);
        p.startNewSubPath(ax, ay);
        p.lineTo(ax - 3.0f, ay - 1.0f);
        p.startNewSubPath(ax, ay);
        p.lineTo(ax + 1.0f, ay - 3.5f);

        return p;
    }

    Path monitor()
    {
        Path p;
        p.addRoundedRectangle(4.0f, 5.0f, 16.0f, 12.0f, 1.5f);
        p.startNewSubPath(9.0f, 20.0f);
        p.lineTo(15.0f, 20.0f);
        p.startNewSubPath(12.0f, 17.0f);
        p.lineTo(12.0f, 20.0f);
        return p;
    }

    Path chevronDown()
    {
        Path p;
        p.startNewSubPath(6.0f, 10.0f);
        p.lineTo(12.0f, 16.0f);
        p.lineTo(18.0f, 10.0f);
        return p;
    }

    Path clip()
    {
        Path p;
        p.startNewSubPath(12.0f, 4.0f);
        p.lineTo(21.0f, 20.0f);
        p.lineTo(3.0f, 20.0f);
        p.closeSubPath();
        p.startNewSubPath(12.0f, 10.0f);
        p.lineTo(12.0f, 16.0f);
        return p;
    }
}
