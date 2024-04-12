#include "hostlist.hpp"
#include <map>
#pragma once

class Info {
  private:
    /**
     * @brief Total number of packets found
     */
    int m_packet_qty;

    /**
     * @brief List of unique senders of class Host, with their MAC address, IP
     * addresses (if available), number of packets linked to them, and whether
     * or not they're an ARP machine
     */
    HostList m_senders;
    /**
     * @brief List of unique receivers of class Host, with their MAC address, IP
     * addresses (if available), number of packets linked to them, and whether
     * or not they're an ARP machine
     */
    HostList m_receivers;

    /**
     * @brief List of ARP machines of class Host with their MAC address
     * number of packets linked to them
     */
    HostList m_arp_machines;

    /**
     * @brief HashSet of UDP source ports
     */
    std::set<int> m_UDP_srcs;
    /**
     * @brief HashSet of UDP destination ports
     */
    std::set<int> m_UDP_dests;

    /**
     * @brief HashMap of packet sizes, with key being size and value being
     * number of times size appears
     */
    std::map<int, int> m_packet_sizes;

    int calc_avg_packet_size();

  public:
    Info();

    /**
     * @brief increase packet quantity by 1
     */
    void increment_packet_qty();

    /**
     * @brief add host to senders list, if duplicate, ignore
     *
     * @param sender new sender host
     */
    void add_sender(Host sender);
    /**
     * @brief add host to receivers list, if duplicate, ignore
     *
     * @param receiver new receiver host
     */
    void add_receiver(Host receiver);

    /**
     * @brief add host to ARP machines list if it is an ARP machine
     *
     * @param arp
     */
    void add_arp_machine(Host arp);

    /**
     * @brief add new port to UDP source ports list if not already there
     *
     * @param port int, port num
     */
    void add_UDP_src(int port);
    /**
     * @brief add new port to UDP dest ports list if not already added
     *
     * @param port int, port num
     */
    void add_UDP_dest(int port);

    /**
     * @brief add packet size to list
     *
     * @param size int size of packet
     */
    void add_packet_size(int size);

    /**
     * @brief Converts all info to string
     */
    std::string to_string();
};
