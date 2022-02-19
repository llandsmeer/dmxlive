#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "colors.h"
#include "libe131/e131.h"
#include "duktape/duktape.h"

#ifdef ENABLE_AUDIO
#include <AL/al.h>
#include <AL/alc.h>
#endif

const char * push_string_file_raw(duk_context *ctx, const char *path) {
    // from duktape/extras/duk-v1-compat/duk_v1_compat.c
    FILE *f = NULL;
    char *buf;
    long sz;  /* ANSI C typing */

    if (!path) {
        goto fail;
    }
    f = fopen(path, "rb");
    if (!f) {
        goto fail;
    }
    if (fseek(f, 0, SEEK_END) < 0) {
        goto fail;
    }
    sz = ftell(f);
    if (sz < 0) {
        goto fail;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        goto fail;
    }
    buf = (char *) duk_push_fixed_buffer(ctx, (duk_size_t) sz);
    if ((size_t) fread(buf, 1, (size_t) sz, f) != (size_t) sz) {
        duk_pop(ctx);
        goto fail;
    }
    (void) fclose(f);  /* ignore fclose() error */
    return duk_buffer_to_string(ctx, -1);

    fail:
    if (f) {
        (void) fclose(f);  /* ignore fclose() error */
    }

    (void) duk_type_error(ctx, "read file error");
    err(EXIT_FAILURE, "duk_string_file_raw");
    return NULL;
}


uint64_t mtime_us(const char * path) {
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtim.tv_sec * 1000000 + attr.st_mtim.tv_nsec / 1000;
}

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

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

static duk_ret_t js_hsv2rgb(duk_context * ctx) {
  int nargs = duk_get_top(ctx);
  float hsv[3] = { 0, 1.0, 1.0 };
  if (nargs >= 1) hsv[0] = duk_get_number_default(ctx, 0, hsv[0]);
  if (nargs >= 2) hsv[1] = duk_get_number_default(ctx, 1, hsv[1]);
  if (nargs >= 3) hsv[2] = duk_get_number_default(ctx, 2, hsv[2]);
  struct rgb rgb = hsv2rgb(hsv);
  duk_idx_t arr = duk_push_array(ctx);
  duk_push_int(ctx, rgb.r);
  duk_put_prop_index(ctx, arr, 0);
  duk_push_int(ctx, rgb.g);
  duk_put_prop_index(ctx, arr, 1);
  duk_push_int(ctx, rgb.b);
  duk_put_prop_index(ctx, arr, 2);
  return 1;  /* one return value */
}

struct rgb get_rgb_color(duk_context * ctx) {
    struct rgb rgb;
    duk_int_t t = duk_get_type(ctx, -1);
    switch(t) {
        case DUK_TYPE_BOOLEAN: {
            bool raw = duk_get_boolean(ctx, -1);
            rgb.r = rgb.g = rgb.b = raw ? 255 : 0;
            return rgb;
        }
        case DUK_TYPE_NUMBER: {
            // single number = grayscale 0-255
            float raw = duk_get_number(ctx, -1);
            rgb.r = rgb.g = rgb.b = clamp_rgb_value(raw);
            return rgb;
        }
        case DUK_TYPE_OBJECT: {
            // array: [r, g, b]
            duk_get_prop_index(ctx, -1, 0);
            duk_get_prop_index(ctx, -2, 1);
            duk_get_prop_index(ctx, -3, 2);
            if (duk_check_type(ctx, -3, DUK_TYPE_NUMBER) &&
                duk_check_type(ctx, -2, DUK_TYPE_NUMBER) &&
                duk_check_type(ctx, -1, DUK_TYPE_NUMBER)) {
                rgb.r = clamp_rgb_value(duk_get_number(ctx, -3));
                rgb.g = clamp_rgb_value(duk_get_number(ctx, -2));
                rgb.b = clamp_rgb_value(duk_get_number(ctx, -1));
                duk_pop_3(ctx);
                return rgb;
            }
            duk_pop_3(ctx);
            rgb.r = rgb.g = rgb.b = 0;
            return rgb;
        }
        case DUK_TYPE_STRING: {
            const char * raw = duk_get_string(ctx, -1);
            int len = strlen(raw);
            if (len == 0) {
                rgb.r = rgb.g = rgb.b = 0;
            } else if (raw[0] == '#' && len == 1 + 3) {
                rgb.r = 16 * hex(raw[1]);
                rgb.g = 16 * hex(raw[2]);
                rgb.b = 16 * hex(raw[3]);
            } else if (raw[0] == '#' && len == 1 + 6) {
                rgb.r = 16 * hex(raw[1]) + hex(raw[2]);
                rgb.g = 16 * hex(raw[3]) + hex(raw[4]);
                rgb.b = 16 * hex(raw[5]) + hex(raw[6]);
            } else {
              rgb = {0 ,0, 0};
              for (size_t i = 0; i < sizeof(namedcolors) / sizeof(struct namedcolor); i++) {
                  if (strcmp(namedcolors[i].name, raw) == 0) {
                      rgb = {namedcolors[i].r, namedcolors[i].g, namedcolors[i].b};
                      break;
                  }
              }
            }
            return rgb;
        }
        default:
        case DUK_TYPE_BUFFER:
        case DUK_TYPE_POINTER:
        case DUK_TYPE_LIGHTFUNC:
        case DUK_TYPE_NONE:
        case DUK_TYPE_NULL:
        case DUK_TYPE_UNDEFINED: {
            rgb.r = rgb.g = rgb.b = 0;
            return rgb;
        }
    }
}

