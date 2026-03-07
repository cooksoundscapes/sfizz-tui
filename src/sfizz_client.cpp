#include "sfizz_client.hpp"
#include "core/midi_types.hpp"
#include "midi_parser.hpp"

void SfizzJackApp::processAudio(float** outputs, uint32_t nframes) {
    engine_->render(outputs[0], outputs[1], nframes);
}

void SfizzJackApp::processMidi(MidiEvent ev) {
    switch (ev.type) {
        case MidiInputType::NOTE_ON:
            if (ev.data2 > 0)
                engine_->noteOn(ev.delay, ev.data1, ev.data2);
            else
                engine_->noteOff(ev.delay, ev.data1, 0);
            break;

        case MidiInputType::NOTE_OFF:
            engine_->noteOff(ev.delay, ev.data1, ev.data2);
            break;

        case MidiInputType::CONTROL_CHANGE:
            engine_->controlChange(ev.delay, ev.data1, ev.data2);
            break;

        case MidiInputType::PITCH_BEND: {
            // Pitch bend is 14 bits: (LSB | (MSB << 7))
            // Central value is 8192 (0x2000)
            int value = ev.data1 | (ev.data2 << 7);
            engine_->pitchBend(ev.delay, value);
            break;
        }
        default:
            break;
    }
}