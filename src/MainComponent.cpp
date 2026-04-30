#include "MainComponent.h"
#include "audio/AudioEngine.h"
#include "plugins/PluginManager.h"

using namespace juce;

static const Colour kBg        { 0xFF1A1A1A };
static const Colour kSurface   { 0xFF282828 };
static const Colour kAccent    { 0xFFD4A017 };
static const Colour kText      { 0xFFEEEEEE };
static const Colour kTextDim   { 0xFF888888 };

MainComponent::MainComponent(AudioDeviceManager& dm,
                             AudioEngine& eng,
                             PluginManager& mgr,
                             PropertiesFile& p)
    : deviceManager(dm)
    , engine(eng)
    , pluginMgr(mgr)
    , props(p)
    , deviceBar(dm)
    , chainView(eng, mgr)
{
    setSize(420, 580);

    addAndMakeVisible(deviceBar);

    chainViewport.setViewedComponent(&chainView, false);
    chainViewport.setScrollBarsShown(true, false);
    chainViewport.setScrollBarThickness(6);
    addAndMakeVisible(chainViewport);

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    auto setupMeterLabel = [this](Label& lbl, const String& text)
    {
        lbl.setText(text, dontSendNotification);
        lbl.setFont(Font(10.0f, Font::bold));
        lbl.setColour(Label::textColourId, kTextDim);
        lbl.setJustificationType(Justification::centredRight);
        addAndMakeVisible(lbl);
    };
    setupMeterLabel(inputMeterLabel,  "INPUT");
    setupMeterLabel(outputMeterLabel, "OUTPUT");

    addPluginButton.setColour(TextButton::buttonColourId,  kAccent.darker(0.3f));
    addPluginButton.setColour(TextButton::textColourOffId, kText);
    addPluginButton.onClick = [this] { showAddPluginMenu(); };
    addAndMakeVisible(addPluginButton);

    rescanButton.setColour(TextButton::buttonColourId,  Colour(0xFF333333));
    rescanButton.setColour(TextButton::textColourOffId, kTextDim);
    rescanButton.onClick = [this] { startScan(); };
    addAndMakeVisible(rescanButton);

    statusLabel.setFont(Font(11.0f));
    statusLabel.setColour(Label::textColourId, kTextDim);
    addAndMakeVisible(statusLabel);

    // Auto-scan on first launch if plugin list is empty
    if (pluginMgr.getKnownPlugins().getNumTypes() == 0)
        startScan();

    chainView.refresh();
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(kBg);

    // Header
    g.setColour(kSurface);
    g.fillRect(0, 0, getWidth(), 42);

    g.setColour(kAccent);
    g.fillRect(0, 0, 3, 42);

    g.setColour(kText);
    g.setFont(Font(16.0f, Font::bold));
    g.drawText("Tondo Host", 14, 0, 200, 42, Justification::centredLeft, false);

    // Section separators
    g.setColour(Colour(0xFF333333));
    g.fillRect(0, 42 + 102 + 2, getWidth(), 1); // below device bar
    g.fillRect(0, getHeight() - 50, getWidth(), 1); // above meters
}

void MainComponent::resized()
{
    const int pad  = 12;
    const int w    = getWidth();

    // Header (42px)
    int y = 42;

    // Device bar
    deviceBar.setBounds(pad, y + 4, w - pad * 2, 96);
    y += 102;

    // CATENA toolbar
    const int toolbarH = 32;
    addPluginButton.setBounds(w - pad - 110, y + 4, 110, 24);
    rescanButton   .setBounds(w - pad - 226, y + 4, 108, 24);
    statusLabel    .setBounds(pad, y + 4, 180, 24);
    y += toolbarH;

    // Chain viewport (flexible height)
    const int meterAreaH = 50;
    const int chainH = getHeight() - y - meterAreaH - pad;
    chainViewport.setBounds(pad, y, w - pad * 2, chainH);
    chainView.setSize(w - pad * 2 - 8, chainView.getHeight());
    y += chainH + 4;

    // Meters
    const int labelW  = 50;
    const int meterH  = 14;
    const int meterW  = w - pad * 2 - labelW - 6;

    inputMeterLabel .setBounds(pad, y + 2,  labelW, meterH);
    inputMeter      .setBounds(pad + labelW + 6, y + 2,  meterW, meterH);
    y += meterH + 8;
    outputMeterLabel.setBounds(pad, y + 2, labelW, meterH);
    outputMeter     .setBounds(pad + labelW + 6, y + 2, meterW, meterH);
}

void MainComponent::timerCallback()
{
    inputMeter .setLevel(engine.getInputLevel());
    outputMeter.setLevel(engine.getOutputLevel());
}

void MainComponent::showAddPluginMenu()
{
    const auto& list = pluginMgr.getKnownPlugins();
    const int n = list.getNumTypes();

    if (n == 0)
    {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,
            "Nessun plugin trovato",
            "Clicca \"Ri-scansiona\" per cercare i plugin AU e VST3 installati.");
        return;
    }

    PopupMenu menu;
    for (int i = 0; i < n; ++i)
    {
        const auto& desc = list.getTypes()[i];
        menu.addItem(i + 1, desc.name + " (" + desc.pluginFormatName + ")");
    }

    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(addPluginButton),
        [this, &list](int result)
        {
            if (result <= 0) return;
            const auto desc = list.getTypes()[result - 1];

            auto* dev = deviceManager.getCurrentAudioDevice();
            const double sr = (dev != nullptr) ? dev->getCurrentSampleRate() : 44100.0;
            const int blockSize = (dev != nullptr) ? dev->getCurrentBufferSizeSamples() : 512;

            pluginMgr.loadPluginAsync(desc, sr, blockSize,
                [this](std::unique_ptr<AudioPluginInstance> instance, const String& error)
                {
                    if (instance == nullptr)
                    {
                        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                            "Errore caricamento plugin", error);
                        return;
                    }
                    engine.addPlugin(std::move(instance));
                    chainView.refresh();
                });
        });
}

void MainComponent::startScan()
{
    if (pluginMgr.isScanning()) return;

    rescanButton.setEnabled(false);
    statusLabel.setText("Scansione plugin in corso...", dontSendNotification);

    Thread::launch([this]
    {
        try
        {
            pluginMgr.scanPlugins([this](int /*percent*/, const String& name)
            {
                MessageManager::callAsync([this, name]
                {
                    statusLabel.setText(name.isNotEmpty() ? name : "Scansione...",
                                        dontSendNotification);
                });
            });
        }
        catch (...)
        {
            // Scan thread caught an unhandled exception; results may be partial.
        }

        MessageManager::callAsync([this]
        {
            pluginMgr.saveToProperties(props);
            rescanButton.setEnabled(true);
            statusLabel.setText(
                String(pluginMgr.getKnownPlugins().getNumTypes()) + " plugin trovati",
                dontSendNotification);
        });
    });
}
