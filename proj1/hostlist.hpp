#include "host.hpp"
#include <set>
#pragma once

class HostList {
  private:
    std::set<Host> m_hosts;

  public:
    HostList();
    void insert(Host h);
    std::set<Host> get_set();
    std::string to_string();
};
