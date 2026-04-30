#include "LogoView.h"
#include "Theme.h"

#if FILO_HAS_LOGO
 #include "FiloAssets.h"
#endif

using namespace juce;

LogoView::LogoView()
{
   #if FILO_HAS_LOGO
    svgDrawable = Drawable::createFromImageData(filo::assets::logo_svg,
                                                filo::assets::logo_svgSize);
   #endif
}

void LogoView::paint(Graphics& g)
{
    namespace c = filo::theme::colour;

    if (svgDrawable != nullptr)
    {
        svgDrawable->drawWithin(g, getLocalBounds().toFloat(),
                                RectanglePlacement::xLeft | RectanglePlacement::yMid,
                                1.0f);
        return;
    }

    auto bounds = getLocalBounds();

    Font logoFont = filo::theme::makeFont(filo::theme::font::display, Font::bold, true);
    logoFont.setExtraKerningFactor(0.22f);

    g.setColour(c::silverBright);
    g.setFont(logoFont);
    g.drawText("FILO",
               bounds.removeFromTop(bounds.getHeight() - 12),
               Justification::centredLeft, false);

    g.setColour(c::textSecondary);
    g.setFont(filo::theme::makeFont(filo::theme::font::xs, Font::plain, true));
    g.drawText(JUCEApplication::getInstance()->getApplicationVersion(),
               bounds, Justification::topLeft, false);
}
