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

int main() {
#ifdef ENABLE_AUDIO
    Audio audio;
#endif
    JSScript script("scene.js");
    printf("ip: %s\n", script.ip);
    printf("nleds: %d\n", script.nleds);
    UnicastDMX dmx(script.ip, script.nleds, /* universe = */1);
    struct timespec ts_begin, ts_curr;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_begin);
    for (;;) {
#ifdef ENABLE_AUDIO
        float v = audio.get_rms();
        script.set_amp(v);
#endif
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts_curr);
        float elapsed = (ts_curr.tv_sec -ts_begin.tv_sec) +
                        (ts_curr.tv_nsec-ts_begin.tv_nsec) * 1e-9;
        for (int pos = 0; pos < script.nleds; pos++) {
            rgb color = script.get_color(pos, elapsed);
            dmx.set(pos, color);
        }
        dmx.send();
        script.recompile_if_changed();
        usleep(10*1000);
    }
}
