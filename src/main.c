#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "libe131/e131.h"
#include "duktape/duktape.h"

int hex(char s) {
    if (s >= '0' && s <= '9') {
        return s - '0';
    }
    if (s >= 'a' && s <= 'f') {
        return s - 'a';
    }
    if (s >= 'A' && s <= 'F') {
        return s - 'A';
    }
    return 0;
}

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

uint8_t clamp_rgb_value(float raw) {
    if (raw <= 0) {
        return 0;
    } else if (raw >= 255) {
        return 255;
    } else {
        return round(raw);
    }
}

struct rgb hsv2rgb(float hsv[3]) {
    // from https://stackoverflow.com/questions/3018313
    double hh, ff;
    uint8_t p, q, t, v;
    long i;
    struct rgb out;
    if(hsv[1] <= 0.0) { out.r = hsv[2]; out.g = hsv[2]; out.b = hsv[2]; return out; }
    hh = fmod(hsv[0] * 180.0 / M_PI + 360, 360) / 60;
    i = (long)hh;
    ff = hh - i;
    p = clamp_rgb_value(255 * hsv[2] * (1.0 - hsv[1]));
    q = clamp_rgb_value(255 * hsv[2] * (1.0 - (hsv[1] * ff)));
    t = clamp_rgb_value(255 * hsv[2] * (1.0 - (hsv[1] * (1.0 - ff))));
    v = clamp_rgb_value(255 * hsv[2]);
    switch(i) {
        case 0: out = { v, t, p }; break;
        case 1: out = { q, v, p }; break;
        case 2: out = { p, v, t }; break;
        case 3: out = { p, q, v }; break;
        case 4: out = { t, p, v }; break;
        case 5:
        default: out = { v, p, q }; break;
    }
    return out;
}

#include "color_names.hpp"
#include "jsscript.hpp"
#include "dmx.hpp"

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