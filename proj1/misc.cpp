#include "misc.hpp"
#include <string>

std::string add_tabs(std::string str) {
    size_t idx_start = 0;
    std::string from = "\n";
    std::string to = "\n\t";
    while ((idx_start = str.find(from, idx_start)) < str.length() - 2) {
        str.replace(idx_start, 1, to);
        idx_start += from.length();
    }
    str.insert(0, "\t");
    return str;
}
