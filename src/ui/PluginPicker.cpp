#include "PluginPicker.h"
#include "Theme.h"
#include "IconPaths.h"

#include <map>

using namespace juce;

namespace c  = filo::theme::colour;
namespace sp = filo::theme::spacing;

namespace
{
    constexpr int kRowHeight = 30;

    bool matchesFilter(const PluginDescription& d, const String& needle)
    {
        if (needle.isEmpty()) return true;
        return d.name.containsIgnoreCase(needle)
            || d.manufacturerName.containsIgnoreCase(needle)
            || d.pluginFormatName.containsIgnoreCase(needle);
    }

    String formatPill(const String& fmt)
    {
        if (fmt.startsWithIgnoreCase("AudioUnit")) return "AU";
        if (fmt.startsWithIgnoreCase("VST3"))      return "VST3";
        return fmt;
    }

    String manufacturerOf(const PluginDescription& d)
    {
        return d.manufacturerName.isNotEmpty() ? d.manufacturerName : String("Sconosciuto");
    }
}

PluginPicker::PluginPicker(const KnownPluginList& list, SelectionCallback selected)
    : source(list)
    , onSelected(std::move(selected))
{
    filterEditor.setTextToShowWhenEmpty("Cerca plugin...", c::textDisabled);
    filterEditor.setFont(filo::theme::makeFont(filo::theme::font::base));
    filterEditor.setColour(TextEditor::backgroundColourId,    c::surfaceSunken);
    filterEditor.setColour(TextEditor::textColourId,          c::textPrimary);
    filterEditor.setColour(TextEditor::outlineColourId,       c::outline);
    filterEditor.setColour(TextEditor::focusedOutlineColourId, c::outlineHover);
    filterEditor.setColour(CaretComponent::caretColourId,     c::silverBright);
    filterEditor.setIndents(10, 6);
    filterEditor.addListener(this);
    addAndMakeVisible(filterEditor);

    listBox.setModel(this);
    listBox.setRowHeight(kRowHeight);
    listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    listBox.setColour(ListBox::outlineColourId,    Colours::transparentBlack);
    listBox.getViewport()->setScrollBarThickness(6);
    addAndMakeVisible(listBox);

    emptyLabel.setText("Nessun plugin corrispondente", dontSendNotification);
    emptyLabel.setFont(filo::theme::makeFont(filo::theme::font::sm));
    emptyLabel.setColour(Label::textColourId, c::textDisabled);
    emptyLabel.setJustificationType(Justification::centred);
    addChildComponent(emptyLabel);

    rebuildEntries();
}

void PluginPicker::resized()
{
    const int pad = sp::s3;
    auto area = getLocalBounds().reduced(pad);

    filterEditor.setBounds(area.removeFromTop(32));
    area.removeFromTop(sp::s2);

    listBox.setBounds(area);
    emptyLabel.setBounds(area);
}

void PluginPicker::paint(Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(c::bgWindow);
    g.fillRoundedRectangle(bounds, filo::theme::radius::medium);
    g.setColour(c::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), filo::theme::radius::medium, 1.0f);
}

int PluginPicker::getNumRows()
{
    return (int) entries.size();
}

void PluginPicker::paintListBoxItem(int rowNumber, Graphics& g,
                                    int width, int height, bool rowIsSelected)
{
    if (! isPositiveAndBelow(rowNumber, (int) entries.size())) return;
    const auto& e = entries[(size_t) rowNumber];

    const Rectangle<float> bounds(0.0f, 0.0f, (float) width, (float) height);

    if (e.kind == RowKind::Header)
    {
        // chevron right (collapsed) / down (expanded)
        Path chev = filo::icons::chevronDown();
        if (! e.expanded)
        {
            // ruota -90 attorno al centro (12,12)
            chev.applyTransform(AffineTransform::rotation(-MathConstants<float>::halfPi,
                                                          12.0f, 12.0f));
        }

        const float iconBox = 24.0f;
        const float scale = (float) height * 0.45f / iconBox;
        const float iconX = (float) sp::s2;
        const float iconY = ((float) height - iconBox * scale) * 0.5f;
        chev.applyTransform(AffineTransform::scale(scale, scale).translated(iconX, iconY));

        g.setColour(c::silverDim);
        g.strokePath(chev, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));

        // manufacturer
        const int textX = sp::s2 + (int) (iconBox * scale) + sp::s2;
        g.setColour(c::silver);
        g.setFont(filo::theme::makeFont(filo::theme::font::xs, Font::plain, true));
        const String label = e.headerText.toUpperCase()
                              + "  (" + String(e.childCount) + ")";
        g.drawText(label, textX, 0, width - textX - sp::s2, height,
                   Justification::centredLeft, false);

        // bottom hairline
        g.setColour(c::separator);
        g.fillRect(0.0f, (float) height - 1.0f, (float) width, 1.0f);
        return;
    }

    // ── plugin row ──
    if (rowIsSelected)
    {
        g.setColour(c::surfaceElevated);
        g.fillRoundedRectangle(bounds.reduced(2.0f, 1.0f), filo::theme::radius::small);
    }

    constexpr int indentX = 28;
    const int padR  = sp::s3;
    const int pillW = 38;
    const int pillH = 16;

    const int nameMaxW = width - indentX - padR - pillW - sp::s2;

    g.setColour(c::textPrimary);
    g.setFont(filo::theme::makeFont(filo::theme::font::base));
    g.drawText(e.plugin.name, indentX, 0, nameMaxW, height,
               Justification::centredLeft, true);

    const Rectangle<float> pill((float) (width - padR - pillW),
                                ((float) height - pillH) * 0.5f,
                                (float) pillW, (float) pillH);
    g.setColour(c::outline);
    g.drawRoundedRectangle(pill, filo::theme::radius::pill, 1.0f);
    g.setColour(c::silverDim);
    g.setFont(filo::theme::makeFont(filo::theme::font::xs, Font::plain, true));
    g.drawText(formatPill(e.plugin.pluginFormatName),
               pill.toNearestInt(), Justification::centred, false);
}

