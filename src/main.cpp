#include "CLI/CLI.hpp"
#include "tui_core.hpp"
#include "sfizz_client.hpp"
#include "core/logger.hpp"
#include <core/utils.hpp>
#include <CLI/CLI.hpp>

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

static std::atomic<bool> running{true};

static void signalHandler(int) {
    running = false;
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    CLI::App app{"SfizzTUI - Terminal Sample Player"};

    auto homeDir = std::getenv("HOME");
    std::string sfzPath = homeDir ? std::string(homeDir) + "/soundfonts" : "./";
    std::string mapPath;

    app.add_option("--sfzpath", sfzPath, "SFZ files root folder")->check(CLI::ExistingFile);
    app.add_option("--midimap", mapPath, "MIDI Map file")->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    Logger logger;
    logger.start();

    SfizzJackApp sfizzApp;
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

    std::thread uiThread([&] {
        tui.run();
        running = false;
    });

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    sfizzApp.close();

    logger.stop();

    if (uiThread.joinable())
        uiThread.join();

    return 0;
}