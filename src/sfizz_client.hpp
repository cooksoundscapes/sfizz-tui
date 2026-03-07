#include "core/jack.hpp"
#include "sfizz_engine.hpp"
#include <memory>

class SfizzJackApp : public JackClient {
public:
    SfizzJackApp() : JackClient("sfizz_tui") {
        engine_ = std::make_unique<SfizzEngine>();
    }
    SfizzEngine& getEngine() { return *engine_; }

protected:
    void processAudio(float** outputs, uint32_t nframes) override;
    void processMidi(MidiEvent ev) override;

private:
    std::unique_ptr<SfizzEngine> engine_;
};