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

int main() {
    const char * path = "scene.js";
    uint64_t last_mtime = mtime_us(path);

    duk_context * ctx = duk_create_heap_default();
    if (!ctx) {
        err(EXIT_FAILURE, "duk_create_heap_default");
    }

    duk_push_string(ctx, "Object.getOwnPropertyNames(Math).forEach((function(name) { this[name] = Math[name] }).bind(this))");
    if (duk_peval(ctx) != DUK_EXEC_SUCCESS) {
        printf("copy math error: %s\n", duk_safe_to_string(ctx, -1));
    }

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
        // sending data
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts_curr);
        float elapsed = (ts_curr.tv_sec -ts_begin.tv_sec) +
                        (ts_curr.tv_nsec-ts_begin.tv_nsec) * 1e-9;
        for (size_t pos = 0; pos < nleds; pos++) {
            duk_get_global_string(ctx, "color");
            duk_push_int(ctx, pos);
            duk_push_number(ctx, elapsed);
            uint8_t level = 0;
            if (duk_pcall(ctx, 2) == DUK_EXEC_SUCCESS) {
                level = (int) duk_get_int(ctx, -1);
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
            packet.dmp.prop_val[1 + 3*pos + 0] = level; // R
            packet.dmp.prop_val[1 + 3*pos + 1] = level; // G
            packet.dmp.prop_val[1 + 3*pos + 2] = level; // B
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
}
