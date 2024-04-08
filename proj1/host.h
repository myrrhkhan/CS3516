#include <stdlib.h>
#include <string>

class host {
  private:
    std::string m_mac;
    std::string m_ip;
    int m_num_packets;
    bool m_is_arp;

  public:
    host(std::string mac, std::string ip);
    host(std::string mac);

    std::string to_string();

    void increment_packet_count();

    int get_packet_count();

    bool operator()(host a, host b);
};
