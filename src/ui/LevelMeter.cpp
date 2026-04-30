#include "LevelMeter.h"
#include "Theme.h"

#include <cmath>

using namespace juce;

static inline float dBToGain(float dB) noexcept
{
    return std::pow(10.0f, dB * 0.05f);
}

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

    if (rmsLinear >= 0.99f)
        clipFrames = kClipHoldFrames;
    else if (clipFrames > 0 && rmsLinear < dBToGain(-3.0f))
        --clipFrames;

    repaint();
}

void LevelMeter::paint(Graphics& g)
{
    namespace c = filo::theme::colour;

    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    g.setColour(c::surfaceSunken);
    g.fillRoundedRectangle(bounds, filo::theme::radius::small);

    const float clampedLevel = jlimit(0.0f, 1.0f, level);
    const float fillW = w * clampedLevel;

    if (fillW > 0.0f)
    {
        ColourGradient grad(c::silverDim.withAlpha(0.45f), 0.0f, 0.0f,
                            c::silverBright,                w, 0.0f, false);
        grad.addColour(0.92, c::danger);

        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds.withWidth(fillW), filo::theme::radius::small);
    }

    // dB ticks
    static const float ticksDb[] = { -40.0f, -30.0f, -20.0f, -12.0f, -6.0f, -3.0f, 0.0f };
    g.setColour(c::separator);
    const float tickH = h * 0.3f;
    for (float dB : ticksDb)
    {
        const float gain = dBToGain(dB);
        const float x = w * gain;
        g.fillRect(x, 0.0f, 1.0f, tickH);
        g.fillRect(x, h - tickH, 1.0f, tickH);
    }

    // Peak marker
    const float clampedPeak = jlimit(0.0f, 1.0f, peakLevel);
    const float peakX = w * clampedPeak;
    if (peakX > 1.0f)
    {
        g.setColour(c::silverBright);
        g.fillRect(peakX - 1.0f, 0.0f, 2.0f, h);
    }

    // Outline + clip border
    g.setColour(clipFrames > 0 ? c::danger : c::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), filo::theme::radius::small,
                           clipFrames > 0 ? 1.5f : 1.0f);
}
