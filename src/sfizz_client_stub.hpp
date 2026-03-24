#pragma once
#include <string>
#include <vector>

// Stub do SfizzEngine
struct StubEngine {
    bool isLoading() const { return false; }
    float getLoad() const { return 0.0f; }
    std::string getCurrentSwitch() const { return "Standard"; }
    void loadSfzAsync(const std::string&) {}
};

// Stub do SfizzJackApp
struct StubSfizzApp {
    StubEngine& getEngine() { return engine_; }

    bool open() { return true; }
    bool activate() { return true; }
    void close() {}

    bool getJackStatus() const { return true; }

    std::string getLastConnectedDevice() const { return "Stub MIDI"; }
    void setLastConnectedDevice(const std::string&) {}

    std::vector<std::string> getAvailableMidiSources() const {
        return {"Stub MIDI Port 1", "Stub MIDI Port 2"};
    }

private:
    StubEngine engine_;
};