void setup_environment(duk_context * ctx) {
    duk_push_string(ctx, "Object.getOwnPropertyNames(Math).forEach((function(name) { this[name] = Math[name] }).bind(this))");
    if (duk_peval(ctx) != DUK_EXEC_SUCCESS) {
        printf("copy math error: %s\n", duk_safe_to_string(ctx, -1));
    }
    duk_push_c_function(ctx, js_hsv2rgb, DUK_VARARGS);
    duk_put_global_string(ctx, "hsv");
}

int main() {
    const char * path = "scene.js";
    uint64_t last_mtime = mtime_us(path);

#ifdef ENABLE_AUDIO
    ALbyte audio_buffer[4410];
    ALint audio_nsamples;
    ALCdevice * device = alcCaptureOpenDevice(NULL, 44100, AL_FORMAT_MONO16, 1024);
    if (!device) {
        printf("alcCaptureOpenDevice: %s\n", alGetString(alGetError()));
        return EXIT_FAILURE;
    }
    alcCaptureStart(device);
#endif

    duk_context * ctx = duk_create_heap_default();
    if (!ctx) {
        err(EXIT_FAILURE, "duk_create_heap_default");
    }

    setup_environment(ctx);
    duk_push_number(ctx, 0);
    duk_put_global_string(ctx, "rms");
    duk_push_number(ctx, 0);
    duk_put_global_string(ctx, "amp");

    push_string_file_raw(ctx, path);
    duk_push_string(ctx, path);
    if (duk_pcompile(ctx, 0) != 0) {
        printf("initial compilation failed: %s\n", duk_safe_to_string(ctx, -1));
    } else {
        if (duk_pcall(ctx, 0) == DUK_EXEC_SUCCESS) {
        } else {
            printf("error: %s\n", duk_safe_to_string(ctx, -1));
        }
    }
    duk_pop(ctx);

    duk_get_global_string(ctx, "ip");
    const char * ip = duk_safe_to_string(ctx, -1);
    duk_pop(ctx);
    printf("ip: %s\n", ip);

    duk_get_global_string(ctx, "nleds");
    int nleds = duk_get_int(ctx, -1);
    duk_pop(ctx);
    printf("nleds: %d\n", nleds);

    int sockfd;
    e131_packet_t packet;
    e131_addr_t dest;

    if ((sockfd = e131_socket()) < 0) {
        err(EXIT_FAILURE, "e131_socket");
    }

    e131_pkt_init(&packet, 1, 3 * nleds);
    memcpy(&packet.frame.source_name, "dmxlive", 18);
    if (e131_set_option(&packet, E131_OPT_PREVIEW, true) < 0) {
        err(EXIT_FAILURE, "e131_set_option");
    }

    if (e131_unicast_dest(&dest, ip, E131_DEFAULT_PORT) < 0) {
        err(EXIT_FAILURE, "e131_unicast_dest");
    }

    struct timespec ts_begin, ts_curr;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_begin);
    char preverror[1024] = "";
    for (;;) {
        // retrieve microphone data
#ifdef ENABLE_AUDIO
        alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &audio_nsamples);
        if (audio_nsamples > sizeof(audio_buffer) / 2) {
            audio_nsamples = sizeof(audio_buffer) / 2;
        }

        if (audio_nsamples > 1024) {
            alcCaptureSamples(device, (ALCvoid *)audio_buffer, audio_nsamples);
            int16_t * buf = (int16_t *)audio_buffer;
            float rms = 0;
            for (int i = 0; i < audio_nsamples; i++) {
                float v = (float)buf[i] / (UINT16_MAX+1);
                rms += v * v;
            }
            duk_push_number(ctx, rms / audio_nsamples);
            duk_put_global_string(ctx, "amp");
        }
#endif
        // sending data
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts_curr);
        float elapsed = (ts_curr.tv_sec -ts_begin.tv_sec) +
                        (ts_curr.tv_nsec-ts_begin.tv_nsec) * 1e-9;
        for (int pos = 0; pos < nleds; pos++) {
            duk_get_global_string(ctx, "color");
            duk_push_int(ctx, pos);
            duk_push_number(ctx, elapsed);
            struct rgb rgb;
            if (duk_pcall(ctx, 2) == DUK_EXEC_SUCCESS) {
                rgb = get_rgb_color(ctx);
                duk_pop(ctx);
            } else {
                const char * error = duk_safe_to_string(ctx, -1);
                if (strncmp(preverror, error, sizeof(preverror)-1) != 0) {
                    strncpy(preverror, error, sizeof(preverror)-1);
                    printf("error: %s\n", error);
                }
                duk_pop(ctx);
                continue;
            }
            packet.dmp.prop_val[1 + 3*pos + 0] = rgb.r;
            packet.dmp.prop_val[1 + 3*pos + 1] = rgb.g;
            packet.dmp.prop_val[1 + 3*pos + 2] = rgb.b;
        }
        if (e131_send(sockfd, &packet, &dest) < 0) {
            err(EXIT_FAILURE, "e131_send");
        }
        packet.frame.seq_number++;
        // dealing with file reloading
        uint64_t curr_mtime = mtime_us(path);
        if (curr_mtime > last_mtime) {
            push_string_file_raw(ctx, path);
            duk_push_string(ctx, path);
            if (duk_pcompile(ctx, 0) != 0) {
                printf("compilation failed: %s\n", duk_safe_to_string(ctx, -1));
            } else {
                if (duk_pcall(ctx, 0) == DUK_EXEC_SUCCESS) {
                } else {
                    printf("error: %s\n", duk_safe_to_string(ctx, -1));
                }
            }
            duk_pop(ctx);
            last_mtime = curr_mtime;
        }
        // sleep
        usleep(5000);
    }

    duk_destroy_heap(ctx);
#ifdef ENABLE_AUDIO
    alcCaptureStop(device);
    alcCaptureCloseDevice(device);
#endif
}
