#include "DeviceSelectorBar.h"
#include "Theme.h"

using namespace juce;

static const int kBufferSizes[] = { 64, 128, 256, 512, 1024 };

DeviceSelectorBar::DeviceSelectorBar(AudioDeviceManager& dm)
    : deviceManager(dm)
{
    auto setupLabel = [this](Label& lbl)
    {
        lbl.setFont(filo::theme::makeFont(filo::theme::font::xs, Font::plain, true));
        lbl.setColour(Label::textColourId, filo::theme::colour::textSecondary);
        lbl.setJustificationType(Justification::centredLeft);
        addAndMakeVisible(lbl);
    };

    setupLabel(inputLabel);
    setupLabel(outputLabel);
    setupLabel(bufferLabel);

    auto setupCombo = [this](ComboBox& box)
    {
        box.addListener(this);
        addAndMakeVisible(box);
    };

    setupCombo(inputCombo);
    setupCombo(outputCombo);
    setupCombo(bufferCombo);

    for (int i = 0; i < (int)std::size(kBufferSizes); ++i)
        bufferCombo.addItem(String(kBufferSizes[i]) + " samples", i + 1);

    populateDeviceLists();
    deviceManager.addChangeListener(this);
}

DeviceSelectorBar::~DeviceSelectorBar()
{
    deviceManager.removeChangeListener(this);
}

void DeviceSelectorBar::resized()
{
    const int rowH       = 28;
    const int rowGap     = 8;
    const int labelW     = 64;
    const int innerPad   = filo::theme::spacing::s3;
    const int totalW     = getWidth();
    const int comboW     = totalW - labelW - rowGap - innerPad * 2;

    int y = innerPad;
    inputLabel .setBounds(innerPad, y, labelW, rowH);
    inputCombo .setBounds(innerPad + labelW + rowGap, y, comboW, rowH);
    y += rowH + 6;
    outputLabel.setBounds(innerPad, y, labelW, rowH);
    outputCombo.setBounds(innerPad + labelW + rowGap, y, comboW, rowH);
    y += rowH + 6;
    bufferLabel.setBounds(innerPad, y, labelW, rowH);
    bufferCombo.setBounds(innerPad + labelW + rowGap, y, comboW, rowH);
}

void DeviceSelectorBar::paint(Graphics& g)
{
    namespace c = filo::theme::colour;
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(c::surface);
    g.fillRoundedRectangle(bounds, filo::theme::radius::medium);

    g.setColour(c::outline);
    g.drawRoundedRectangle(bounds, filo::theme::radius::medium, 1.0f);
}

void DeviceSelectorBar::populateDeviceLists()
{
    updating = true;

    auto* device   = deviceManager.getCurrentAudioDevice();
    const auto setup = deviceManager.getAudioDeviceSetup();

    StringArray inputNames;
    StringArray outputNames;

    for (auto* type : deviceManager.getAvailableDeviceTypes())
    {
        inputNames  = type->getDeviceNames(true);
        outputNames = type->getDeviceNames(false);
        break; // CoreAudio on macOS exposes one type with all devices
    }

    inputCombo.clear(dontSendNotification);
    for (int i = 0; i < inputNames.size(); ++i)
        inputCombo.addItem(inputNames[i], i + 1);

    outputCombo.clear(dontSendNotification);
    for (int i = 0; i < outputNames.size(); ++i)
        outputCombo.addItem(outputNames[i], i + 1);

    // Select current devices
    inputCombo .setText(setup.inputDeviceName,  dontSendNotification);
    outputCombo.setText(setup.outputDeviceName, dontSendNotification);

    // Select current buffer size
    const int currentBuf = (device != nullptr) ? device->getCurrentBufferSizeSamples() : 128;
    for (int i = 0; i < (int)std::size(kBufferSizes); ++i)
    {
        if (kBufferSizes[i] == currentBuf)
        {
            bufferCombo.setSelectedItemIndex(i, dontSendNotification);
            break;
        }
    }
    if (bufferCombo.getSelectedItemIndex() < 0)
        bufferCombo.setSelectedItemIndex(1, dontSendNotification); // default 128

    updating = false;
}

void DeviceSelectorBar::applySetup()
{
    if (updating) return;

    AudioDeviceManager::AudioDeviceSetup setup = deviceManager.getAudioDeviceSetup();

    const String inputName  = inputCombo.getText();
    const String outputName = outputCombo.getText();
    const int    bufIdx     = bufferCombo.getSelectedItemIndex();

    if (inputName.isNotEmpty())  setup.inputDeviceName  = inputName;
    if (outputName.isNotEmpty()) setup.outputDeviceName = outputName;
    if (bufIdx >= 0)             setup.bufferSize       = kBufferSizes[bufIdx];

    setup.useDefaultInputChannels  = true;
    setup.useDefaultOutputChannels = true;

    deviceManager.setAudioDeviceSetup(setup, true);
}

void DeviceSelectorBar::comboBoxChanged(ComboBox*)
{
    applySetup();
}

void DeviceSelectorBar::changeListenerCallback(ChangeBroadcaster*)
{
    populateDeviceLists();
}
