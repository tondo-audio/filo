#include "PluginChainView.h"
#include "audio/AudioEngine.h"
#include "plugins/PluginManager.h"

using namespace juce;

// ─── PluginWindow ────────────────────────────────────────────────────────────

PluginWindow::PluginWindow(AudioProcessor& processor, int idx)
    : DocumentWindow(processor.getName(),
                     Colour(0xFF282828),
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
    uiButton.setColour(TextButton::buttonColourId,    Colour(0xFF444444));
    uiButton.setColour(TextButton::textColourOffId,   Colour(0xFFCCCCCC));
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

    bypassBtn.setButtonText("bypass");
    bypassBtn.setToggleState(bypassed, dontSendNotification);
    bypassBtn.setColour(ToggleButton::textColourId,          Colour(0xFF888888));
    bypassBtn.setColour(ToggleButton::tickColourId,          Colour(0xFFFFCC00));
    bypassBtn.setColour(ToggleButton::tickDisabledColourId,  Colour(0xFF555555));
    bypassBtn.onStateChange = [this]
    {
        engine.setPluginBypassed(chainIndex, bypassBtn.getToggleState());
        // Defer to avoid refreshing (and deleting) this component from within its own callback
        MessageManager::callAsync(onChanged);
    };
    addAndMakeVisible(bypassBtn);

    removeButton.setColour(TextButton::buttonColourId,  Colour(0xFF442222));
    removeButton.setColour(TextButton::textColourOffId, Colour(0xFFFF6666));
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
    const int pad  = 6;
    const int btnH = h - pad * 2;
    int x = getWidth();

    x -= (30 + pad);
    removeButton.setBounds(x, pad, 30, btnH);

    x -= (52 + pad);
    bypassBtn.setBounds(x, pad, 52, btnH);

    x -= (36 + pad);
    uiButton.setBounds(x, pad, 36, btnH);
}

void PluginRowComponent::paint(Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour(bypassed ? Colour(0xFF252525) : Colour(0xFF2E2E2E));
    g.fillRoundedRectangle(bounds.reduced(2.0f, 1.0f), 4.0f);

    // drag handle
    g.setColour(Colour(0xFF555555));
    for (int i = 0; i < 3; ++i)
        g.fillEllipse(8.0f, (float)(getHeight() / 2 - 6 + i * 6), 4.0f, 4.0f);

    // plugin name
    g.setColour(bypassed ? Colour(0xFF666666) : Colour(0xFFEEEEEE));
    g.setFont(Font(14.0f));
    g.drawText(pluginName, 22, 0, getWidth() - 160, getHeight(),
               Justification::centredLeft, true);
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
    g.fillAll(Colour(0xFF1E1E1E));

    if (rows.empty())
    {
        g.setColour(Colour(0xFF444444));
        g.setFont(Font(13.0f));
        g.drawText("Nessun plugin. Clicca \"+ Aggiungi\".",
                   getLocalBounds(), Justification::centred, false);
    }

    // drop indicator line
    if (dragOverIndex >= 0)
    {
        const int lineY = dragOverIndex * kRowHeight;
        g.setColour(Colour(0xFFD4A017));
        g.fillRect(4, lineY - 1, getWidth() - 8, 2);
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
