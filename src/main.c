#include <vector>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> // usleep()

#include "colors.hpp"
#include "dmx.hpp"
#include "jsscript.hpp"

#ifdef ENABLE_AUDIO
#include "audio.hpp"
#endif

#ifdef ENABLE_MIDI
#include "midi.hpp"
#endif

int main() {
#ifdef ENABLE_AUDIO
    Audio audio;
#endif
#ifdef ENABLE_MIDI
    Midi midi;
#endif
    JSScript script("scene.js");
    std::vector<UnicastDMX> dmx;
    for (long unsigned i = 0; i < script.vec_ip.size(); i++) {
        const char * ip = script.vec_ip.at(i).c_str();
        int nleds = script.vec_nleds.at(i);
        printf("device [%lu] ip: %s\n", i, ip);
        printf("device [%lu] nleds: %d\n", i, nleds);
        printf("device [%lu] universe: 1\n", i);
        dmx.emplace_back(ip, nleds, /* universe */ 1);
    }
    struct timespec ts_begin, ts_curr;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_begin);
    for (;;) {
#ifdef ENABLE_MIDI
        for (;;) {
            auto m = midi.get_message();
            if (m.empty()) {
                /* value changes like to queue up */
                break;
            }
            std::string name = m.variable_name();
            script.set_number(name, m.value);
            std::cout << name << " = " << (int)m.value << std::endl;
        }
#endif
#ifdef ENABLE_AUDIO
        float v = audio.get_rms();
        script.set_amp(v);
#endif
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts_curr);
        float elapsed = (ts_curr.tv_sec -ts_begin.tv_sec) +
                        (ts_curr.tv_nsec-ts_begin.tv_nsec) * 1e-9;
        int dmx_id = 1;
        for (auto & d : dmx) {
            for (int pos = 0; pos < d._nleds; pos++) {
                rgb color = script.get_color(pos, elapsed, dmx_id);
                d.set(pos, color);
            }
            dmx_id += 1;
        }
        for (auto & d : dmx) {
            d.send();
        }
        script.recompile_if_changed();
        usleep(10*1000);
    }
}
