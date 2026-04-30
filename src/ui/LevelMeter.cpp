#include "LevelMeter.h"

using namespace juce;

LevelMeter::LevelMeter()
{
    setOpaque(false);
}

void LevelMeter::setLevel(float rmsLinear)
{
    const float kDecayFactor = 0.85f;
    level = rmsLinear + (level - rmsLinear) * kDecayFactor;

    if (rmsLinear > peakLevel)
    {
        peakLevel      = rmsLinear;
        peakHoldFrames = kPeakHoldDuration;
    }
    else if (peakHoldFrames > 0)
    {
        --peakHoldFrames;
    }
    else
    {
        peakLevel *= kDecayFactor;
    }

    repaint();
}

void LevelMeter::paint(Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    g.setColour(Colour(0xFF1A1A1A));
    g.fillRect(bounds);

    const float clampedLevel = jlimit(0.0f, 1.0f, level);
    const float fillW = w * clampedLevel;

    if (fillW > 0.0f)
    {
        ColourGradient grad(Colour(0xFF3CB371), 0.0f, 0.0f,
                            Colour(0xFFFF4444), w, 0.0f, false);
        grad.addColour(0.7, Colour(0xFFFFCC00));

        g.setGradientFill(grad);
        g.fillRect(bounds.withWidth(fillW));
    }

    // Peak marker
    const float clampedPeak = jlimit(0.0f, 1.0f, peakLevel);
    const float peakX = w * clampedPeak;
    if (peakX > 1.0f)
    {
        g.setColour(Colours::white.withAlpha(0.8f));
        g.fillRect(peakX - 1.0f, 0.0f, 2.0f, h);
    }

    g.setColour(Colour(0xFF404040));
    g.drawRect(bounds, 1.0f);
}
