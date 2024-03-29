#include <pcap/pcap.h>
#include <stdlib.h>

void pcap_callback(u_char *user, const struct pcap_pkthdr *arg2,
                   const u_char *arg3);
int main(int argc, char **argv) {

    // validate args, if not 2, error, tell user to submit filename as arg
    if (argc != 2) {
        printf("Incorrect Usage. Please add a file name like so:\n./wireview "
               "<filename>\n");
        return 1;
    }

    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the pcap file, if it fails, print error and exit
    pcap_t *packet = pcap_open_offline(argv[1], errbuf);
    if (packet == NULL) {
        printf("Error opening given pcap file: %s\n", errbuf);
        return 1;
    }

    // Check that data provided is via Ethernet via pcap_datalink()
    // https://www.man7.org/linux/man-pages/man3/pcap_datalink.3pcap.html
    // source: https://www.tcpdump.org/linktypes.html
    int header_type = pcap_datalink(packet);
    char *str = header_type == DLT_EN10MB ? "" : "not ";
    printf("Ethernet data was %sprovided\n", str);

    u_char *user = malloc(10);

    // Loop through packets in the pcap file
    pcap_loop(packet, -1, pcap_callback, NULL);

    free(user);

    // Close the pcap file
    pcap_close(packet);
}

void pcap_callback(u_char *user, const struct pcap_pkthdr *arg2,
                   const u_char *arg3) {
    // callback function for pcap_loop
    // arg1: user data
    // arg2: packet header
    // arg3: packet data
    printf("Packet captured\n");
    // print first arg
    printf("First arg: %s\n", user);
    // print packet header
    printf("Packet Start Time: %ld %d\n", arg2->ts.tv_sec, arg2->ts.tv_usec);
    printf("Packet Capture Length: %d\n", arg2->caplen);
    printf("Packet Length: %d\n", arg2->len);
}
