#include "PluginChainView.h"
#include "audio/AudioEngine.h"
#include "plugins/PluginManager.h"
#include "Theme.h"
#include "IconPaths.h"

using namespace juce;

// ─── PluginWindow ────────────────────────────────────────────────────────────

PluginWindow::PluginWindow(AudioProcessor& processor, int idx)
    : DocumentWindow(processor.getName(),
                     filo::theme::colour::bgWindow,
                     DocumentWindow::closeButton)
    , chainIndex(idx)
{
    if (auto* editor = processor.createEditorIfNeeded())
    {
        setContentOwned(editor, true);
        setResizable(true, false);
        setUsingNativeTitleBar(true);
        centreWithSize(editor->getWidth(), editor->getHeight());
        setVisible(true);
    }
}

PluginWindow::~PluginWindow() = default;

void PluginWindow::closeButtonPressed()
{
    setVisible(false);
}

// ─── PluginRowComponent ───────────────────────────────────────────────────────

PluginRowComponent::PluginRowComponent(int index,
                                       const String& name,
                                       bool isBypassed,
                                       AudioEngine& eng,
                                       PluginManager& mgr,
                                       std::function<void()> changed)
    : chainIndex(index)
    , pluginName(name)
    , bypassed(isBypassed)
    , engine(eng)
    , pluginMgr(mgr)
    , onChanged(std::move(changed))
{
    uiButton.setPath(filo::icons::monitor());
    uiButton.onClick = [this]
    {
        if (pluginWindow && pluginWindow->isVisible())
        {
            pluginWindow->toFront(true);
            return;
        }
        if (auto* plug = engine.getPlugin(chainIndex))
        {
            pluginWindow = std::make_unique<PluginWindow>(*plug, chainIndex);
        }
    };
    addAndMakeVisible(uiButton);

    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setPath(filo::icons::power(false));
    bypassBtn.setPathToggled(filo::icons::power(true));
    bypassBtn.setToggleState(bypassed, dontSendNotification);
    bypassBtn.onStateChange = [this]
    {
        engine.setPluginBypassed(chainIndex, bypassBtn.getToggleState());
        MessageManager::callAsync(onChanged);
    };
    addAndMakeVisible(bypassBtn);

    removeButton.setPath(filo::icons::cross());
    removeButton.setDangerColour(true);
    removeButton.onClick = [this]
    {
        engine.removePlugin(chainIndex);
        MessageManager::callAsync(onChanged);
    };
    addAndMakeVisible(removeButton);
}

PluginRowComponent::~PluginRowComponent() = default;

void PluginRowComponent::resized()
{
    const int h    = getHeight();
    const int pad  = filo::theme::spacing::s2;
    const int gap  = filo::theme::spacing::s1;
    const int btnSize = jmin(h - pad * 2, 32);
    int x = getWidth() - pad;

    x -= btnSize;
    removeButton.setBounds(x, (h - btnSize) / 2, btnSize, btnSize);
    x -= gap;

    x -= btnSize;
    bypassBtn.setBounds(x, (h - btnSize) / 2, btnSize, btnSize);
    x -= gap;

    x -= btnSize;
    uiButton.setBounds(x, (h - btnSize) / 2, btnSize, btnSize);
}

