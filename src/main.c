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
    printf("ip: '%s\n", script.ip);
    printf("ip2: '%s'\n", script.ip2);
    printf("ip3: '%s'\n", script.ip3);
    printf("ip4: '%s'\n", script.ip4);
    printf("nleds: %d\n", script.nleds);
    printf("nleds2: %d\n", script.nleds2);
    printf("nleds3: %d\n", script.nleds3);
    printf("nleds4: %d\n", script.nleds4);
    std::vector<UnicastDMX> dmx;
    dmx.emplace_back(script.ip, script.nleds, /* universe */1);
    if (strcmp(script.ip2, "undefined") != 0) {
        dmx.emplace_back(script.ip2, script.nleds2, /* universe */1);
    }
    if (strcmp(script.ip3, "undefined") != 0) {
        dmx.emplace_back(script.ip3, script.nleds3, /* universe */1);
    }
    if (strcmp(script.ip4, "undefined") != 0) {
        dmx.emplace_back(script.ip4, script.nleds4, /* universe */1);
    }
    //UnicastDMX dmx(script.ip, script.nleds, /* universe = */1);
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
