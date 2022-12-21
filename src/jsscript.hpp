#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <sys/stat.h> // stat()
#include <err.h> // err()
#include <duktape/duktape.h>

duk_ret_t js_hsv2rgb(duk_context * ctx) {
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

struct JSScript {
    duk_context * ctx;
    std::vector<std::string> vec_ip;
    std::vector<int> vec_nleds;
    std::string path;
    uint64_t last_mtime;
    std::string preverror = "";

    uint64_t mtime_us(const char * path) {
        struct stat attr;
        stat(path, &attr);
        return attr.st_mtim.tv_sec * 1000000 + attr.st_mtim.tv_nsec / 1000;
    }

    JSScript(const char * filename) {
        path = filename;
        last_mtime = mtime_us(filename);
        ctx = duk_create_heap_default();
        if (!ctx) {
            err(EXIT_FAILURE, "duk_create_heap_default");
        }
        setup_environment();
        duk_push_number(ctx, 0);
        duk_put_global_string(ctx, "amp");

        push_file();
        duk_push_string(ctx, filename);
        if (duk_pcompile(ctx, 0) != 0) {
            printf("initial compilation failed: %s\n", duk_safe_to_string(ctx, -1));
        } else {
            if (duk_pcall(ctx, 0) == DUK_EXEC_SUCCESS) {
            } else {
                printf("error: %s\n", duk_safe_to_string(ctx, -1));
            }
        }
        duk_pop(ctx);

        duk_get_global_string(ctx, "wled_config");
        duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1) != 0) {
            const char * ip_raw = duk_safe_to_string(ctx, -2);
            int nleds = duk_get_int(ctx, -1);
            std::string ip(ip_raw);
            vec_ip.push_back(ip);
            vec_nleds.push_back(nleds);
            duk_pop_2(ctx); // kv pair
        }
        duk_pop(ctx); // enum
        duk_pop(ctx); // wled_config
    }

    void set_amp(float amp) {
        duk_push_number(ctx, amp);
        duk_put_global_string(ctx, "amp");
    }

    void set_number(std::string & name, int value) {
        duk_push_number(ctx, value);
        duk_put_global_string(ctx, name.c_str());
    }

    rgb get_color(int led_idx, float time, int dmx_id) {
        duk_get_global_string(ctx, "color");
        duk_push_int(ctx, led_idx);
        duk_push_number(ctx, time);
        duk_push_number(ctx, dmx_id);
        struct rgb color;
        if (duk_pcall(ctx, 3) == DUK_EXEC_SUCCESS) {
            color = get_rgb_color();
            duk_pop(ctx);
        } else {
            const char * error = duk_safe_to_string(ctx, -1);
            if (preverror != error) {
                preverror = error;
                printf("error: %s\n", error);
            }
            duk_pop(ctx);
            return {0, 0, 0};
        }
        return color;
    }

    void recompile_if_changed() {
        uint64_t curr_mtime = mtime_us(path.c_str());
        if (curr_mtime > last_mtime) {
            push_file();
            duk_push_string(ctx, path.c_str());
            if (duk_pcompile(ctx, 0) != 0) {
                printf("compilation failed: %s\n", duk_safe_to_string(ctx, -1));
            } else {
                if (duk_pcall(ctx, 0) == DUK_EXEC_SUCCESS) {
                    printf("reload\n");
                } else {
                    printf("error: %s\n", duk_safe_to_string(ctx, -1));
                }
            }
            duk_pop(ctx);
            last_mtime = curr_mtime;
        }
    }

    rgb get_rgb_color() {
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

    void setup_environment() {
        duk_push_string(ctx, "Object.getOwnPropertyNames(Math).forEach((function(name) { this[name] = Math[name] }).bind(this))");
        if (duk_peval(ctx) != DUK_EXEC_SUCCESS) {
            printf("copy math error: %s\n", duk_safe_to_string(ctx, -1));
        }
        duk_push_c_function(ctx, js_hsv2rgb, DUK_VARARGS);
        duk_put_global_string(ctx, "hsv");
    }

    const char * push_file() {
        // from duktape/extras/duk-v1-compat/duk_v1_compat.c
        FILE *f = NULL;
        char *buf;
        long sz;  /* ANSI C typing */
        if (!path.c_str()) {
            goto fail;
        }
        f = fopen(path.c_str(), "rb");
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
        perror("Error while reading script file"); // don't exit, might be harmless
        return NULL;
    }

    ~JSScript() {
        duk_destroy_heap(ctx);
    }
};
