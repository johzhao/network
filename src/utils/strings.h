#ifndef STRINGS_H
#define STRINGS_H

#include <functional>
#include <string>
#include <vector>

class Strings {
public:
    static std::vector<std::string> Split(const std::string &data, const std::string &delim);

    static std::string TrimSpace(const std::string &data);

    static std::string Trim(const std::string &data, const std::function<bool(char)> &keep_character_callback);

    static std::string Hex2Bin(const std::string &str);

    static std::string Bin2Hex(const std::string &data);
};

#endif //STRINGS_H
