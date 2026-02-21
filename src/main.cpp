#include "jack.hpp"
#include "tui.hpp"
#include "sfizz_client.hpp"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

static std::atomic<bool> running{true};

static void signalHandler(int) {
    running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // diretório dos SFZs
    std::string sfzDir = "./sfz";
    if (argc > 1) {
        sfzDir = argv[1];
    }

    // JACK
    SfizzJackApp sfizz_app;
    if (!sfizz_app.open())
        return 1;

    if (!sfizz_app.activate())
        return 1;

    // TUI
    TuiClient tui(sfzDir);

    tui.onSfzSelected = [&](const std::string& filename) {
        std::string fullPath = sfzDir + "/" + filename;
        sfizz_app.loadSfz(fullPath);
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

    if (uiThread.joinable())
        uiThread.join();

    return 0;
}