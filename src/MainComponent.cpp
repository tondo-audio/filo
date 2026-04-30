#include "MainComponent.h"
#include "audio/AudioEngine.h"
#include "plugins/PluginManager.h"
#include "ui/Theme.h"
#include "ui/IconPaths.h"
#include "ui/PluginPicker.h"

using namespace juce;

namespace c = filo::theme::colour;
namespace sp = filo::theme::spacing;

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
    addAndMakeVisible(logo);
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
        lbl.setFont(filo::theme::makeFont(filo::theme::font::xs, Font::plain, true));
        lbl.setColour(Label::textColourId, c::textSecondary);
        lbl.setJustificationType(Justification::centredLeft);
        addAndMakeVisible(lbl);
    };
    setupMeterLabel(inputMeterLabel,  "INPUT");
    setupMeterLabel(outputMeterLabel, "OUTPUT");

    addPluginButton.setLabel("Aggiungi");
    addPluginButton.setPath(filo::icons::plus());
    addPluginButton.onClick = [this] { showAddPluginMenu(); };
    addAndMakeVisible(addPluginButton);

    rescanButton.setLabel("Ri-scansiona");
    rescanButton.setPath(filo::icons::refresh());
    rescanButton.onClick = [this] { startScan(); };
    addAndMakeVisible(rescanButton);

    statusLabel.setFont(filo::theme::makeFont(filo::theme::font::xs));
    statusLabel.setColour(Label::textColourId, c::textSecondary);
    statusLabel.setJustificationType(Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    addChildComponent(scanOverlay);

    chainView.refresh();
    startTimerHz(30);

    // posticipa lo scan all'avvio finché il MainComponent è stato dimensionato,
    // cosi l'overlay copre l'intera finestra e non parte a 0x0.
    if (pluginMgr.getKnownPlugins().getNumTypes() == 0)
        MessageManager::callAsync([this] { startScan(); });
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(c::bgBase);

    constexpr int headerH = 48;
    g.setColour(c::bgWindow);
    g.fillRect(0, 0, getWidth(), headerH);

    g.setColour(c::separator);
    g.fillRect(0, headerH,     getWidth(), 1);
    g.setColour(c::silverDim.withAlpha(0.18f));
    g.fillRect(0, headerH + 1, getWidth(), 1);

    constexpr int statusBarH = 22;
    const int statusY = getHeight() - statusBarH;
    g.setColour(c::separator);
    g.fillRect(0, statusY, getWidth(), 1);
}

void MainComponent::resized()
{
    constexpr int headerH    = 48;
    constexpr int toolbarH   = 36;
    constexpr int meterAreaH = 64;
    constexpr int statusBarH = 22;

    const int pad  = sp::s4;
    const int w    = getWidth();

    logo.setBounds(pad, 8, 140, headerH - 12);

    int y = headerH + sp::s3;

    constexpr int deviceBarH = 108;
    deviceBar.setBounds(pad, y, w - pad * 2, deviceBarH);
    y += deviceBarH + sp::s3;

    const int btnW = 130;
    addPluginButton.setBounds(w - pad - btnW, y, btnW, toolbarH - 4);
    rescanButton.setBounds(w - pad - btnW * 2 - sp::s2, y, btnW, toolbarH - 4);
    y += toolbarH;

    const int chainBottom = getHeight() - meterAreaH - statusBarH - sp::s2;
    const int chainH = chainBottom - y;
    chainViewport.setBounds(pad, y, w - pad * 2, jmax(chainH, 40));
    chainView.setSize(w - pad * 2 - 8, chainView.getHeight());
    y = chainBottom + sp::s2;

    constexpr int meterH    = 12;
    constexpr int labelH    = 12;
    const int meterX        = pad;
    const int meterW        = w - pad * 2;

    inputMeterLabel.setBounds(meterX, y, meterW, labelH);
    inputMeter     .setBounds(meterX, y + labelH + 2, meterW, meterH);
    y += labelH + meterH + 6;
    outputMeterLabel.setBounds(meterX, y, meterW, labelH);
    outputMeter     .setBounds(meterX, y + labelH + 2, meterW, meterH);

    statusLabel.setBounds(pad, getHeight() - statusBarH + 2,
                          w - pad * 2, statusBarH - 4);

    scanOverlay.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
    inputMeter .setLevel(engine.getInputPeak(),  engine.getInputRms());
    outputMeter.setLevel(engine.getOutputPeak(), engine.getOutputRms());
}

void MainComponent::showAddPluginMenu()
{
    const auto& list = pluginMgr.getKnownPlugins();

    if (list.getNumTypes() == 0)
    {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,
            "Nessun plugin trovato",
            "Clicca \"Ri-scansiona\" per cercare i plugin AU e VST3 installati.");
        return;
    }

    auto picker = std::make_unique<PluginPicker>(list,
        [this](const PluginDescription& desc)
        {
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

    picker->setSize(420, 460);

    const auto target = addPluginButton.getScreenBounds();
    CallOutBox::launchAsynchronously(std::move(picker), target, nullptr);
}

void MainComponent::startScan()
{
    if (pluginMgr.isScanning()) return;

    // blocca l'intera UI per evitare race con la chain audio durante lo scan
    addPluginButton.setEnabled(false);
    rescanButton.setEnabled(false);
    deviceBar.setEnabled(false);
    chainViewport.setEnabled(false);
    scanOverlay.setActive(true);

    statusLabel.setText("Scansione plugin in corso...", dontSendNotification);

    Thread::launch([this]
    {
        try
        {
            pluginMgr.scanPlugins([this](int /*percent*/, const String& name)
            {
                MessageManager::callAsync([this, name]
                {
                    scanOverlay.setCurrentItem(name);
                    statusLabel.setText(name.isNotEmpty() ? name : "Scansione...",
                                        dontSendNotification);
                });
            });
        }
        catch (...)
        {
        }

        MessageManager::callAsync([this]
        {
            pluginMgr.saveToProperties(props);

            scanOverlay.setActive(false);
            addPluginButton.setEnabled(true);
            rescanButton.setEnabled(true);
            deviceBar.setEnabled(true);
            chainViewport.setEnabled(true);

            statusLabel.setText(
                String(pluginMgr.getKnownPlugins().getNumTypes()) + " plugin trovati",
                dontSendNotification);
        });
    });
}
