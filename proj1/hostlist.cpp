#include "hostlist.hpp"
#include "misc.hpp"
#include <set>

HostList::HostList() { m_hosts = std::set<Host>(); }

void HostList::insert(Host h) {
    // check if host is already in list
    // if it is, increment packet count
    // if it isn't, add it to the list

    // if host is already in list
    if (m_hosts.find(h) != m_hosts.end()) {
        // increment packet count
        Host to_replace = *(m_hosts.find(h));
        to_replace.increment_packet_count();
        m_hosts.erase(h);
        m_hosts.insert(to_replace);
    } else {
        // add host to list
        m_hosts.insert(h);
    }
}

std::string HostList::to_string() {
    std::string res = "";
    for (Host h : m_hosts) {
        std::string hoststr = add_tabs(h.to_string());
        res += hoststr + "\n";
    }
    return res;
}
