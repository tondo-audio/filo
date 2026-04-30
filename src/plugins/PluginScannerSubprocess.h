#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

namespace filo
{
    // Stesso UID lato parent e worker — JUCE lo usa per riconoscere il flag IPC nel commandLine.
    inline constexpr const char* kScannerProcessUID = "tondo.host.scanner";

    // Lato child: avvia un worker che riceve richieste di scan e risponde con XML di PluginDescription.
    // Ritorna un handle non-null se il commandLine indica modalità worker (chiamare dopo START_JUCE_APPLICATION).
    // L'handle va mantenuto vivo finché l'app gira; quando il parent muore, il worker chiama JUCEApplicationBase::quit().
    class ScannerWorkerHandle
    {
    public:
        virtual ~ScannerWorkerHandle() = default;
    };

    std::unique_ptr<ScannerWorkerHandle> tryStartScannerWorker (const juce::String& commandLine);

    // Lato parent: si registra come CustomScanner sulla KnownPluginList.
    // Ogni richiesta di scan viene inoltrata a un sottoprocesso; se il sottoprocesso muore mentre carica un plugin,
    // findPluginTypesFor() ritorna false e JUCE blacklist'a quel path automaticamente.
    class OutOfProcessScanner final : public juce::KnownPluginList::CustomScanner
    {
    public:
        OutOfProcessScanner();
        ~OutOfProcessScanner() override;

        bool findPluginTypesFor (juce::AudioPluginFormat& format,
                                 juce::OwnedArray<juce::PluginDescription>& result,
                                 const juce::String& fileOrIdentifier) override;

        void scanFinished() override;

    private:
        class Superprocess;
        std::unique_ptr<Superprocess> superprocess;
    };
}