void PluginPicker::listBoxItemClicked(int row, const MouseEvent&)
{
    if (! isPositiveAndBelow(row, (int) entries.size())) return;
    const auto& e = entries[(size_t) row];

    if (e.kind == RowKind::Header)
    {
        toggleManufacturer(e.headerText);
        return;
    }

    commitRow(row);
}

void PluginPicker::listBoxItemDoubleClicked(int row, const MouseEvent&)
{
    if (! isPositiveAndBelow(row, (int) entries.size())) return;
    const auto& e = entries[(size_t) row];

    if (e.kind == RowKind::Header)
    {
        toggleManufacturer(e.headerText);
        return;
    }

    commitRow(row);
}

void PluginPicker::returnKeyPressed(int lastRowSelected)
{
    commitRow(lastRowSelected);
}

void PluginPicker::commitRow(int row)
{
    if (! isPositiveAndBelow(row, (int) entries.size())) return;
    const auto& e = entries[(size_t) row];
    if (e.kind != RowKind::Plugin) return;

    if (onSelected)
        onSelected(e.plugin);

    dismiss();
}

void PluginPicker::toggleManufacturer(const String& manufacturer)
{
    auto it = expandedManufacturers.find(manufacturer);
    if (it == expandedManufacturers.end())
        expandedManufacturers.insert(manufacturer);
    else
        expandedManufacturers.erase(it);

    rebuildEntries();
}

void PluginPicker::textEditorTextChanged(TextEditor& ed)
{
    currentFilter = ed.getText().trim();
    rebuildEntries();
}

void PluginPicker::textEditorReturnKeyPressed(TextEditor&)
{
    selectFirstPlugin();

    const int sel = listBox.getSelectedRow();
    if (sel >= 0)
        commitRow(sel);
}

void PluginPicker::textEditorEscapeKeyPressed(TextEditor&)
{
    dismiss();
}

void PluginPicker::selectFirstPlugin()
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (entries[i].kind == RowKind::Plugin)
        {
            listBox.selectRow((int) i);
            return;
        }
    }
}

void PluginPicker::rebuildEntries()
{
    entries.clear();

    // Raggruppa tutti i tipi per manufacturer (alfabetici); il filter lo applichiamo
    // solo sui plugin (per decidere quali mostrare se gruppo espanso) e sul gruppo
    // (per auto-espanderlo quando matcha qualcosa).
    std::map<String, std::vector<PluginDescription>, std::less<>> groups;
    for (const auto& d : source.getTypes())
        groups[manufacturerOf(d)].push_back(d);

    for (auto& [_, list] : groups)
        std::sort(list.begin(), list.end(),
                  [](const PluginDescription& a, const PluginDescription& b)
                  { return a.name.compareIgnoreCase(b.name) < 0; });

    bool any = false;
    for (auto& [manufacturer, list] : groups)
    {
        // filtriamo i plugin del gruppo
        std::vector<PluginDescription> filtered;
        if (currentFilter.isEmpty())
        {
            filtered = list;
        }
        else
        {
            for (const auto& d : list)
                if (matchesFilter(d, currentFilter))
                    filtered.push_back(d);

            if (filtered.empty()) continue; // gruppo escluso del tutto
        }

        const bool manualExpanded = expandedManufacturers.count(manufacturer) > 0;
        const bool autoExpanded   = ! currentFilter.isEmpty();
        const bool expanded       = manualExpanded || autoExpanded;

        Entry header;
        header.kind        = RowKind::Header;
        header.headerText  = manufacturer;
        header.childCount  = (int) filtered.size();
        header.expanded    = expanded;
        entries.push_back(std::move(header));
        any = true;

        if (expanded)
        {
            for (auto& d : filtered)
            {
                Entry pe;
                pe.kind   = RowKind::Plugin;
                pe.plugin = d;
                entries.push_back(std::move(pe));
            }
        }
    }

    listBox.updateContent();
    emptyLabel.setVisible(! any);

    if (! currentFilter.isEmpty() && any)
        selectFirstPlugin();
}

void PluginPicker::dismiss()
{
    if (auto* box = findParentComponentOfClass<CallOutBox>())
        box->dismiss();
}
