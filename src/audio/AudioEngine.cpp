#include "AudioEngine.h"

using namespace juce;

class LevelMeasurer : public AudioProcessor
{
public:
    LevelMeasurer(std::atomic<float>& peakOut,
                  std::atomic<float>& rmsOut,
                  AudioChannelSet busLayout = AudioChannelSet::stereo())
        : AudioProcessor(BusesProperties()
              .withInput("Input",   busLayout, true)
              .withOutput("Output", busLayout, true))
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
    graph.setChannelLayoutOfBus(true,  0, AudioChannelSet::stereo());
    graph.setChannelLayoutOfBus(false, 0, AudioChannelSet::stereo());

    using IOProc = AudioProcessorGraph::AudioGraphIOProcessor;

    auto inputNode = graph.addNode(
        std::make_unique<IOProc>(IOProc::audioInputNode));
    auto outputNode = graph.addNode(
        std::make_unique<IOProc>(IOProc::audioOutputNode));
    // Input measurer is mono: collapses stereo input to a single guitar-friendly
    // signal (sum L+R) before the plugin chain. Output measurer stays stereo.
    auto inMeasurer = graph.addNode(
        std::make_unique<LevelMeasurer>(inputPeak, inputRms, AudioChannelSet::mono()));
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
    auto node = graph.addNode(std::move(instance));
    chain.push_back({ node->nodeID });
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
    if (auto* node = graph.getNodeForId(chain[index].nodeID))
        node->setBypassed(bypassed);
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
    auto* node = graph.getNodeForId(chain[index].nodeID);
    return node != nullptr && node->isBypassed();
}

void AudioEngine::rebuildConnections()
{
    for (auto& conn : graph.getConnections())
        graph.removeConnection(conn);

    std::vector<AudioProcessorGraph::NodeID> nodes;
    nodes.push_back(inputNodeID);
    nodes.push_back(inputMeasurerNodeID);
    for (auto& entry : chain)
        nodes.push_back(entry.nodeID);
    nodes.push_back(outputMeasurerNodeID);
    nodes.push_back(outputNodeID);

    for (int i = 0; i < (int)nodes.size() - 1; ++i)
    {
        auto* a = graph.getNodeForId(nodes[i]);
        auto* b = graph.getNodeForId(nodes[i + 1]);
        if (a == nullptr || b == nullptr) continue;

        const int aOut = a->getProcessor()->getMainBusNumOutputChannels();
        const int bIn  = b->getProcessor()->getMainBusNumInputChannels();
        if (aOut == 0 || bIn == 0) continue;

        // Asymmetric channel routing: mono→stereo replicates, stereo→mono sums.
        const int n = jmax(aOut, bIn);
        for (int ch = 0; ch < n; ++ch)
            graph.addConnection({ { nodes[i],     ch % aOut },
                                  { nodes[i + 1], ch % bIn  } });
    }
}
