#include "host.h"

host::host(std::string mac, std::string ip) {
    m_mac = mac;
    m_ip = ip;
    m_is_arp = false;
}

host::host(std::string mac) {
    m_mac = mac;
    m_ip = "";
    m_is_arp = true;
}

std::string host::to_string() {
    std::string to_string = "MAC Address: %s\n", m_mac;
    to_string += (m_ip == "") ? "" : ("IPv4 Address: %s\n", m_ip);
    to_string += ("Number of Packets: %d\n", m_num_packets);
    return to_string;
}

void host::increment_packet_count() { m_num_packets++; }

int host::get_packet_count() { return m_num_packets; }

bool host::operator()(host a, host b) { return a.m_mac < b.m_mac; }
