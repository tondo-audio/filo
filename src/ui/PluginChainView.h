#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "IconButton.h"

class AudioEngine;
class PluginManager;

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor& processor, int chainIndex);
    ~PluginWindow() override;
    void closeButtonPressed() override;

    const int chainIndex;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};

class PluginRowComponent : public juce::Component
{
public:
    PluginRowComponent(int index,
                       const juce::String& name,
                       bool bypassed,
                       AudioEngine& engine,
                       PluginManager& pluginMgr,
                       std::function<void()> onChanged);
    ~PluginRowComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void openPluginWindow();
    void closePluginWindow();

    int getChainIndex() const { return chainIndex; }

private:
    int chainIndex;
    juce::String pluginName;
    bool bypassed;
    AudioEngine& engine;
    PluginManager& pluginMgr;
    std::function<void()> onChanged;

    IconButton uiButton;
    IconButton bypassBtn;
    IconButton removeButton;

    std::unique_ptr<PluginWindow> pluginWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginRowComponent)
};

class PluginChainView : public juce::Component,
                        public juce::DragAndDropTarget
{
public:
    PluginChainView(AudioEngine& engine, PluginManager& pluginMgr);

    void refresh();

    void resized() override;
    void paint(juce::Graphics& g) override;

    // DragAndDropTarget
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

    static constexpr int kRowHeight = 52;

private:
    int getDropIndex(int localY) const;

    AudioEngine&  engine;
    PluginManager& pluginMgr;
    std::vector<std::unique_ptr<PluginRowComponent>> rows;

    int dragOverIndex { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainView)
};