void PluginRowComponent::paint(Graphics& g)
{
    namespace c = filo::theme::colour;

    const auto bounds = getLocalBounds().toFloat().reduced(2.0f, 2.0f);

    g.setColour(bypassed ? c::surface : c::surfaceElevated);
    g.fillRoundedRectangle(bounds, filo::theme::radius::medium);

    g.setColour(c::outline.withAlpha(bypassed ? 0.4f : 0.7f));
    g.drawRoundedRectangle(bounds, filo::theme::radius::medium, 1.0f);

    // drag handle (3 burger lines)
    {
        const float cx = 16.0f;
        const float cy = (float) getHeight() * 0.5f;
        Path handle;
        for (int i = 0; i < 3; ++i)
        {
            const float y = cy - 5.0f + (float) i * 5.0f;
            handle.startNewSubPath(cx - 6.0f, y);
            handle.lineTo(cx + 6.0f, y);
        }
        g.setColour(c::silverDim);
        g.strokePath(handle, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // plugin name
    g.setColour(bypassed ? c::textDisabled : c::textPrimary);
    g.setFont(filo::theme::makeFont(filo::theme::font::base));
    g.drawText(pluginName, 32, 0, getWidth() - 32 - 130, getHeight(),
               Justification::centredLeft, true);

    // bypassed: diagonal slash
    if (bypassed)
    {
        g.setColour(c::silverDim.withAlpha(0.4f));
        const Line<float> line { bounds.getX() + 6.0f, bounds.getBottom() - 6.0f,
                                 bounds.getRight() - 6.0f, bounds.getY() + 6.0f };
        g.drawLine(line, 1.0f);
    }
}

void PluginRowComponent::mouseDrag(const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 8)
    {
        if (auto* container = DragAndDropContainer::findParentDragContainerFor(this))
            container->startDragging("plugin-row:" + String(chainIndex), this);
    }
}

void PluginRowComponent::openPluginWindow()
{
    if (auto* plug = engine.getPlugin(chainIndex))
        pluginWindow = std::make_unique<PluginWindow>(*plug, chainIndex);
}

void PluginRowComponent::closePluginWindow()
{
    pluginWindow.reset();
}

// ─── PluginChainView ─────────────────────────────────────────────────────────

PluginChainView::PluginChainView(AudioEngine& eng, PluginManager& mgr)
    : engine(eng), pluginMgr(mgr)
{
}

void PluginChainView::refresh()
{
    rows.clear();

    const int n = engine.getNumPlugins();
    for (int i = 0; i < n; ++i)
    {
        auto* plug   = engine.getPlugin(i);
        const String name = (plug != nullptr) ? plug->getName() : "Unknown";
        const bool   bypassed = engine.isPluginBypassed(i);

        auto row = std::make_unique<PluginRowComponent>(
            i, name, bypassed, engine, pluginMgr, [this] { refresh(); });

        addAndMakeVisible(*row);
        rows.push_back(std::move(row));
    }

    const int totalH = (int)rows.size() * kRowHeight;
    setSize(getWidth(), jmax(totalH, 10));
    resized();
    repaint();
}

void PluginChainView::resized()
{
    for (int i = 0; i < (int)rows.size(); ++i)
        rows[i]->setBounds(0, i * kRowHeight, getWidth(), kRowHeight);
}

void PluginChainView::paint(Graphics& g)
{
    namespace c = filo::theme::colour;

    if (rows.empty())
    {
        g.setColour(c::textDisabled);
        g.setFont(filo::theme::makeFont(filo::theme::font::sm));
        g.drawText("Nessun plugin. Clicca \"+ Aggiungi\".",
                   getLocalBounds(), Justification::centred, false);
    }

    // drop indicator
    if (dragOverIndex >= 0)
    {
        const int lineY = dragOverIndex * kRowHeight;
        g.setColour(c::silverBright);
        g.fillRect(8, lineY - 1, getWidth() - 16, 2);

        // small triangle markers at sides
        Path tri;
        tri.startNewSubPath(2.0f, (float) lineY - 4.0f);
        tri.lineTo(8.0f, (float) lineY);
        tri.lineTo(2.0f, (float) lineY + 4.0f);
        tri.closeSubPath();

        const float rightX = (float) getWidth() - 8.0f;
        Path triR;
        triR.startNewSubPath((float) getWidth() - 2.0f, (float) lineY - 4.0f);
        triR.lineTo(rightX, (float) lineY);
        triR.lineTo((float) getWidth() - 2.0f, (float) lineY + 4.0f);
        triR.closeSubPath();

        g.fillPath(tri);
        g.fillPath(triR);
    }
}

bool PluginChainView::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.toString().startsWith("plugin-row:");
}

void PluginChainView::itemDragEnter(const SourceDetails& details)
{
    dragOverIndex = getDropIndex(details.localPosition.y);
    repaint();
}

void PluginChainView::itemDragMove(const SourceDetails& details)
{
    dragOverIndex = getDropIndex(details.localPosition.y);
    repaint();
}

void PluginChainView::itemDragExit(const SourceDetails&)
{
    dragOverIndex = -1;
    repaint();
}

void PluginChainView::itemDropped(const SourceDetails& details)
{
    dragOverIndex = -1;

    const String desc = details.description.toString();
    const int fromIndex = desc.fromFirstOccurrenceOf(":", false, false).getIntValue();
    int toIndex = getDropIndex(details.localPosition.y);

    if (fromIndex >= 0 && fromIndex != toIndex)
    {
        engine.movePlugin(fromIndex, toIndex);
        refresh();
    }
    repaint();
}

int PluginChainView::getDropIndex(int localY) const
{
    const int idx = localY / kRowHeight;
    return jlimit(0, (int)rows.size(), idx);
}
