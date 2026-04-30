#include "PluginScannerSubprocess.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

#if JUCE_MAC
 #include <CoreFoundation/CoreFoundation.h>
 #include <mach/mach.h>
 #include <pthread.h>
 #include <signal.h>
 #include <unistd.h>
#endif

using namespace juce;

namespace
{
   #if JUCE_MAC
    // Thread dedicato che riceve qualsiasi mach exception per il task e termina silenziosamente.
    // Intercettando l'eccezione PRIMA che arrivi al ReportCrash, evitiamo sia il dialog di sistema
    // sia il file .ips che macOS genererebbe per ogni worker che muore caricando un plugin rotto.
    mach_port_t g_exceptionPort = MACH_PORT_NULL;

    void* machExceptionRecvThread (void*)
    {
        struct {
            mach_msg_header_t head;
            char body[1024];
        } msg{};

        for (;;)
        {
            const kern_return_t kr = mach_msg (&msg.head,
                                               MACH_RCV_MSG,
                                               0,
                                               sizeof (msg),
                                               g_exceptionPort,
                                               MACH_MSG_TIMEOUT_NONE,
                                               MACH_PORT_NULL);
            if (kr == KERN_SUCCESS)
                _exit (1);
        }
    }

    void silentSignalHandler (int) { _exit (1); }

