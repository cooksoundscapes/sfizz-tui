#include "sfizz_engine.hpp"

#include <sfizz.hpp>
#include <cstring>

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

void SfizzEngine::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    synth_->setSampleRate(sampleRate_);
}

void SfizzEngine::noteOn(int delay, uint8_t note, uint8_t vel)
{
    // O sfizz aceita o delay como primeiro argumento em muitas sobrecargas
    synth_->noteOn(delay, note, vel);
}

void SfizzEngine::noteOff(int delay, uint8_t note, uint8_t vel)
{
    synth_->noteOff(delay, note, vel);
}

void SfizzEngine::controlChange(int delay, uint8_t cc, uint8_t val)
{
    synth_->cc(delay, cc, val);
}

void SfizzEngine::pitchBend(int delay, int value)
{
    synth_->pitchWheel(delay, value);
}

void SfizzEngine::render(float* outL, float* outR, uint32_t nframes)
{
    // sfizz soma nos buffers → limpar antes
    // MAS jack ja deveria fazer
    // std::memset(outL, 0, sizeof(float) * nframes);
    // std::memset(outR, 0, sizeof(float) * nframes);

    float* outputs[2] = { outL, outR };
    synth_->renderBlock(outputs, nframes);
}

uint32_t SfizzEngine::numActiveVoices() const
{
    return synth_->getNumActiveVoices();
}