#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

class InputClosed : public std::exception {};

inline std::string read_text(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string value;
    if (!std::getline(std::cin, value)) throw InputClosed();
    if (!value.empty() && value.back() == '\r') value.pop_back();
    return value;
}

inline int read_number(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        auto value = read_text(prompt);
        std::istringstream parser(value);
        int result = 0;
        char extra = 0;
        if ((parser >> result) && !(parser >> extra) && result >= minimum && result <= maximum) return result;
        std::cout << "输入无效，请输入 " << minimum << " 到 " << maximum << " 之间的整数。\n";
    }
}
