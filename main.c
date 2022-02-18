#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include "libe131/e131.h"

int main() {
  int sockfd;
  e131_packet_t packet;
  e131_addr_t dest;

  // create a socket for E1.31
  if ((sockfd = e131_socket()) < 0)
    err(EXIT_FAILURE, "e131_socket");

    int nleds = 150;

  // initialize the new E1.31 packet in universe 1 with nleds slots in preview mode
  e131_pkt_init(&packet, 1, 3*nleds);
  memcpy(&packet.frame.source_name, "E1.31 Test Client", 18);
  if (e131_set_option(&packet, E131_OPT_PREVIEW, true) < 0)
    err(EXIT_FAILURE, "e131_set_option");

  // set remote system destination as unicast address
  if (e131_unicast_dest(&dest, "192.168.1.92", E131_DEFAULT_PORT) < 0)
    err(EXIT_FAILURE, "e131_unicast_dest");

  // loop to send cycling levels for each slot
  uint8_t level = 0;
  for (;;) {
    for (size_t pos=0; pos<nleds; pos++) {
      packet.dmp.prop_val[1 + 3*pos + 0] = level; // R
      packet.dmp.prop_val[1 + 3*pos + 1] = level; // G
      packet.dmp.prop_val[1 + 3*pos + 2] = level; // B
    }
    level += 2;
    if (e131_send(sockfd, &packet, &dest) < 0)
      err(EXIT_FAILURE, "e131_send");
    // e131_pkt_dump(stderr, &packet);
    packet.frame.seq_number++;
    usleep(25000);
  }
}
