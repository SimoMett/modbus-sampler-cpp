#include "nlohmann/json.hpp"
#include "simomett/common.h"

#pragma once

using nlohmann::json;

typedef uint32_t addr_t;

class ModbusTags
{
public:
    ModbusTags(json &tags);

    bool contain_address_in_words(addr_t);
    bool contain_address_in_floats(addr_t);
    bool contain_address_in_dwords(addr_t);
    bool contain_address_in_coils(addr_t);
    bool contain_address_in_bits(addr_t);

    const std::string & get_word_name(addr_t);
    const std::string & get_float_name(addr_t);
    const std::string & get_dword_name(addr_t);
    const std::string & get_coil_name(addr_t);
    const std::string & get_bit_name(addr_t, unsigned int bit);

private:
    std::unordered_map<addr_t, std::string> words_names;
    std::unordered_map<addr_t, std::string> floats_names;
    std::unordered_map<addr_t, std::string> dwords_names;
    std::unordered_map<addr_t, std::string> coils_names;
    std::unordered_map<addr_t, std::array<std::string, 16>> bits_names;
};