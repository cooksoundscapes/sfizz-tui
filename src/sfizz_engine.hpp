#pragma once

#include <cstdint>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>


#include "sfz_cache.hpp"

// forward-declare sfizz
namespace sfz {
class Sfizz;
}

using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

class SfizzEngine {
public:
    SfizzEngine();
    ~SfizzEngine();

    bool loadSfz(const std::string& path);
    bool loadSfzAsync(const std::string& path);

    void setSampleRate(double sampleRate);

    void noteOn(int delay, uint8_t note, uint8_t velocity);
    void noteOff(int delay, uint8_t note, uint8_t velocity);
    void controlChange(int delay, uint8_t cc, uint8_t value);
    void pitchBend(int delay, int value); // -8192..8191

    void render(float* outL, float* outR, uint32_t nframes);

    float getLoad() { return audioLoad.load(); }
    std::string getLoadedFile() { return loadedFile_; }
    uint32_t getNumActiveVoices() const;
    bool isLoading() const { return isLoading_.load(); }
    const std::string& getCurrentSwitch() { return activeSwitch_; }

private:
    SfizzEngine(const SfizzEngine&) = delete;
    SfizzEngine& operator=(const SfizzEngine&) = delete;

    sfz::Sfizz* synth_ = nullptr;
    double sampleRate_ = 48000.0;

    std::string loadedFile_;

    std::atomic<bool> isLoading_{false};
    std::mutex engineMutex_;

    std::atomic<float> audioLoad = 0.0f;
    const float loadCoeff = 0.1f;

    void updateLoad(time_point, time_point, uint32_t);

    template<typename F>
    void dispatchToSynth(F&& func) {
        std::unique_lock<std::mutex> lock(engineMutex_, std::try_to_lock);
        if (lock.owns_lock() && !isLoading_) {
            func();
        }
    }

    SfzMetaData metadata;
    std::string activeSwitch_;
};