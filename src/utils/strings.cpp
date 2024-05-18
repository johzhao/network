#include "strings.h"

#include <sstream>

std::vector<std::string> Strings::Split(const std::string &data, const std::string &delim) {
    std::vector<std::string> result;

    std::size_t previous = 0;
    std::size_t current = data.find(delim);
    while (current != std::string::npos) {
        if (current > previous) {
            result.push_back(data.substr(previous, current - previous));
        }
        previous = current + delim.length();
        current = data.find(delim, previous);
    }

    if (previous != data.size()) {
        result.push_back(data.substr(previous));
    }

    return result;
}

std::string Strings::TrimSpace(const std::string &data) {
    std::function<bool(char)> keep_character_callback = [](char c) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            return false;
        }

        return true;
    };

    return Strings::Trim(data, keep_character_callback);
}

std::string Strings::Trim(const std::string &data, const std::function<bool(char)> &keep_character_callback) {
    if (data.empty()) {
        return data;
    }

    int data_length = static_cast<int>(data.length());

    int start_position = 0;
    while (start_position < data_length) {
        char character = data[start_position];
        if (keep_character_callback(character)) {
            break;
        }

        ++start_position;
    }

    int end_position = data_length - 1;
    while (end_position >= 0) {
        char character = data[end_position];
        if (keep_character_callback(character)) {
            break;
        }

        --end_position;
    }

    if (end_position <= start_position) {
        return "";
    }

    return data.substr(start_position, end_position - start_position + 1);
}

std::string Strings::Hex2Bin(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i += 2) {
        std::string byte = str.substr(i, 2);
        char chr = (char)strtol(byte.c_str(), nullptr, 16);
        result.push_back(chr);
    }
    return result;
}

std::string Strings::Bin2Hex(const std::string &data) {
    char temp[3] = {0};
    std::stringstream ss;
    for (unsigned char i : data) {
        sprintf(temp, "%02x", i);
        ss << temp;
    }
    return ss.str();
}
