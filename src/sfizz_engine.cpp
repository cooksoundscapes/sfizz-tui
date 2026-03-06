#include "sfizz_engine.hpp"
#include "sfz_parser.hpp"

#include <sfizz.hpp>
#include <cstring>
#include <thread>

SfizzEngine::SfizzEngine()
{
    synth_ = new sfz::Sfizz();
    synth_->setSampleRate(sampleRate_);
}

SfizzEngine::~SfizzEngine()
{
    delete synth_;
}

bool SfizzEngine::loadSfz(const std::string& path)
{
    return synth_->loadSfzFile(path);
}

bool SfizzEngine::loadSfzAsync(const std::string& path) {
    if (isLoading_) return false;

    if (path == loadedFile_) {
        return false;
    }
    loadedFile_ = path;
    isLoading_ = true;
    
    // Dispara uma thread separada para não travar a UI
    std::thread([this, path]() {
        {
            std::lock_guard<std::mutex> lock(engineMutex_);
            synth_->allSoundOff();
            synth_->loadSfzFile(path);

            metadata = SfzParser::load(path);
            int defaultId = metadata.default_switch;

            if (defaultId >= 0 && defaultId < 128 && metadata.keyswitch_map[defaultId].active) {
                // Se o default existe e tem label, usa ele
                activeSwitch_ = metadata.keyswitch_map[defaultId].label;
            } else {
                // Caso contrário, limpa ou define um padrão
                activeSwitch_ = "Standard"; 
            }
        }
        isLoading_ = false;
    }).detach();
    
    return true;
}

void SfizzEngine::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    synth_->setSampleRate(sampleRate_);
}

void SfizzEngine::noteOn(int delay, uint8_t note, uint8_t vel)
{
    dispatchToSynth([&] {
        synth_->noteOn(delay, note, vel);
        if (metadata.keyswitch_map[note].active)
            activeSwitch_ = metadata.keyswitch_map[note].label;
    });
}

void SfizzEngine::noteOff(int delay, uint8_t note, uint8_t vel)
{
    dispatchToSynth([&] { synth_->noteOff(delay, note, vel); });
}

void SfizzEngine::controlChange(int delay, uint8_t cc, uint8_t val)
{
    dispatchToSynth([&] { synth_->cc(delay, cc, val); });
}

void SfizzEngine::pitchBend(int delay, int value)
{
    dispatchToSynth([&] { synth_->pitchWheel(delay, value); });
}

void SfizzEngine::render(float* outL, float* outR, uint32_t nframes)
{
    // Tenta travar (try_lock). Se o carregamento estiver usando o mutex,
    // o render apenas retorna silêncio para não travar o JACK (xrun).
    std::unique_lock<std::mutex> lock(engineMutex_, std::try_to_lock);
    
    std::memset(outL, 0, sizeof(float) * nframes);
    std::memset(outR, 0, sizeof(float) * nframes);

    if (lock.owns_lock()) {
        auto start = std::chrono::high_resolution_clock::now();
        float* outputs[2] = { outL, outR };
        synth_->renderBlock(outputs, nframes);
        auto end = std::chrono::high_resolution_clock::now();
        updateLoad(start, end, nframes);
    }
}

void SfizzEngine::updateLoad(time_point start, time_point end, uint32_t nframes) {
    std::chrono::duration<double> diff = end - start;
    float instantLoad = diff.count() / (nframes / sampleRate_) * 100.0f;

    float current = audioLoad.load();
    audioLoad.store(current + loadCoeff * (instantLoad - current));
}

uint32_t SfizzEngine::numActiveVoices() const
{
    return synth_->getNumActiveVoices();
}

