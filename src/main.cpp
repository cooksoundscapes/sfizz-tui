#include "CLI/CLI.hpp"
#include "tui_core.hpp"
#include "core/logger.hpp"

#ifdef STUB_MODE
#include "sfizz_client_stub.hpp"
using SfizzJackAppType = StubSfizzApp;
#else
#include "sfizz_client.hpp"
using SfizzJackAppType = SfizzJackApp;
#endif

#include <core/utils.hpp>
#include <CLI/CLI.hpp>

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

static std::atomic<bool> running{true};
static std::atomic<bool> signaled{false};


static void signalHandler(int) {
    running = false;
}

static void customSigHandler(int) {
    signaled = true;
}

int main(int argc, char** argv)
{
    // OS Setup -------------------
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGUSR1, customSigHandler);

    CLI::App app{"SfizzTUI - Terminal Sample Player"};

    auto homeDir = std::getenv("HOME");
    std::string sfzPath = homeDir ? std::string(homeDir) + "/soundfonts" : "./";
    std::string mapPath;

    app.add_option("--sfzpath", sfzPath, "SFZ files root folder")->check(CLI::ExistingFile);
    app.add_option("--midimap", mapPath, "MIDI Map file")->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    // Main classes ----------------

    Logger logger;
    logger.start();

    SfizzJackAppType sfizzApp;
    if (!sfizzApp.open())
        return 1;

    TuiClient tui(sfzPath);
    if (!mapPath.empty()) {
        tui.loadMidiMap(mapPath);
    }

    sfizzApp.midiExternalCallback = [&](MidiEvent e) {
        tui.handleMidiEvent(e);
    };
    tui.onSfzSelected = [&](const std::string& fullPath) {
        sfizzApp.getEngine().loadSfzAsync(fullPath);
    };
    tui.context().isEngineLoading = [&](){
        return sfizzApp.getEngine().isLoading();
    };
    tui.context().getMidiDeviceName = [&](){
        return sfizzApp.getLastConnectedDevice();
    };
    tui.context().getCpuLoad = [&](){
        return sfizzApp.getEngine().getLoad();
    };
    tui.context().getEngineStatus = [&](){
        return sfizzApp.getJackStatus();
    };
    tui.context().getMidiDevices = [&](){
        return sfizzApp.getAvailableMidiSources();
    };
    tui.context().onMidiSourceSelected = [&](std::string source){
        sfizzApp.setLastConnectedDevice(source);
    };
    tui.context().getLogBuffer = [&](){
        return logger.getBufferView();
    };
    tui.context().getActiveKeyswitch = [&](){
        return sfizzApp.getEngine().getCurrentSwitch();
    };

    if (!sfizzApp.activate())
        return 1;

    tui.start();
    int frame = 0, fps = 30, refresh_fps = 5;
    while (!tui.loopHasQuitted()) {
        if (signaled.exchange(false)) {
            std::cerr << "MIDI device connected!\n";
        }
        bool shouldRedraw = (frame % (fps / refresh_fps)) == 0;
        tui.runOnce(shouldRedraw);
        frame = (frame + 1) % fps;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
    }

    sfizzApp.close();

    logger.stop();

    return 0;
}