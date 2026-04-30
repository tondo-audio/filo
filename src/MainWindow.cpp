#include "MainWindow.h"
#include "MainComponent.h"
#include "ui/Theme.h"

MainWindow::MainWindow(juce::AudioDeviceManager& dm,
                       AudioEngine& engine,
                       PluginManager& pluginMgr,
                       juce::PropertiesFile& props)
    : DocumentWindow("Filo",
                     filo::theme::colour::bgWindow,
                     DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(dm, engine, pluginMgr, props), true);
    setResizable(true, false);
    setResizeLimits(380, 500, 1200, 2000);
    centreWithSize(460, 640);
    setVisible(true);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
