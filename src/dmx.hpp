#include <vector>
#include <string.h>
#include <err.h> // err()
#include <libe131/e131.h>

struct UnicastDMX {
    std::vector<e131_packet_t> packets;
    e131_addr_t dest;
    int sockfd;
    int _nleds = 0;
    const char * source_name = "dmxlive";
    UnicastDMX(const char * ip, int nleds, int universe = 1) {
        _nleds = nleds;
        // init socket
        if ((sockfd = e131_socket()) < 0) {
            err(EXIT_FAILURE, "e131_socket");
        }
        // init packets (one for each 170 leds)
        int packet_idx = 0;
        while (packet_idx * 171 < nleds) {
            packets.emplace_back(e131_packet_t());
            e131_pkt_init(&packets[packet_idx], packet_idx + universe, 512);
            memcpy(&packets[packet_idx].frame.source_name, source_name, strlen(source_name));
            if (e131_set_option(&packets[packet_idx], E131_OPT_PREVIEW, true) < 0) {
                err(EXIT_FAILURE, "e131_set_option");
            }
            packet_idx += 1;
        }
        // set up dest addr
        if (e131_unicast_dest(&dest, ip, E131_DEFAULT_PORT) < 0) {
            err(EXIT_FAILURE, "e131_unicast_dest");
        }
    }

    void set(int led_idx, rgb color) {
        // 171 = amount of rgb values that fit in 512 bytes
        int packet_idx = 0;
        while (led_idx > 171) {
            packet_idx += 1;
            led_idx -= 171;
        }
        // offset 1 = dmx universe
        packets[packet_idx].dmp.prop_val[1 + 3*led_idx + 0] = color.r;
        packets[packet_idx].dmp.prop_val[1 + 3*led_idx + 1] = color.g;
        packets[packet_idx].dmp.prop_val[1 + 3*led_idx + 2] = color.b;
    }

    void send() {
        for (size_t i = 0; i < packets.size(); i++) {
            if (e131_send(sockfd, &packets[i], &dest) < 0) {
                printf("ERROR COULD NOT SEND PACKET[%d]\n", (int)i);
            }
            packets[i].frame.seq_number += 1;
        }
    }
};
