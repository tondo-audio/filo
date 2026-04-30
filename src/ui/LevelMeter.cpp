#include "LevelMeter.h"
#include "Theme.h"

#include <cmath>

using namespace juce;

LevelMeter::LevelMeter()
{
    setOpaque(false);
}

float LevelMeter::dbToNorm(float dB) noexcept
{
    return jlimit(0.0f, 1.0f,
                  jmap(dB, kFloorDb, 0.0f, 0.0f, 1.0f));
}

float LevelMeter::linearToDb(float linear) noexcept
{
    return jmax(kFloorDb, 20.0f * std::log10(jmax(linear, 1.0e-6f)));
}

void LevelMeter::setLevel(float peakLinear, float rmsLinear)
{
    const float rmsDb  = linearToDb(rmsLinear);
    const float peakDbInstant = linearToDb(peakLinear);

    if (rmsDb >= levelDb)
        levelDb = rmsDb;
    else
        levelDb = jmax(rmsDb, levelDb - kBarDecayDbPerFrame);

    if (peakDbInstant >= peakDb)
    {
        peakDb         = peakDbInstant;
        peakHoldFrames = kPeakHoldDuration;
    }
    else if (peakHoldFrames > 0)
    {
        --peakHoldFrames;
    }
    else
    {
        peakDb = jmax(kFloorDb, peakDb - kPeakDecayDbPerFrame);
    }

    if (peakDbInstant >= kClipThresholdDb)
        clipFrames = kClipHoldFrames;
    else if (clipFrames > 0)
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

    const float fillW = w * dbToNorm(levelDb);

    if (fillW > 0.0f)
    {
        ColourGradient grad(c::silverDim.withAlpha(0.45f), 0.0f, 0.0f,
                            c::silverBright,                w, 0.0f, false);
        grad.addColour(dbToNorm(-6.0f), c::silverBright);
        grad.addColour(dbToNorm(-3.0f), c::danger);

        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds.withWidth(fillW), filo::theme::radius::small);
    }

    static const float ticksDb[] = { -48.0f, -36.0f, -24.0f, -12.0f, -6.0f, 0.0f };
    g.setColour(c::separator);
    const float tickH = h * 0.3f;
    for (float dB : ticksDb)
    {
        const float x = w * dbToNorm(dB);
        g.fillRect(x, 0.0f, 1.0f, tickH);
        g.fillRect(x, h - tickH, 1.0f, tickH);
    }

    const float peakX = w * dbToNorm(peakDb);
    if (peakX > 1.0f && peakDb > kFloorDb)
    {
        g.setColour(c::silverBright);
        g.fillRect(peakX - 1.0f, 0.0f, 2.0f, h);
    }

    g.setColour(clipFrames > 0 ? c::danger : c::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), filo::theme::radius::small,
                           clipFrames > 0 ? 1.5f : 1.0f);
}
