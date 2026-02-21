#pragma once

#include <cstdint>
#include <string>
#include <vector>

// forward-declare sfizz
namespace sfz {
class Sfizz;
}

class SfizzEngine {
public:
    SfizzEngine();
    ~SfizzEngine();

    // lifecycle
    bool loadSfz(const std::string& path);
    void setSampleRate(double sampleRate);

    // MIDI-like API (agnóstico de backend)
    void noteOn(int delay, uint8_t note, uint8_t velocity);
    void noteOff(int delay, uint8_t note, uint8_t velocity);
    void controlChange(int delay, uint8_t cc, uint8_t value);
    void pitchBend(int delay, int value); // -8192..8191

    // render
    // outL/outR: buffers fornecidos pelo chamador
    void render(float* outL, float* outR, uint32_t nframes);

    // introspecção (já pensando no futuro)
    uint32_t numActiveVoices() const;

private:
    SfizzEngine(const SfizzEngine&) = delete;
    SfizzEngine& operator=(const SfizzEngine&) = delete;

    sfz::Sfizz* synth_ = nullptr;
    double sampleRate_ = 48000.0;
};