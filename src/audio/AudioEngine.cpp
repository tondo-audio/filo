#include "AudioEngine.h"

using namespace juce;

class LevelMeasurer : public AudioProcessor
{
public:
    LevelMeasurer(std::atomic<float>& peakOut, std::atomic<float>& rmsOut)
        : AudioProcessor(BusesProperties()
              .withInput("Input",   AudioChannelSet::stereo(), true)
              .withOutput("Output", AudioChannelSet::stereo(), true))
        , peakRef(peakOut)
        , rmsRef(rmsOut)
    {}

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        float peak = 0.0f;
        float rms  = 0.0f;
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        for (int ch = 0; ch < numCh; ++ch)
        {
            peak = jmax(peak, buffer.getMagnitude(ch, 0, numSamples));
            rms += buffer.getRMSLevel(ch, 0, numSamples);
        }
        if (numCh > 0)
            rms /= numCh;
        peakRef.store(peak);
        rmsRef .store(rms);
    }

    const String getName() const override            { return "LevelMeasurer"; }
    void prepareToPlay(double, int) override         {}
    void releaseResources() override                 {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    {
        return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
            && ! l.getMainInputChannelSet().isDisabled();
    }
    double getTailLengthSeconds() const override     { return 0.0; }
    bool acceptsMidi() const override                { return false; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    AudioProcessorEditor* createEditor() override    { return nullptr; }
    bool hasEditor() const override                  { return false; }
    int getNumPrograms() override                    { return 1; }
    int getCurrentProgram() override                 { return 0; }
    void setCurrentProgram(int) override             {}
    const String getProgramName(int) override        { return {}; }
    void changeProgramName(int, const String&) override {}
    void getStateInformation(MemoryBlock&) override  {}
    void setStateInformation(const void*, int) override {}

private:
    std::atomic<float>& peakRef;
    std::atomic<float>& rmsRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeasurer)
};

AudioEngine::AudioEngine()
{
    graph.enableAllBuses();

    using IOProc = AudioProcessorGraph::AudioGraphIOProcessor;

    auto inputNode = graph.addNode(
        std::make_unique<IOProc>(IOProc::audioInputNode));
    auto outputNode = graph.addNode(
        std::make_unique<IOProc>(IOProc::audioOutputNode));
    auto inMeasurer = graph.addNode(
        std::make_unique<LevelMeasurer>(inputPeak, inputRms));
    auto outMeasurer = graph.addNode(
        std::make_unique<LevelMeasurer>(outputPeak, outputRms));

    inputNodeID           = inputNode->nodeID;
    outputNodeID          = outputNode->nodeID;
    inputMeasurerNodeID   = inMeasurer->nodeID;
    outputMeasurerNodeID  = outMeasurer->nodeID;

    rebuildConnections();
}

AudioEngine::~AudioEngine()
{
    stop();
}

void AudioEngine::start(AudioDeviceManager& dm)
{
    deviceManager = &dm;
    player.setProcessor(&graph);
    dm.addAudioCallback(&player);
}

void AudioEngine::stop()
{
    if (deviceManager != nullptr)
    {
        deviceManager->removeAudioCallback(&player);
        deviceManager = nullptr;
    }
    player.setProcessor(nullptr);
}

void AudioEngine::addPlugin(std::unique_ptr<AudioPluginInstance> instance)
{
    instance->enableAllBuses();
    auto node = graph.addNode(std::move(instance));
    chain.push_back({ node->nodeID, false });
    rebuildConnections();
}

void AudioEngine::removePlugin(int index)
{
    if (index < 0 || index >= (int)chain.size()) return;
    graph.removeNode(chain[index].nodeID);
    chain.erase(chain.begin() + index);
    rebuildConnections();
}

void AudioEngine::movePlugin(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex) return;
    if (fromIndex < 0 || fromIndex >= (int)chain.size()) return;

    toIndex = juce::jlimit(0, (int)chain.size() - 1, toIndex);

    auto entry = chain[fromIndex];
    chain.erase(chain.begin() + fromIndex);
    chain.insert(chain.begin() + toIndex, entry);
    rebuildConnections();
}

void AudioEngine::setPluginBypassed(int index, bool bypassed)
{
    if (index < 0 || index >= (int)chain.size()) return;
    chain[index].bypassed = bypassed;
    rebuildConnections();
}

int AudioEngine::getNumPlugins() const
{
    return (int)chain.size();
}

AudioPluginInstance* AudioEngine::getPlugin(int index) const
{
    if (index < 0 || index >= (int)chain.size()) return nullptr;
    auto* node = graph.getNodeForId(chain[index].nodeID);
    if (node == nullptr) return nullptr;
    return dynamic_cast<AudioPluginInstance*>(node->getProcessor());
}

bool AudioEngine::isPluginBypassed(int index) const
{
    if (index < 0 || index >= (int)chain.size()) return false;
    return chain[index].bypassed;
}

void AudioEngine::rebuildConnections()
{
    for (auto& conn : graph.getConnections())
        graph.removeConnection(conn);

    std::vector<AudioProcessorGraph::NodeID> nodes;
    nodes.push_back(inputNodeID);
    nodes.push_back(inputMeasurerNodeID);
    for (auto& entry : chain)
        if (!entry.bypassed)
            nodes.push_back(entry.nodeID);
    nodes.push_back(outputMeasurerNodeID);
    nodes.push_back(outputNodeID);

    for (int i = 0; i < (int)nodes.size() - 1; ++i)
        for (int ch = 0; ch < 2; ++ch)
            graph.addConnection({ { nodes[i], ch }, { nodes[i + 1], ch } });
}