    void disableCrashReporterForWorker()
    {
        if (mach_port_allocate (mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &g_exceptionPort) != KERN_SUCCESS)
            return;

        if (mach_port_insert_right (mach_task_self(), g_exceptionPort, g_exceptionPort,
                                    MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS)
            return;

        const exception_mask_t mask =
              EXC_MASK_BAD_ACCESS
            | EXC_MASK_BAD_INSTRUCTION
            | EXC_MASK_ARITHMETIC
            | EXC_MASK_EMULATION
            | EXC_MASK_BREAKPOINT;

        task_set_exception_ports (mach_task_self(),
                                  mask,
                                  g_exceptionPort,
                                  EXCEPTION_DEFAULT,
                                  THREAD_STATE_NONE);

        pthread_t thr;
        pthread_create (&thr, nullptr, machExceptionRecvThread, nullptr);
        pthread_detach (thr);

        // Cintura+bretelle: se per qualche motivo il signal arriva via BSD path comunque, exit silenzioso.
        struct sigaction sa{};
        sa.sa_handler = silentSignalHandler;
        sigemptyset (&sa.sa_mask);
        sa.sa_flags = SA_RESETHAND;
        sigaction (SIGSEGV, &sa, nullptr);
        sigaction (SIGBUS,  &sa, nullptr);
        sigaction (SIGILL,  &sa, nullptr);
        sigaction (SIGFPE,  &sa, nullptr);
        sigaction (SIGABRT, &sa, nullptr);
    }
   #else
    void disableCrashReporterForWorker() {}
   #endif
}

namespace filo
{
    //==============================================================================
    // Worker side: scansiona un singolo plugin in-process e manda back l'XML.
    // Si auto-termina (JUCEApplicationBase::quit) appena la connessione col parent si perde.
    class ScannerWorker final : public ScannerWorkerHandle,
                                private ChildProcessWorker,
                                private AsyncUpdater
    {
    public:
        ScannerWorker()
        {
            formatManager.addDefaultFormats();
        }

        bool tryInitialise (const String& commandLine)
        {
            return initialiseFromCommandLine (commandLine, kScannerProcessUID);
        }

    private:
        void handleMessageFromCoordinator (const MemoryBlock& mb) override
        {
            if (mb.isEmpty())
                return;

            const std::lock_guard<std::mutex> lock (mutex);

            // Per i formati che richiedono il main thread, accodiamo via AsyncUpdater
            // (handleMessageFromCoordinator viene invocato da un thread di lettura IPC).
            if (auto results = doScan (mb); ! results.isEmpty())
                sendResults (results);
            else
            {
                pendingBlocks.emplace (mb);
                triggerAsyncUpdate();
            }
        }

        void handleConnectionLost() override
        {
            JUCEApplicationBase::quit();
        }

        void handleAsyncUpdate() override
        {
            for (;;)
            {
                const std::lock_guard<std::mutex> lock (mutex);
                if (pendingBlocks.empty())
                    return;

                sendResults (doScan (pendingBlocks.front()));
                pendingBlocks.pop();
            }
        }

        OwnedArray<PluginDescription> doScan (const MemoryBlock& block)
        {
            MemoryInputStream stream (block, false);
            const auto formatName = stream.readString();
            const auto identifier = stream.readString();

            PluginDescription pd;
            pd.fileOrIdentifier = identifier;
            pd.uniqueId = pd.deprecatedUid = 0;

            AudioPluginFormat* matchingFormat = nullptr;
            for (auto* f : formatManager.getFormats())
                if (f->getName() == formatName)
                    matchingFormat = f;

            OwnedArray<PluginDescription> results;

            if (matchingFormat != nullptr
                && (MessageManager::getInstance()->isThisTheMessageThread()
                    || matchingFormat->requiresUnblockedMessageThreadDuringCreation (pd)))
            {
                matchingFormat->findAllTypesForFile (results, identifier);
            }

            return results;
        }

        void sendResults (const OwnedArray<PluginDescription>& results)
        {
            XmlElement xml ("LIST");
            for (const auto& desc : results)
                xml.addChildElement (desc->createXml().release());

            const auto str = xml.toString();
            sendMessageToCoordinator ({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
        }

        std::mutex mutex;
        std::queue<MemoryBlock> pendingBlocks;
        AudioPluginFormatManager formatManager;
    };

    std::unique_ptr<ScannerWorkerHandle> tryStartScannerWorker (const String& commandLine)
    {
        auto worker = std::make_unique<ScannerWorker>();
        if (! worker->tryInitialise (commandLine))
            return nullptr;

        disableCrashReporterForWorker();
        return worker;
    }

    //==============================================================================
    // Parent side: lancia il worker, gli manda il path da scansionare, attende risposta o crash.
    class OutOfProcessScanner::Superprocess final : private ChildProcessCoordinator
    {
    public:
        Superprocess()
        {
            launchWorkerProcess (File::getSpecialLocation (File::currentExecutableFile),
                                 kScannerProcessUID, 0, 0);
        }

        enum class State { timeout, gotResult, connectionLost };

        struct Response
        {
            State state;
            std::unique_ptr<XmlElement> xml;
        };

        Response getResponse()
        {
            std::unique_lock<std::mutex> lock (mutex);

            if (! condvar.wait_for (lock,
                                    std::chrono::milliseconds (50),
                                    [&] { return gotResult || connectionLost; }))
                return { State::timeout, nullptr };

            const auto state = connectionLost ? State::connectionLost : State::gotResult;
            connectionLost = false;
            gotResult = false;

            return { state, std::move (pluginDescription) };
        }

        using ChildProcessCoordinator::sendMessageToWorker;

    private:
        void handleMessageFromWorker (const MemoryBlock& mb) override
        {
            const std::lock_guard<std::mutex> lock (mutex);
            pluginDescription = parseXML (mb.toString());
            gotResult = true;
            condvar.notify_one();
        }

        void handleConnectionLost() override
        {
            const std::lock_guard<std::mutex> lock (mutex);
            connectionLost = true;
            condvar.notify_one();
        }

        std::mutex mutex;
        std::condition_variable condvar;
        std::unique_ptr<XmlElement> pluginDescription;
        bool connectionLost = false;
        bool gotResult = false;
    };

    OutOfProcessScanner::OutOfProcessScanner() = default;
    OutOfProcessScanner::~OutOfProcessScanner() = default;

    bool OutOfProcessScanner::findPluginTypesFor (AudioPluginFormat& format,
                                                  OwnedArray<PluginDescription>& result,
                                                  const String& fileOrIdentifier)
    {
        // Su macOS gli AudioUnit girano già in audiocomponentd (sandbox di sistema), quindi
        // scansionarli in-process non può far crashare l'host. Mandarli al worker sarebbe solo
        // overhead IPC. Solo i VST3 fanno dlopen reale e hanno bisogno dell'isolamento.
        if (format.getName() == "AudioUnit")
        {
            format.findAllTypesForFile (result, fileOrIdentifier);
            return true;
        }

        if (superprocess == nullptr)
            superprocess = std::make_unique<Superprocess>();

        MemoryBlock block;
        MemoryOutputStream stream (block, true);
        stream.writeString (format.getName());
        stream.writeString (fileOrIdentifier);
        stream.flush();

        if (! superprocess->sendMessageToWorker (block))
        {
            superprocess.reset();
            return false;
        }

        for (;;)
        {
            if (shouldExit())
                return true;

            const auto response = superprocess->getResponse();

            if (response.state == Superprocess::State::timeout)
                continue;

            if (response.xml != nullptr)
            {
                for (const auto* item : response.xml->getChildIterator())
                {
                    auto desc = std::make_unique<PluginDescription>();
                    if (desc->loadFromXml (*item))
                        result.add (std::move (desc));
                }
            }

            // connectionLost: worker morto durante questo plugin → JUCE blacklista il path.
            // gotResult: scan pulito, riusiamo il subprocess per il prossimo plugin.
            if (response.state == Superprocess::State::connectionLost)
            {
                superprocess.reset();
                return false;
            }

            return true;
        }
    }

    void OutOfProcessScanner::scanFinished()
    {
        superprocess.reset();
    }
}
