#include "info.hpp"
#include "misc.hpp"

int Info::calc_avg_packet_size() {
    int total_size = 0;
    int total_occ = 0;
    for (auto i = m_packet_sizes.begin(); i != m_packet_sizes.end(); i++) {
        total_size += i->first * i->second;
        total_occ += i->second;
    }
    return total_size / total_occ;
}

Info::Info() {
    m_packet_qty = 0;

    m_senders = HostList();
    m_receivers = HostList();

    m_arp_machines = HostList();

    m_UDP_srcs = std::set<int>();
    m_UDP_dests = std::set<int>();

    m_packet_sizes = std::map<int, int>();
}

void Info::increment_packet_qty() { m_packet_qty++; }

void Info::add_sender(Host sender) { m_senders.insert(sender); }
void Info::add_receiver(Host receiver) { m_receivers.insert(receiver); }

void Info::add_arp_machine(Host arp) { m_arp_machines.insert(arp); }

void Info::add_UDP_src(int port) { m_UDP_srcs.insert(port); }
void Info::add_UDP_dest(int port) { m_UDP_dests.insert(port); }

void Info::add_packet_size(int packet_size) {
    int num_occ = 1;
    if (m_packet_sizes.find(packet_size) != m_packet_sizes.end()) {
        num_occ = m_packet_sizes.at(packet_size) + 1;
    }
    m_packet_sizes[packet_size] = num_occ;
}

std::string UDP_tostr(std::set<int> udp, std::string label) {
    std::string udp_str = "";
    for (int i : udp) {
        udp_str += std::to_string(i);
        udp_str += " ";
    }
    if (udp_str != "") {
        return "UDP " + label + " Ports:\n\t" + udp_str + "\n";
    }
    return "";
}

std::string Info::to_string() {
    std::string res = "";
    res += "Total number of packets: " + std::to_string(m_packet_qty) + "\n";

    res += "Senders: \n";
    res += add_tabs(m_senders.to_string());
    res += "\n";
    res += "Receivers: \n";
    res += add_tabs(m_receivers.to_string());
    res += "\n";
    res += "ARP machines: \n";
    res += add_tabs(m_arp_machines.to_string());
    res += "\n";

    res += UDP_tostr(m_UDP_srcs, "Source");
    res += UDP_tostr(m_UDP_dests, "Destination");

    res +=
        "Average packet size: " + std::to_string(calc_avg_packet_size()) + "\n";

    // get min and max packet sizes
    // minimum size is first element in map
    // maximum size is last element in map
    res += "Minimum packet size: " +
           std::to_string(m_packet_sizes.begin()->first) + "\n";
    res += "Maximum packet size: " +
           std::to_string(m_packet_sizes.rbegin()->first) + "\n";

    return res;
}
