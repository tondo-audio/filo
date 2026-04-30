#include "IconButton.h"
#include "Theme.h"

using namespace juce;

IconButton::IconButton(const String& name)
    : Button(name)
{
}

void IconButton::setPath(Path p)
{
    basePath = std::move(p);
    repaint();
}

void IconButton::setPathToggled(Path p)
{
    toggledPath = std::move(p);
    repaint();
}

void IconButton::paintButton(Graphics& g, bool over, bool down)
{
    namespace c = filo::theme::colour;

    const auto bounds = getLocalBounds().toFloat();

    if (over || down)
    {
        g.setColour(down ? c::surfaceElevated : c::surface);
        g.fillRoundedRectangle(bounds.reduced(1.0f), filo::theme::radius::small);
        g.setColour(c::outline);
        g.drawRoundedRectangle(bounds.reduced(1.0f), filo::theme::radius::small, 1.0f);
    }

    Colour iconColour;
    if (danger)
        iconColour = (over || down) ? c::danger : c::dangerSoft;
    else if (down)
        iconColour = c::silver;
    else if (over)
        iconColour = c::silverBright;
    else
        iconColour = c::silverDim;

    if (! isEnabled())
        iconColour = c::textDisabled;

    const Path& source = (getToggleState() && ! toggledPath.isEmpty()) ? toggledPath : basePath;

    if (! source.isEmpty())
    {
        const float iconBox = 24.0f;
        const float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.6f / iconBox;
        Path scaled = source;
        scaled.applyTransform(AffineTransform::scale(scale, scale)
                                  .translated((bounds.getWidth()  - iconBox * scale) * 0.5f,
                                              (bounds.getHeight() - iconBox * scale) * 0.5f));

        g.setColour(iconColour);

        if (label.isEmpty())
        {
            g.strokePath(scaled, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));
            if (getToggleState() && ! toggledPath.isEmpty())
                g.fillPath(scaled);
        }
        else
        {
            const float iconAreaW = bounds.getHeight();
            Path iconOnly = source;
            const float scaleL = bounds.getHeight() * 0.5f / iconBox;
            iconOnly.applyTransform(AffineTransform::scale(scaleL, scaleL)
                                        .translated(filo::theme::spacing::s2,
                                                    (bounds.getHeight() - iconBox * scaleL) * 0.5f));
            g.strokePath(iconOnly, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));

            g.setColour(iconColour);
            g.setFont(filo::theme::makeFont(filo::theme::font::sm,
                                            Font::plain,
                                            true));
            g.drawText(label,
                       (int) iconAreaW + filo::theme::spacing::s1,
                       0,
                       (int) bounds.getWidth() - (int) iconAreaW - filo::theme::spacing::s2,
                       (int) bounds.getHeight(),
                       Justification::centredLeft,
                       false);
        }
    }
    else if (! label.isEmpty())
    {
        g.setColour(iconColour);
        g.setFont(filo::theme::makeFont(filo::theme::font::sm, Font::plain, true));
        g.drawText(label, bounds.toNearestInt(), Justification::centred, false);
    }
}
