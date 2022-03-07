#pragma once

#include <math.h>

#include "color_names.hpp"

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
