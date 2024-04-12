#include "host.hpp"
#include "info.hpp"
#include <iomanip>
#include <iostream>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap/pcap.h>
#include <stdlib.h>
#define SIZE_ETHERNET 14

void pcap_callback(u_char *user, const struct pcap_pkthdr *arg2,
                   const u_char *arg3);
void print_packet_timing(const struct pcap_pkthdr *packet);
void parse_unique_senders_receivers(Info *info, const u_char *packet);

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

    // initialize a desired_info struct
    Info info = Info(); // cast to u_char to fit pcap_callback arg params
    u_char *info_uchar = (u_char *)&info; // mutable borrow: 1

    // Check that data provided is via Ethernet via pcap_datalink()
    // https://www.man7.org/linux/man-pages/man3/pcap_datalink.3pcap.html
    // source: https://www.tcpdump.org/linktypes.html
    int header_type = pcap_datalink(packet);
    const char *str = header_type == DLT_EN10MB ? "" : "not ";
    printf("Ethernet data was %sprovided\n", str);

    // Loop through packets in the pcap file
    pcap_loop(packet, -1, pcap_callback, info_uchar);
    // pcap_loop(packet, -1, pcap_callback, NULL);

    // Close the pcap file
    pcap_close(packet);

    // print final
    printf("Final\n");
    // convert u_char back to Info class
    info = *(Info *)info_uchar;
    std::cout << info.to_string() << std::endl;
}

/**
 * @brief callback function to be called when calling pcap_loop
 *
 * @param user u_char * to piece of data you want to be modified. Any data type
 * can be cast to u_char
 * @param arg2 const struct pcap_pkthdr packet info struct
 * @param arg3 const u_char packet
 */
void pcap_callback(u_char *user, const struct pcap_pkthdr *header,
                   const u_char *packet) {
    // callback function for pcap_loop
    // arg1: user data
    // arg2: packet header
    // arg3: packet data
    // printf("Packet captured\n");

    Info *info = (Info *)user;
    // increment packet quantity
    info->increment_packet_qty();

    // add packet size:
    info->add_packet_size(header->len);

    // print timing for packet
    print_packet_timing(header);
    parse_unique_senders_receivers(info, packet);
}

/**
 * @brief Parses header of packet, called by pcap_callback
 *
 * @param packet The packet header to parse
 */
void print_packet_timing(const struct pcap_pkthdr *header) {
    // Parse the packet header
    // Print the start time of the packet

    // get ACII time
    time_t time = (time_t)header->ts.tv_sec;
    // add microseconds to time
    time += (time_t)header->ts.tv_usec / 1000000;

    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&time));
    std::cout << "Packet Start Time " << time_str << "." << std::setw(6)
              << std::setfill('0') << header->ts.tv_usec << std::endl;

    // printf("Packet Start Time: %s\n", asctime(gmtime(&time)));
    // Print the capture length of the packet
    printf("Packet Capture Length: %d\n", header->caplen);
    // Print the length of the packet
    printf("Packet Length: %d\n", header->len);
}

void save_packet_size(Info *info, const struct pcap_pkthdr *header) {
    info->add_packet_size(header->len);
}

/**
 * @brief Finds unique senders and receivers, called during callback
 *
 * @param info information being held, Info class
 * @param packet current packet, const u_char pointer
 */
void parse_unique_senders_receivers(Info *info, const u_char *packet) {
    std::string mac_dest;
    std::string mac_src;
    std::string ip_src;
    std::string ip_dest;
    bool is_arp = false;
    bool is_ip = false;
    Host src = Host("", "");
    Host dest = Host("", "");

    // convert to ether header struct to get info
    struct ether_header *eth = (struct ether_header *)packet;
    struct ip *ip = (struct ip *)(packet + SIZE_ETHERNET);
    struct sniff_udp {
        u_short uh_sport;
        u_short uh_dport;
    };
    struct sniff_udp *udp =
        (struct sniff_udp *)(packet + SIZE_ETHERNET + ip->ip_hl * 4);

    // get type of transmission
    uint32_t type = ntohs(eth->ether_type);
    if (type == ETHERTYPE_IP) {
        is_ip = true;
    } else if (type == ETHERTYPE_ARP) {
        is_arp = true;
    } // else it's just ethernet

    // Save MAC addresses
    mac_dest = ether_ntoa((struct ether_addr *)eth->ether_dhost);
    mac_src = ether_ntoa((struct ether_addr *)eth->ether_shost);

    // if IP is true, collect for both, save host vars
    if (is_ip == true) {
        ip_src = std::string(inet_ntoa(ip->ip_src));
        ip_dest = std::string(inet_ntoa(ip->ip_dst));
        src = Host(mac_src, ip_src);
        dest = Host(mac_dest, ip_dest);
    } else {
        src = Host(mac_src, is_arp);
        dest = Host(mac_dest, is_arp);
    }

    info->add_sender(src);
    info->add_receiver(dest);

    if (is_arp) {
        info->add_arp_machine(src);
        info->add_arp_machine(dest);
    }

    // if UDP exists, save ports
    if (udp != NULL) {
        // get src and dest ports
        info->add_UDP_src(ntohs(udp->uh_sport));
        info->add_UDP_dest(ntohs(udp->uh_dport));
        std::cout << "UDP Source Port: " << ntohs(udp->uh_sport) << std::endl;
        std::cout << "UDP Destination Port: " << ntohs(udp->uh_dport)
                  << std::endl;
    }
}
