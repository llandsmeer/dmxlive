#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "libe131/e131.h"
#include "duktape/duktape.h"

int main() {
    int nleds = 150;
    const char * ip = "192.168.1.92";
    const char * js_expr = "i % 16 != t % 16 ? 0 : 255";

    duk_context * ctx = duk_create_heap_default();
    if (!ctx) {
        err(EXIT_FAILURE, "duk_create_heap_default");
    }

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
    for (;;) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts_curr);
        float elapsed = (ts_curr.tv_sec  - ts_begin.tv_sec) +
                        (ts_curr.tv_nsec - ts_begin.tv_nsec) * 1e-9;
        for (size_t pos = 0; pos < nleds; pos++) {
            duk_push_int(ctx, pos);
            duk_put_global_string(ctx, "i");
            duk_push_int(ctx, (int)(50*elapsed));
            duk_put_global_string(ctx, "t");
            duk_eval_string(ctx, js_expr);
            uint8_t level = (int) duk_get_int(ctx, -1);
            packet.dmp.prop_val[1 + 3*pos + 0] = level; // R
            packet.dmp.prop_val[1 + 3*pos + 1] = level; // G
            packet.dmp.prop_val[1 + 3*pos + 2] = level; // B
        }
        if (e131_send(sockfd, &packet, &dest) < 0)
            err(EXIT_FAILURE, "e131_send");
        packet.frame.seq_number++;
        usleep(2500);
    }

    duk_destroy_heap(ctx);
}
