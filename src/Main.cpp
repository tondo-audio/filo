#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "MainWindow.h"
#include "audio/AudioEngine.h"
#include "plugins/PluginManager.h"
#include "plugins/PluginScannerSubprocess.h"

class FiloApplication : public juce::JUCEApplication
{
public:
    FiloApplication() = default;

    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String& commandLine) override
    {
        // Worker mode: questo processo è stato lanciato dal parent come scanner di un singolo plugin.
        // Resta vivo a servire richieste IPC; quit() avviene su connection lost (parent morto).
        if (auto worker = filo::tryStartScannerWorker(commandLine))
        {
            scannerWorker = std::move(worker);
            return;
        }

        normalModeInitialised = true;

        juce::PropertiesFile::Options opts;
        opts.applicationName     = "TondoHost";
        opts.filenameSuffix      = ".settings";
        opts.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(opts);

        pluginManager = std::make_unique<PluginManager>();
        pluginManager->loadFromProperties(*appProperties.getUserSettings());

        audioEngine = std::make_unique<AudioEngine>();

        juce::RuntimePermissions::request(
            juce::RuntimePermissions::recordAudio,
            [this](bool granted)
            {
                auto savedState = appProperties.getUserSettings()->getXmlValue("audioDevice");
                const juce::String err = deviceManager.initialise(
                    granted ? 2 : 0, 2, savedState.get(), true);

                if (err.isNotEmpty())
                    DBG("AudioDeviceManager init error: " + err);

                audioEngine->start(deviceManager);

                mainWindow = std::make_unique<MainWindow>(
                    deviceManager, *audioEngine, *pluginManager,
                    *appProperties.getUserSettings());
            });
    }

    void shutdown() override
    {
        // Worker mode: nient'altro da fare oltre a rilasciare l'handle.
        if (scannerWorker)
        {
            scannerWorker.reset();
            return;
        }

        // Se l'init normale non è mai partito (worker non riconosciuto + run loop chiuso prima),
        // non toccare appProperties né nessun altro stato non inizializzato.
        if (! normalModeInitialised)
            return;

        if (auto xml = deviceManager.createStateXml())
            appProperties.getUserSettings()->setValue("audioDevice", xml.get());

        if (pluginManager)
            pluginManager->saveToProperties(*appProperties.getUserSettings());

        appProperties.getUserSettings()->saveIfNeeded();

        mainWindow.reset();
        audioEngine.reset();
        pluginManager.reset();
    }

    void systemRequestedQuit() override { quit(); }

private:
    juce::ApplicationProperties appProperties;
    juce::AudioDeviceManager    deviceManager;

    std::unique_ptr<PluginManager> pluginManager;
    std::unique_ptr<AudioEngine>   audioEngine;
    std::unique_ptr<MainWindow>    mainWindow;

    std::unique_ptr<filo::ScannerWorkerHandle> scannerWorker;
    bool normalModeInitialised = false;
};

START_JUCE_APPLICATION(FiloApplication)
