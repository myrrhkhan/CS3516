#include <string>
#pragma once

class Host {
  private:
    std::string m_mac;
    std::string m_ip;
    int m_num_packets;
    bool m_is_arp;

  public:
    Host(std::string mac, std::string ip);
    Host(std::string mac, bool is_arp);

    std::string to_string();

    void increment_packet_count();

    int get_packet_count();

    bool operator<(const Host b) const;
    bool compare(Host a, Host b);
};
