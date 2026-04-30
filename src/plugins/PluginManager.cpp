#include "PluginManager.h"
#include "PluginScannerSubprocess.h"

#include <unordered_set>

using namespace juce;

namespace
{
    String normalizeKey(const String& s)
    {
        String r;
        for (auto c : s)
        {
            const auto lc = CharacterFunctions::toLowerCase(c);
            if (CharacterFunctions::isLetterOrDigit(lc))
                r += lc;
        }
        return r;
    }

    void rebuildKeySet(const KnownPluginList& list,
                       std::unordered_set<String>& out)
    {
        out.clear();
        for (const auto& d : list.getTypes())
        {
            const String n = normalizeKey(d.name);
            if (n.isNotEmpty()) out.insert(n);

            const String mn = normalizeKey(d.manufacturerName + d.name);
            if (mn.isNotEmpty()) out.insert(mn);
        }
    }
}

static FileSearchPath getSearchPathsForFormat(const String& formatName)
{
    FileSearchPath paths;
    const File homeDir = File::getSpecialLocation(File::userHomeDirectory);

    if (formatName == "AudioUnit")
    {
        paths.add(File("/Library/Audio/Plug-Ins/Components"));
        paths.add(homeDir.getChildFile("Library/Audio/Plug-Ins/Components"));
    }
    else if (formatName == "VST3")
    {
        paths.add(File("/Library/Audio/Plug-Ins/VST3"));
        paths.add(homeDir.getChildFile("Library/Audio/Plug-Ins/VST3"));
    }

    return paths;
}

PluginManager::PluginManager()
{
    formatManager.addDefaultFormats();

    // Lo scan vero avviene in un sottoprocesso: se un plugin crasha durante dlopen/init,
    // il worker muore, JUCE blacklist'a quel path e il main resta in piedi.
    knownPlugins.setCustomScanner(std::make_unique<filo::OutOfProcessScanner>());
}

void PluginManager::scanPlugins(std::function<void(int, String)> progress)
{
    scanning.store(true);

    const File crashGuardDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                                   .getChildFile("Tondo Audio/Filo");
    crashGuardDir.createDirectory();

    // Ordine fisso: AU prima (veloci, in-process), VST3 dopo (lenti, subprocess).
    // Cosi possiamo skippare i bundle VST3 il cui nome corrisponde a un AU
    // gia scansionato — la stragrande maggioranza dei vendor mantiene lo stesso
    // bundle name tra AU e VST3, evitando il caricamento del bundle VST3.
    std::vector<AudioPluginFormat*> orderedFormats;
    auto pickFormat = [&](const String& target) -> AudioPluginFormat*
    {
        for (int i = 0, n = formatManager.getNumFormats(); i < n; ++i)
            if (formatManager.getFormat(i)->getName() == target)
                return formatManager.getFormat(i);
        return nullptr;
    };
    if (auto* au = pickFormat("AudioUnit")) orderedFormats.push_back(au);
    if (auto* v3 = pickFormat("VST3"))      orderedFormats.push_back(v3);

    std::unordered_set<String> knownKeys;
    rebuildKeySet(knownPlugins, knownKeys);

    for (auto* format : orderedFormats)
    {
        const FileSearchPath searchPath = getSearchPathsForFormat(format->getName());
        if (searchPath.getNumPaths() == 0) continue;

        // Pedal file kept as belt-and-braces: il custom scanner gestisce gia il crash via blacklist,
        // ma se un giorno disabilitassimo l'OutOfProcessScanner questa rete sarebbe ancora utile.
        const File crashGuard = crashGuardDir.getChildFile(
            "crash_guard_" + format->getName() + ".txt");

        PluginDirectoryScanner scanner(knownPlugins, *format, searchPath, true, crashGuard);
        const bool dedupCrossFormat = (format->getName() == "VST3");

        while (true)
        {
            const String nextFile = scanner.getNextPluginFileThatWillBeScanned();
            String bundle;

            if (nextFile.isNotEmpty())
            {
                bundle = File(nextFile).getFileNameWithoutExtension();

                if (dedupCrossFormat)
                {
                    const String key = normalizeKey(bundle);
                    if (key.isNotEmpty() && knownKeys.count(key) > 0)
                    {
                        if (progress)
                            progress(-1, "Saltato (gia presente come AU): " + bundle);

                        if (! scanner.skipNextFile())
                            break;
                        continue;
                    }
                }

                // Annuncia il file PRIMA di iniziare la scansione (che per VST3 e' lenta).
                // Cosi l'overlay non resta sull'ultimo "Saltato: ..." finche il subprocess
                // non riporta indietro il nome del plugin.
                if (progress)
                    progress(-1, bundle);
            }

            String currentPlugin;
            if (! scanner.scanNextFile(true, currentPlugin))
                break;

            if (progress && currentPlugin.isNotEmpty() && currentPlugin != bundle)
                progress(-1, currentPlugin);
        }

        // Aggiorna il set per il formato successivo (anche se per ora dopo VST3 non
        // c'e' altro, lasciamolo: e' future-proof e ha costo trascurabile).
        rebuildKeySet(knownPlugins, knownKeys);
    }

    knownPlugins.scanFinished();

    if (progress)
        progress(100, {});

    scanning.store(false);
}

void PluginManager::loadPluginAsync(const PluginDescription& desc,
                                    double sampleRate,
                                    int blockSize,
                                    std::function<void(std::unique_ptr<AudioPluginInstance>, String)> callback)
{
    formatManager.createPluginInstanceAsync(
        desc, sampleRate, blockSize,
        [cb = std::move(callback)](std::unique_ptr<AudioPluginInstance> instance, const String& error)
        {
            cb(std::move(instance), error);
        });
}

void PluginManager::saveToProperties(PropertiesFile& props)
{
    if (auto xml = knownPlugins.createXml())
        props.setValue("knownPlugins", xml.get());
}

void PluginManager::loadFromProperties(PropertiesFile& props)
{
    if (auto xml = props.getXmlValue("knownPlugins"))
        knownPlugins.recreateFromXml(*xml);
}
