#include "MainWindow.h"
#include "MainComponent.h"

MainWindow::MainWindow(juce::AudioDeviceManager& dm,
                       AudioEngine& engine,
                       PluginManager& pluginMgr,
                       juce::PropertiesFile& props)
    : DocumentWindow("Tondo Host",
                     juce::Colour(0xFF1A1A1A),
                     DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(dm, engine, pluginMgr, props), true);
    setResizable(false, false);
    centreWithSize(420, 580);
    setVisible(true);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
