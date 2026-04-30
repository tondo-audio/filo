#include "PluginManager.h"
#include "PluginScannerSubprocess.h"

using namespace juce;

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
                                   .getChildFile("Tondo Audio/Tondo Host");
    crashGuardDir.createDirectory();

    const int numFormats = formatManager.getNumFormats();

    for (int fi = 0; fi < numFormats; ++fi)
    {
        auto* format = formatManager.getFormat(fi);
        const FileSearchPath searchPath = getSearchPathsForFormat(format->getName());

        if (searchPath.getNumPaths() == 0) continue;

        // Pedal file kept as belt-and-braces: il custom scanner gestisce già il crash via blacklist,
        // ma se un giorno disabilitassimo l'OutOfProcessScanner questa rete sarebbe ancora utile.
        const File crashGuard = crashGuardDir.getChildFile(
            "crash_guard_" + format->getName() + ".txt");

        PluginDirectoryScanner scanner(knownPlugins, *format, searchPath, true, crashGuard);
        String currentPlugin;

        while (scanner.scanNextFile(true, currentPlugin))
            if (progress)
                progress(-1, currentPlugin);
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
