#include "FiloLookAndFeel.h"
#include "Theme.h"
#include "IconPaths.h"
#include "FiloAssets.h"

using namespace juce;

namespace c = filo::theme::colour;

FiloLookAndFeel::FiloLookAndFeel()
{
    interTypeface = Typeface::createSystemTypefaceFor(
        filo::assets::InterVariable_ttf,
        filo::assets::InterVariable_ttfSize);

    if (interTypeface != nullptr)
        setDefaultSansSerifTypeface(interTypeface);

    setColour(ResizableWindow::backgroundColourId,   c::bgWindow);
    setColour(DocumentWindow::backgroundColourId,    c::bgWindow);
    setColour(DocumentWindow::textColourId,          c::textPrimary);

    setColour(Label::textColourId,                   c::textPrimary);
    setColour(Label::backgroundColourId,             Colours::transparentBlack);

    setColour(TextButton::buttonColourId,            c::surfaceElevated);
    setColour(TextButton::buttonOnColourId,          c::surface);
    setColour(TextButton::textColourOffId,           c::silver);
    setColour(TextButton::textColourOnId,            c::silverBright);

    setColour(ToggleButton::textColourId,            c::textSecondary);
    setColour(ToggleButton::tickColourId,            c::silverBright);
    setColour(ToggleButton::tickDisabledColourId,    c::silverDim);

    setColour(ComboBox::backgroundColourId,          c::surface);
    setColour(ComboBox::textColourId,                c::textPrimary);
    setColour(ComboBox::arrowColourId,               c::silver);
    setColour(ComboBox::outlineColourId,             c::outline);
    setColour(ComboBox::buttonColourId,              c::surface);
    setColour(ComboBox::focusedOutlineColourId,      c::outlineHover);

    setColour(PopupMenu::backgroundColourId,         c::bgWindow);
    setColour(PopupMenu::textColourId,               c::textPrimary);
    setColour(PopupMenu::headerTextColourId,         c::textSecondary);
    setColour(PopupMenu::highlightedBackgroundColourId, c::surfaceElevated);
    setColour(PopupMenu::highlightedTextColourId,    c::silverBright);

    setColour(ScrollBar::backgroundColourId,         Colours::transparentBlack);
    setColour(ScrollBar::thumbColourId,              c::silverDim);
    setColour(ScrollBar::trackColourId,              c::surfaceSunken);

    setColour(AlertWindow::backgroundColourId,       c::bgWindow);
    setColour(AlertWindow::textColourId,             c::textPrimary);
    setColour(AlertWindow::outlineColourId,          c::outline);

    setColour(TooltipWindow::backgroundColourId,     c::surfaceElevated);
    setColour(TooltipWindow::textColourId,           c::textPrimary);
    setColour(TooltipWindow::outlineColourId,        c::outline);
}

Typeface::Ptr FiloLookAndFeel::getTypefaceForFont(const Font& f)
{
    if (interTypeface != nullptr && f.getTypefaceName() == Font::getDefaultSansSerifFontName())
        return interTypeface;

    return LookAndFeel_V4::getTypefaceForFont(f);
}

void FiloLookAndFeel::drawButtonBackground(Graphics& g, Button& b,
                                           const Colour& /*backgroundColour*/,
                                           bool isHighlighted, bool isDown)
{
    const auto bounds = b.getLocalBounds().toFloat().reduced(0.5f);

    Colour fill;
    if (isDown)
        fill = c::surfaceElevated.brighter(0.05f);
    else if (isHighlighted)
        fill = c::surfaceElevated;
    else
        fill = c::surface;

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, filo::theme::radius::small);

    g.setColour(isHighlighted ? c::outlineHover : c::outline);
    g.drawRoundedRectangle(bounds, filo::theme::radius::small, 1.0f);
}

void FiloLookAndFeel::drawButtonText(Graphics& g, TextButton& b,
                                     bool /*isHighlighted*/, bool /*isDown*/)
{
    g.setColour(b.findColour(b.getToggleState() ? TextButton::textColourOnId
                                                : TextButton::textColourOffId)
                    .withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.5f));
    g.setFont(getTextButtonFont(b, b.getHeight()));

    const int margin = jmin(b.getWidth(), b.getHeight()) / 4;
    g.drawFittedText(b.getButtonText(),
                     margin, 0, b.getWidth() - margin * 2, b.getHeight(),
                     Justification::centred, 1);
}

Font FiloLookAndFeel::getTextButtonFont(TextButton&, int /*buttonHeight*/)
{
    return filo::theme::makeFont(filo::theme::font::sm, Font::plain, true);
}

void FiloLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool /*down*/,
                                   int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                   ComboBox& box)
{
    const auto bounds = Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);

    g.setColour(c::surface);
    g.fillRoundedRectangle(bounds, filo::theme::radius::small);

    g.setColour(box.isMouseOver() ? c::outlineHover : c::outline);
    g.drawRoundedRectangle(bounds, filo::theme::radius::small, 1.0f);

    Path arrow = filo::icons::chevronDown();
    const float iconBox = 24.0f;
    const float scale = (float) height * 0.45f / iconBox;
    arrow.applyTransform(AffineTransform::scale(scale, scale)
                              .translated((float) width - iconBox * scale - 8.0f,
                                          ((float) height - iconBox * scale) * 0.5f));
    g.setColour(c::silver);
    g.strokePath(arrow, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

Font FiloLookAndFeel::getComboBoxFont(ComboBox&)
{
    return filo::theme::makeFont(filo::theme::font::sm);
}

void FiloLookAndFeel::positionComboBoxText(ComboBox& box, Label& label)
{
    label.setBounds(10, 0,
                    box.getWidth() - 28,
                    box.getHeight());
    label.setFont(getComboBoxFont(box));
}

void FiloLookAndFeel::drawPopupMenuBackground(Graphics& g, int width, int height)
{
    const auto bounds = Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    g.setColour(c::bgWindow);
    g.fillRoundedRectangle(bounds, filo::theme::radius::medium);

    g.setColour(c::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), filo::theme::radius::medium, 1.0f);
}

Font FiloLookAndFeel::getPopupMenuFont()
{
    return filo::theme::makeFont(filo::theme::font::sm);
}

void FiloLookAndFeel::drawScrollbar(Graphics& g, ScrollBar& /*bar*/,
                                    int x, int y, int width, int height,
                                    bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                    bool isMouseOver, bool isMouseDown)
{
    Rectangle<float> thumb;
    if (isScrollbarVertical)
        thumb = Rectangle<float>((float) x + 1.0f, (float) thumbStartPosition,
                                 (float) width - 2.0f, (float) thumbSize);
    else
        thumb = Rectangle<float>((float) thumbStartPosition, (float) y + 1.0f,
                                 (float) thumbSize, (float) height - 2.0f);

    Colour thumbColour;
    if (isMouseDown)
        thumbColour = c::silverBright;
    else if (isMouseOver)
        thumbColour = c::silver;
    else
        thumbColour = c::silverDim;

    g.setColour(thumbColour.withAlpha(0.85f));
    g.fillRoundedRectangle(thumb, jmin(thumb.getWidth(), thumb.getHeight()) * 0.5f);
}
