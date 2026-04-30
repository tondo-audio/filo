#include "ScanOverlay.h"
#include "Theme.h"

using namespace juce;

namespace c  = filo::theme::colour;
namespace sp = filo::theme::spacing;

ScanOverlay::ScanOverlay()
{
    setVisible(false);
    setInterceptsMouseClicks(true, false);
    setWantsKeyboardFocus(true);
}

void ScanOverlay::setActive(bool shouldBeActive)
{
    setVisible(shouldBeActive);
    if (shouldBeActive)
    {
        toFront(false);
        startTimerHz(30);
        grabKeyboardFocus();
    }
    else
    {
        stopTimer();
        currentItem = {};
    }
    repaint();
}

void ScanOverlay::setCurrentItem(const String& name)
{
    if (currentItem == name) return;
    currentItem = name;
    repaint();
}

void ScanOverlay::timerCallback()
{
    spinnerAngle += MathConstants<float>::twoPi / 30.0f / 1.2f; // ~1.2s per giro
    if (spinnerAngle > MathConstants<float>::twoPi)
        spinnerAngle -= MathConstants<float>::twoPi;
    repaint();
}

void ScanOverlay::paint(Graphics& g)
{
    // 1. veil semi-trasparente che oscura la UI sotto
    g.fillAll(c::bgBase.withAlpha(0.86f));

    // 2. card centrale
    constexpr int cardW = 360;
    constexpr int cardH = 200;
    auto bounds = getLocalBounds();
    Rectangle<float> card(((float) bounds.getWidth()  - cardW) * 0.5f,
                          ((float) bounds.getHeight() - cardH) * 0.5f,
                          (float) cardW, (float) cardH);

    g.setColour(c::surfaceElevated);
    g.fillRoundedRectangle(card, filo::theme::radius::medium);
    g.setColour(c::outline);
    g.drawRoundedRectangle(card.reduced(0.5f), filo::theme::radius::medium, 1.0f);

    // 3. spinner in alto al centro
    {
        const float cx = card.getCentreX();
        const float cy = card.getY() + sp::s5;
        const float radius = 12.0f;
        constexpr int dots = 8;

        for (int i = 0; i < dots; ++i)
        {
            const float angle = (float) i * MathConstants<float>::twoPi / (float) dots
                                + spinnerAngle;
            const float dx = cx + radius * std::sin(angle);
            const float dy = cy - radius * std::cos(angle);
            const float alpha = 0.15f + 0.85f * ((float) i / (float) (dots - 1));

            g.setColour(c::silverBright.withAlpha(alpha));
            g.fillEllipse(dx - 2.0f, dy - 2.0f, 4.0f, 4.0f);
        }
    }

    // 4. testo
    auto textArea = card.toNearestInt().reduced(sp::s5, sp::s4);
    textArea.removeFromTop(sp::s5);  // spazio sotto spinner

    g.setColour(c::silverBright);
    g.setFont(filo::theme::makeFont(filo::theme::font::sm, Font::plain, true));
    g.drawText("SCANSIONE PLUGIN",
               textArea.removeFromTop(18),
               Justification::centred, false);

    textArea.removeFromTop(sp::s2);

    g.setColour(c::textSecondary);
    g.setFont(filo::theme::makeFont(filo::theme::font::xs));
    g.drawFittedText("Analisi dei plugin AU e VST3 installati. "
                     "L'interfaccia riprende al termine.",
                     textArea.removeFromTop(34),
                     Justification::centred, 2);

    textArea.removeFromTop(sp::s2);

    g.setColour(c::silver);
    g.setFont(filo::theme::makeFont(filo::theme::font::sm));
    const String label = currentItem.isNotEmpty() ? currentItem : String("In attesa...");
    g.drawText(label,
               textArea.removeFromTop(20),
               Justification::centred, true);
}
