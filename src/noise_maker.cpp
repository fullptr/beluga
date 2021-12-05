#include "noise_maker.h"
#include "helpers.h"
#include "constants.h"

#include <Windows.h>

#include <cmath>
#include <vector>
#include <string>
#include <limits>

namespace blga {
namespace {

auto scale(double value) -> double
{
	return std::clamp(value, -1.0, 1.0) * std::numeric_limits<short>::max();
}

}

noise_maker::noise_maker(const blga::instrument& instrument)
    : d_instrument{instrument}
    , d_audio_buffer{}
    , d_thread{}
    , d_ready{true}
    , d_time{0.0}
    , d_semaphore{num_blocks}
{
    // Callback passed to WaveOpen. Releases the semaphore to request more data.
    const auto cb = +[](HWAVEOUT, UINT msg, DWORD_PTR user_data, DWORD_PTR, DWORD_PTR) {
        if (msg == WOM_DONE) {
            auto semaphore = (std::counting_semaphore<num_blocks>*)user_data;
            semaphore->release();
        }
    };

    auto device = HWAVEOUT{};
    const auto rc = waveOutOpen(
        &device, 0, &format, (DWORD_PTR)cb, (DWORD_PTR)&d_semaphore, CALLBACK_FUNCTION
    );

    if (rc != S_OK) {
        throw std::exception("Could not open sound device");
    }

    // Thread that acquires the semaphore to send more data to WaveOpen.
    d_thread = std::jthread([&, device]{
        while (d_ready) {
            d_semaphore.acquire();
            auto lock = std::unique_lock{d_instrument_mtx};
            
            auto& block = d_audio_buffer.next_block();
            for (auto& datum : block.data) {
                double amp = 0.0;
                for (const auto& note : d_notes) {
                    if (note.active || d_time < note.toggle_time + d_instrument.d_envelope.release_time) {
                        amp += d_instrument.d_envelope.amplitude(d_time, note.toggle_time, note.active) *
                               d_instrument.d_oscillator(blga::note_frequency(note.key), d_time);
                    }
                }
                datum = static_cast<short>(scale(amp) * 0.2);
                d_time += 1.0 / sample_rate;
            }

            block.send_to_sound_device(device);
        }
    });
}

noise_maker::~noise_maker()
{
    d_ready = false;
}

auto noise_maker::note_on(int key, double time) -> void
{
    auto lock = std::unique_lock{d_instrument_mtx};
    d_notes.emplace_back(key, time, true);
}

auto noise_maker::note_off(int key, double time) -> void
{
    auto lock = std::unique_lock{d_instrument_mtx};
    for (auto& note : d_notes) {
        if (note.key == key && note.active) {
            note.active = false;
            note.toggle_time = time;
        }
    }
}

}