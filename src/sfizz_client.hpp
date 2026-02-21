#include "jack.hpp"
#include "sfizz_engine.hpp"
#include <memory>

class SfizzJackApp : public JackClient {
public:
    SfizzJackApp() : JackClient("sfizz_tui") {
        // Inicialize o sfizz aqui ou receba por injeção
        engine_ = std::make_unique<SfizzEngine>();
    }

    void loadSfz(const std::string& path) { engine_->loadSfz(path); }

protected:
    void processAudio(float** outputs, uint32_t nframes) override {
        // outputs[0] é o L, outputs[1] é o R
        engine_->render(outputs[0], outputs[1], nframes);
    }

    void processMidi(const jack_midi_event_t& ev) {
    if (ev.size == 0) return;

    uint8_t status = ev.buffer[0];
    uint8_t command = status & 0xF0; // Tipo de mensagem (Note On, CC, etc)
    // uint8_t channel = status & 0x0F; // Canal (0-15)
    int delay = static_cast<int>(ev.time);

    switch (command) {
        case 0x90: // Note On
            if (ev.size >= 3) {
                if (ev.buffer[2] > 0)
                    engine_->noteOn(delay, ev.buffer[1], ev.buffer[2]);
                else
                    engine_->noteOff(delay, ev.buffer[1], 0);
            }
            break;

        case 0x80: // Note Off
            if (ev.size >= 3) {
                engine_->noteOff(delay, ev.buffer[1], ev.buffer[2]);
            }
            break;

        case 0xB0: // Control Change (CC)
            if (ev.size >= 3) {
                engine_->controlChange(delay, ev.buffer[1], ev.buffer[2]);
            }
            break;

        case 0xE0: // Pitch Bend
            if (ev.size >= 3) {
                // Pitch bend são 14 bits: (LSB | (MSB << 7))
                // O valor central é 8192 (0x2000)
                int value = ev.buffer[1] | (ev.buffer[2] << 7);
                engine_->pitchBend(delay, value);
            }
            break;

        default:
            // Outros eventos (Aftertouch, Program Change, etc)
            // Você pode implementar conforme a necessidade.
            break;
    }
}

private:
    std::unique_ptr<SfizzEngine> engine_;
};