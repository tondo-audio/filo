#include "DeviceSelectorBar.h"

using namespace juce;

static const int kBufferSizes[] = { 64, 128, 256, 512, 1024 };

DeviceSelectorBar::DeviceSelectorBar(AudioDeviceManager& dm)
    : deviceManager(dm)
{
    auto setupLabel = [this](Label& lbl)
    {
        lbl.setFont(Font(11.0f, Font::bold));
        lbl.setColour(Label::textColourId, Colour(0xFF888888));
        addAndMakeVisible(lbl);
    };

    setupLabel(inputLabel);
    setupLabel(outputLabel);
    setupLabel(bufferLabel);

    auto setupCombo = [this](ComboBox& box)
    {
        box.setColour(ComboBox::backgroundColourId,   Colour(0xFF333333));
        box.setColour(ComboBox::textColourId,         Colour(0xFFEEEEEE));
        box.setColour(ComboBox::arrowColourId,        Colour(0xFF888888));
        box.setColour(ComboBox::outlineColourId,      Colour(0xFF404040));
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
    const int rowH   = 28;
    const int gap    = 8;
    const int labelW = 60;
    const int x      = 0;
    const int totalW = getWidth();
    const int comboW = totalW - labelW - gap;

    int y = 8;
    inputLabel .setBounds(x, y, labelW, rowH);
    inputCombo .setBounds(x + labelW + gap, y, comboW, rowH);
    y += rowH + 6;
    outputLabel.setBounds(x, y, labelW, rowH);
    outputCombo.setBounds(x + labelW + gap, y, comboW, rowH);
    y += rowH + 6;
    bufferLabel.setBounds(x, y, labelW, rowH);
    bufferCombo.setBounds(x + labelW + gap, y, comboW, rowH);
}

void DeviceSelectorBar::paint(Graphics& g)
{
    g.setColour(Colour(0xFF282828));
    g.fillRect(getLocalBounds());
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
