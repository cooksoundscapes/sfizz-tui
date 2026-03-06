#include "tui.hpp"
#include "sfizz_client.hpp"
#include "core/logger.hpp"
#include <core/utils.hpp>

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
    // Configura signals
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // diretório dos SFZs
    auto homeDir = std::getenv("HOME");
    std::string sfzDir;
    if (argc > 1) {
        sfzDir = argv[1];
    } else {
        sfzDir = homeDir + std::string("/soundfonts");
    }

    // Inicia logger e captura stderr em memoria
    Logger logger;
    logger.start();

    // JACK
    SfizzJackApp sfizz_app;
    if (!sfizz_app.open())
        return 1;

    if (!sfizz_app.activate())
        return 1;

    // TUI
    TuiClient tui(sfzDir);

    tui.onSfzSelected = [&](const std::string& fullPath) {
        sfizz_app.getEngine().loadSfzAsync(fullPath);
    };
    tui.isEngineLoading = [&](){
        return sfizz_app.getEngine().isLoading();
    };
    tui.getMidiDeviceName = [&](){
        return sfizz_app.getLastConnectedDevice();
    };
    tui.getCpuLoad = [&](){
        return sfizz_app.getEngine().getLoad();
    };
    tui.getEngineStatus = [&](){
        return sfizz_app.getJackStatus();
    };
    tui.getMidiDevices = [&](){
        return sfizz_app.getAvailableMidiSources();
    };
    tui.onMidiSourceSelected = [&](std::string source){
        sfizz_app.setLastConnectedDevice(source);
    };
    tui.getLogBuffer = [&](){
        return logger.getBufferView();
    };
    tui.getActiveKeyswitch = [&](){
        return sfizz_app.getEngine().getCurrentSwitch();
    };

    // roda a UI em thread separada
    std::thread uiThread([&] {
        tui.run();
        running = false; // se a UI sair, encerra o app
    });

    // loop principal (só mantém o processo vivo)
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // shutdown ordenado
    sfizz_app.close();

    logger.stop();

    if (uiThread.joinable())
        uiThread.join();

    return 0;
}