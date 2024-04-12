#include "host.hpp"

Host::Host(std::string mac, std::string ip) {
    m_mac = mac;
    m_ip = ip;
    m_is_arp = false;
    m_num_packets = 1;
}

Host::Host(std::string mac, bool is_arp) {
    m_mac = mac;
    m_ip = "";
    m_is_arp = is_arp;
    m_num_packets = 1;
}

std::string Host::to_string() {

    std::string str = "MAC Address: " + m_mac + "\n";
    if (m_ip != "") {
        str += "IP Address: " + m_ip + "\n";
    }
    if (m_is_arp) {
        str += "Is ARP Machine\n";
    }
    str += "Number of Packets: " + std::to_string(m_num_packets) + "\n";

    return str;
}

void Host::increment_packet_count() { m_num_packets++; }

int Host::get_packet_count() { return m_num_packets; }

bool Host::operator<(Host b) const { return m_mac + m_ip < b.m_mac + b.m_ip; }
