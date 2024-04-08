#include "host.h"
#include <functional>
#include <list>
#include <netinet/if_ether.h>
#include <pcap/pcap.h>
#include <stdlib.h>

void pcap_callback(u_char *user, const struct pcap_pkthdr *arg2,
                   const u_char *arg3);
void parse_packet_timing(const struct pcap_pkthdr *packet);

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

    struct desired_info {
        int num_packets;
        // create list of unique senders and receivers. list should have both
        // MAC and IP addresses each element of list should have sublist with 2
        // elements, one for MAC one for IP
        std::list<host> unique_senders;
        std::list<host> unique_receivers;

        std::hash<int> UDP_sources;
        std::hash<int> UDP_dests;

        std::list<int> packet_sizes;
    };

    // initialize a desired_info struct
    struct desired_info info_struct = {.num_packets = 0,
                                       .unique_senders = std::list<host>(),
                                       .unique_receivers = std::list<host>(),

                                       .UDP_sources = std::hash<int>(),
                                       .UDP_dests = std::hash<int>(),

                                       .packet_sizes = std::list<int>()};
    // cast to u_char to fit pcap_callback arg params
    u_char *info = (u_char *)&info_struct; // mutable borrow: 1

    // Check that data provided is via Ethernet via pcap_datalink()
    // https://www.man7.org/linux/man-pages/man3/pcap_datalink.3pcap.html
    // source: https://www.tcpdump.org/linktypes.html
    int header_type = pcap_datalink(packet);
    const char *str = header_type == DLT_EN10MB ? "" : "not ";
    printf("Ethernet data was %sprovided\n", str);

    // Loop through packets in the pcap file
    pcap_loop(packet, -1, pcap_callback, info);
    // pcap_loop(packet, -1, pcap_callback, NULL);

    // Close the pcap file
    pcap_close(packet);
}

/**
 * @brief callback function to be called when calling pcap_loop
 *
 * @param user u_char * to piece of data you want to be modified. Any data type
 * can be cast to u_char
 * @param arg2 const struct pcap_pkthdr packet info struct
 * @param arg3
 */
void pcap_callback(u_char *user, const struct pcap_pkthdr *arg2,
                   const u_char *arg3) {
    // callback function for pcap_loop
    // arg1: user data
    // arg2: packet header
    // arg3: packet data
    printf("Packet captured\n");

    struct desired_info *info = (struct desired_info *)user;

    parse_packet_timing(arg2);
}

/**
 * @brief Parses header of packet, called by pcap_callback
 *
 * @param packet The packet header to parse
 */
void parse_packet_timing(const struct pcap_pkthdr *packet) {
    // Parse the packet header
    // Print the start time of the packet
    printf("Packet Start Time: %ld %d\n", packet->ts.tv_sec,
           packet->ts.tv_usec);
    // Print the capture length of the packet
    printf("Packet Capture Length: %d\n", packet->caplen);
    // Print the length of the packet
    printf("Packet Length: %d\n", packet->len);
}

void parse_unique_senders_receivers(u_char *info,
                                    const struct pcap_pkthdr *packet) {
    // Find the source and destination MAC addresses from packet, print it
    struct ether_header *eth_header = (struct ether_header *)packet;
    char *source_mac = eth_header->ether_shost;
    char *dest_mac = eth_header->ether_dhost;

    // Find the source and destination IP addresses from packet, print it
    struct ip *ip_header = (struct ip *)(packet + sizeof(struct ether_header));
    char *source_ip = inet_ntoa(ip_header->);
    char *dest_ip = inet_ntoa(ip_header->);
}
