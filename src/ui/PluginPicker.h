#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <set>

class PluginPicker : public juce::Component,
                     private juce::ListBoxModel,
                     private juce::TextEditor::Listener
{
public:
    using SelectionCallback = std::function<void(const juce::PluginDescription&)>;

    PluginPicker(const juce::KnownPluginList& list, SelectionCallback onSelected);
    ~PluginPicker() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    enum class RowKind { Header, Plugin };

    struct Entry
    {
        RowKind kind;
        juce::String headerText;       // manufacturer (per header)
        int childCount { 0 };          // numero plugin (per header)
        bool expanded  { false };      // stato visivo (per header)
        juce::PluginDescription plugin;
    };

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void returnKeyPressed(int lastRowSelected) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>&) override { return {}; }

    void textEditorTextChanged(juce::TextEditor&) override;
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;

    void rebuildEntries();
    void selectFirstPlugin();
    void commitRow(int row);
    void toggleManufacturer(const juce::String& manufacturer);
    void dismiss();

    const juce::KnownPluginList& source;
    SelectionCallback onSelected;

    juce::TextEditor filterEditor;
    juce::ListBox    listBox;
    juce::Label      emptyLabel;

    juce::String currentFilter;
    std::set<juce::String> expandedManufacturers;
    std::vector<Entry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPicker)
};
